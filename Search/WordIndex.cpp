#include "./WordIndex.hpp"

#include <algorithm>

namespace searchserver {

WordIndex::WordIndex() = default;

size_t WordIndex::num_words() {
  return word_map.size();
}

void WordIndex::record(const string& word, const string& doc_name) {
  // Increment the count for this word in this document
  word_map[word][doc_name]++;
}

vector<Result> WordIndex::lookup_word(const string& word) {
  vector<Result> result;
  
  // Check if the word exists in index
  auto it = word_map.find(word);
  if (it != word_map.end()) {
    // For each document containing this word
    for (const auto& doc_count_pair : it->second) {
      // Create a Result with the document name and count
      Result r;
      r.doc_name = doc_count_pair.first;
      r.rank = doc_count_pair.second;
      result.push_back(r);
    }
    
    // Sort results by rank in descending order
    std::sort(result.begin(), result.end(),
              [](const Result& a, const Result& b) {
                return a.rank > b.rank;
              });
  }
  return result;
}

vector<Result> WordIndex::lookup_query(const vector<string>& query) {
  if (query.empty()) {
    return {};
  }
  
  if (query.size() == 1) {
    // If only one word then use lookup_word
    return lookup_word(query[0]);
  }
  
  // Get documents containing the first word
  std::unordered_map<string, int> doc_ranks;
  auto first_word_results = lookup_word(query[0]);
  for (const auto& result : first_word_results) {
    doc_ranks[result.doc_name] = result.rank;
  }
  
  // For each additional word, keep only documents that also contain this word
  for (size_t i = 1; i < query.size(); i++) {
    auto word_results = lookup_word(query[i]);
    
    // Create a map of documents containing this word
    std::unordered_map<string, int> word_docs;
    for (const auto& result : word_results) {
      word_docs[result.doc_name] = result.rank;
    }
    
    // Remove documents that don't contain this word
    std::vector<string> to_remove;
    for (const auto& [doc, _] : doc_ranks) {
      if (word_docs.find(doc) == word_docs.end()) {
        to_remove.push_back(doc);
      }
    }
    
    for (const auto& doc : to_remove) {
      doc_ranks.erase(doc);
    }
    
    // Update ranks for remaining documents
    for (auto& [doc, rank] : doc_ranks) {
      if (word_docs.find(doc) != word_docs.end()) {
        rank += word_docs[doc];
      }
    }
  }
  
  // Convert map to vector of Results
  vector<Result> results;
  for (const auto& [doc, rank] : doc_ranks) {
    Result r;
    r.doc_name = doc;
    r.rank = rank;
    results.push_back(r);
  }
  
  // Sort by rank in descending order
  std::sort(results.begin(), results.end(), 
            [](const Result& a, const Result& b) {
              return a.rank > b.rank;
            });
  
  return results;
}

}  // namespace searchserver