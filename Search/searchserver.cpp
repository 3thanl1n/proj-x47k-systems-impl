#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "CrawlFileTree.hpp"
#include "HttpSocket.hpp"
#include "HttpUtils.hpp"
#include "ServerSocket.hpp"
#include "ThreadPool.hpp"
#include "WordIndex.hpp"

using namespace searchserver;

// HTML temp. for search page
const char* const SEARCH_TEMPLATE_STR = R"(
<html><head><title>595gle</title></head>
<body>
<center style="font-size:500%;">
<span style="position:relative;bottom:-0.33em;color:orange;">5</span><span style="color:red;">9</span><span style="color:gold;">5</span><span style="color:blue;">g</span><span style="color:green;">l</span><span style="color:red;">e</span>
</center>
<p>
<div style="height:20px;"></div>
<center>
<form action="/query" method="get">
<input type="text" size=30 name="terms" />
<input type="submit" value="Search" />
</form>
</center><p>
)";

//Create HTTP responses
std::string generate_html_response(const std::string& content, int status = 200) {
  std::string status_text = (status == 200) ? "OK" : "Not Found";
  std::string response =
      "HTTP/1.1 " + std::to_string(status) + " " + status_text + "\r\n"
      "Content-length: " + std::to_string(content.size()) + "\r\n"
      "\r\n" + content;
  return response;
}

std::string generate_plain_response(const std::string& content) {
  std::string response =
      "HTTP/1.1 200 OK\r\n"
      "Content-type: text/plain\r\n"
      "Content-length: " + std::to_string(content.size()) + "\r\n"
      "\r\n" + content;
  return response;
}

std::string generate_404_response() {
  std::string content = "<html><body><h1>404 Not Found</h1></body></html>";
  return generate_html_response(content, 404);
}

// Handle the request
std::string handle_request(const std::string& request_header, WordIndex& index, const std::string& root_dir) {
  // Extract first line of the request
  size_t firstLineEnd = request_header.find("\r\n");
  if (firstLineEnd == std::string::npos) {
    return generate_404_response();
  }
  
  std::string firstLine = request_header.substr(0, firstLineEnd);
  
  //Parse the request line
  std::vector<std::string> components = split(firstLine, " ");
  if (components.size() < 3) {
    return generate_404_response();
  }
  
  std::string method = components[0];
  std::string fullUri = components[1];
  std::string version = components[2];
  
  // Parse the  URL
  URLParser url_parser;
  url_parser.parse(fullUri);
  std::string path = url_parser.path();

  // Home page
  if (path == "/" || path.empty()) {
    return generate_html_response(SEARCH_TEMPLATE_STR);
  }
  
  // Query  handling
  if (path == "/query") {
    if (url_parser.args().find("terms") != url_parser.args().end()) {
      std::string query = url_parser.args()["terms"];
      
      // Make query to lowercase and  split into terms
      std::transform(query.begin(), query.end(), query.begin(),
                    [](unsigned char c) { return std::tolower(c); });
      
      std::vector<std::string> query_terms = split(query, " +");
      std::vector<Result> results;
      
      if (query_terms.size() > 1) {
        results = index.lookup_query(query_terms);
      } else if (!query.empty()) {
        results = index.lookup_word(query);
      }

      std::stringstream html;
      html << SEARCH_TEMPLATE_STR;
      html << "<p><br>\n";
      html << results.size() << " results found for <b>" << escape_html(query) << "</b>\n";
      html << "<p>\n\n<ul>\n";
      
      for (const auto& result : results) {
        html << " <li> <a href=\"/static/" << escape_html(result.doc_name) << "\">"
             << escape_html(result.doc_name) << "</a> [" << result.rank
             << "]<br>\n";
      }
      
      html << "</ul>\n</body>\n</html>\n";
      return generate_html_response(html.str());
    }
    return generate_404_response();
  }

  // Handle  static files
  if (path.find("/static/") == 0) {
    // Extract the path after "/static/"
    std::string file_path = path.substr(8);
    
    // Try to open the file directly
    std::ifstream file(file_path);
    if (file) {
      std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
      return generate_plain_response(content);
    }
    
    
    std::string full_path = root_dir;
    // Remove trailing slash from root_dir
    if (!full_path.empty() && full_path.back() == '/') {
      full_path.pop_back();
    }
    // Remove leading slash from file_path
    if (!file_path.empty() && file_path.front() == '/') {
      file_path = file_path.substr(1);
    }
    full_path += "/" + file_path;
    
    std::ifstream file2(full_path);
    if (file2) {
      std::string content((std::istreambuf_iterator<char>(file2)),
                        std::istreambuf_iterator<char>());
      return generate_plain_response(content);
    }
    
    if (file_path.size() >= 2 && file_path.substr(0, 2) == "./") {
      std::string alt_path = file_path.substr(2);
      std::ifstream file3(alt_path);
      if (file3) {
        std::string content((std::istreambuf_iterator<char>(file3)),
                          std::istreambuf_iterator<char>());
        return generate_plain_response(content);
      }
    }
    
    return generate_404_response();
  }

  return generate_404_response();
}

// Make client handling struct for threads
struct ClientContext {
  HttpSocket client;
  WordIndex* index;
  std::string root_dir;

  ClientContext(HttpSocket&& c, WordIndex* idx, const std::string& dir)
      : client(std::move(c)), index(idx), root_dir(dir) {}
};

// Client handler function
void client_handler(void* arg) {
  std::unique_ptr<ClientContext> ctx(static_cast<ClientContext*>(arg));

  try {
    while (auto request = ctx->client.next_request()) {
      std::string response = handle_request(*request, *ctx->index, ctx->root_dir);
      if (!ctx->client.write_response(response)) {
        break;
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Client handling error: " << e.what() << "\n";
  }
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <port> <directory>\n";
    return EXIT_FAILURE;
  }

  const uint16_t port = static_cast<uint16_t>(std::stoi(argv[1]));
  const std::string root_dir = argv[2];

  // Build search index
  auto index_opt = crawl_filetree(root_dir);
  if (!index_opt) {
    std::cerr << "Failed to build search index\n";
    return EXIT_FAILURE;
  }
  WordIndex index = std::move(*index_opt);

  try {
    // Set up the server
    ServerSocket server(AF_INET6, "::", port);
    std::cout << "Accepting connections...\n";

    ThreadPool pool(4);

    // Main server loop
    while (true) {
      auto client_opt = server.accept_client();
      if (!client_opt) {
        continue;
      }

      // Create client struc and dispatch to thread pool
      ClientContext* ctx = new ClientContext(std::move(*client_opt), &index, root_dir);
      ThreadPool::Task task{};
      task.func_ = client_handler;
      task.arg_ = ctx;
      pool.dispatch(task);
    }
  } catch (const std::exception& e) {
    std::cerr << "Server error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}