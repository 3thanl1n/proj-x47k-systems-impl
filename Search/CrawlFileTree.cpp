/*
 * Copyright ©2025 Travis McGaha.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Pennsylvania
 * CIT 5950 for use solely during Fall Semester 2025 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include "./CrawlFileTree.hpp"
#include "./HttpUtils.hpp"
#include <fstream>
#include <algorithm>
#include <cctype>

using std::string;
using std::optional;
using std::nullopt;

namespace searchserver {

//////////////////////////////////////////////////////////////////////////////
// Internal helper functions and constants
//////////////////////////////////////////////////////////////////////////////

static bool handle_dir(const string& dir_path, WordIndex& index);

// Read and parse the specified file, then inject it into the MemIndex.
static void handle_file(const string& fpath, WordIndex& index);


//////////////////////////////////////////////////////////////////////////////
// Externally-exported functions
//////////////////////////////////////////////////////////////////////////////

optional<WordIndex> crawl_filetree(const string& root_dir) {
  // Create a new word index
  WordIndex index;
  
  // Call handle_dir on the root directory to start the crawl
  if (!handle_dir(root_dir, index)) {
    // Return nullopt if there was an error processing the directory
    return nullopt;
  }
  
  // Return the populated index
  return index;
}


//////////////////////////////////////////////////////////////////////////////
// Internal helper functions
//////////////////////////////////////////////////////////////////////////////

static bool handle_dir(const string& dir_path, WordIndex& index) {
  // Recursively descend into the passed-in directory, looking for files and
  // subdirectories.  Any encountered files are processed via handle_file(); any
  // subdirectories are recusively handled by handle_dir().

  // Use the readdir function from HttpUtils to get the directory entries
  auto entries_opt = readdir(dir_path);
  
  // Check if able to read directory
  if (!entries_opt) {
    return false;
  }
  
  // Get the entries from the optional
  auto entries = *entries_opt;
  
  // Process each entry
  for (const auto& entry : entries) {
    // Skip "." and ".." directories
    if (entry.name == "." || entry.name == "..") {
      continue;
    }
    
    // Construct the full path to the entry
    string full_path = dir_path;
    if (full_path.back() != '/') {
      full_path += "/";
    }
    full_path += entry.name;
    
    if (entry.is_dir) {
      // If it's a directory, recursively handle it
      if (!handle_dir(full_path, index)) {
        return false;
      }
    } else {
      // If it's a file, process it
      handle_file(full_path, index);
    }
  }
  
  return true;
}

static void handle_file(const string& fpath, WordIndex &index) {
  // Read the contents of the specified file into a string
  std::ifstream file(fpath);
  if (!file.is_open()) {
    return;
  }
  
    string content((std::istreambuf_iterator<char>(file)),
                  std::istreambuf_iterator<char>());
  
  // Close the file
  file.close();
  
  // Define the delimiters
  const string delimiters = " \r\t\v\n,.:;?!";
  vector<string> tokens = split(content, delimiters);
  
  // Record each token as a word into the WordIndex
  for (string token : tokens) {
    // Skip empty tokens
    if (token.empty()) {
      continue;
    }
    
    // Convert to lowercase
    std::transform(token.begin(), token.end(), token.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    
    // Record the word in the index using the exact file path
    index.record(token, fpath);
  }
}

}  // namespace searchserver