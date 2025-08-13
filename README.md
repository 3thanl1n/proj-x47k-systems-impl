# Multithreaded Web Search Engine

A high-performance search engine implemented in C++ featuring concurrent HTTP request handling, file system crawling, and relevance-based search results.

## Overview

This project implements a complete search engine system with three main components:
- **File System Crawler**: Recursively indexes documents from a specified directory
- **Inverted Word Index**: Efficient data structure for fast word lookups and multi-term queries  
- **HTTP Web Server**: Multithreaded server with custom thread pool for handling concurrent requests

## Features

- **Multithreaded Architecture**: Custom thread pool with pthread synchronization for concurrent client handling
- **HTTP Protocol Support**: Full HTTP/1.1 implementation with IPv4/IPv6 socket programming
- **Search Capabilities**: Single-word and multi-term queries with relevance ranking
- **File Serving**: Static file serving with proper MIME type handling
- **Web Interface**: Clean search interface similar to popular search engines

## Technical Implementation

### Core Components

- **ThreadPool**: Custom implementation using pthread mutex/condition variables
- **WordIndex**: Inverted index using STL unordered_map for O(1) word lookups
- **HttpSocket**: HTTP protocol parser with persistent connection support
- **ServerSocket**: IPv4/IPv6 socket server with proper error handling
- **CrawlFileTree**: Recursive file system crawler with text tokenization

### Key Algorithms

- **Relevance Ranking**: Term frequency scoring across documents
- **Multi-term Search**: Intersection-based query processing
- **Concurrent Processing**: Thread-safe task dispatching and execution

## Building and Running

### Prerequisites

- C++20 compatible compiler (clang++ 15 or later)
- POSIX-compliant system (Linux/macOS)
- pthread library

### Compilation

```bash
make all
```

This will create two executables:
- `searchserver`: Main search server application
- `test_suite`: Unit tests for all components

### Usage

```bash
./searchserver <port> <directory>
```

**Parameters:**
- `port`: Port number for the HTTP server (e.g., 8080)
- `directory`: Root directory to index and serve files from

**Example:**
```bash
./searchserver 8080 ./test_documents
```

### Testing

Run the comprehensive test suite:
```bash
./test_suite
```

## API Endpoints

### Web Interface
- `GET /` - Main search page
- `GET /query?terms=<search_terms>` - Search results page

### File Access
- `GET /static/<file_path>` - Serve static files from indexed directory

## Project Structure

```
├── searchserver.cpp       # Main server application
├── ThreadPool.hpp/cpp     # Custom thread pool implementation
├── WordIndex.hpp/cpp      # Inverted index data structure
├── HttpSocket.hpp/cpp     # HTTP protocol handling
├── ServerSocket.hpp/cpp   # Socket server implementation
├── HttpUtils.hpp/cpp      # HTTP utility functions
├── CrawlFileTree.hpp/cpp  # File system crawler
├── Result.hpp             # Search result data structure
├── Makefile              # Build configuration
└── test_*.cpp            # Unit tests for each component
```

## Example Usage

1. **Start the server:**
   ```bash
   ./searchserver 8080 ./documents
   ```

2. **Access the search interface:**
   Open `http://localhost:8080` in your browser

3. **Perform searches:**
   - Single word: "algorithm"
   - Multiple words: "data structure"
   - View ranked results with document links

4. **Access files directly:**
   `http://localhost:8080/static/path/to/document.txt`

## Technical Details

### Concurrency Model
- Master thread accepts incoming connections
- Worker threads from thread pool handle client requests
- Thread-safe word index allows concurrent read operations
- Proper synchronization prevents race conditions

### Search Algorithm
1. Tokenize query into individual terms
2. Convert to lowercase for case-insensitive matching
3. For single terms: direct index lookup
4. For multiple terms: intersection of document sets
5. Rank results by cumulative term frequency
6. Return sorted results in descending relevance order

### Performance Optimizations
- Efficient STL container usage (unordered_map, deque)
- Minimal memory copying with move semantics
- Optimized file I/O with buffered reading
- Thread pool eliminates thread creation overhead

## Course Context
- Low-level systems programming in C++
- Network programming and socket APIs
- Multithreading and synchronization primitives
- HTTP protocol implementation
- Memory management and resource handling
- Unix system calls and POSIX APIs

## License

Copyright ©2025 University of Pennsylvania. Educational use only.
