/*
 * Copyright ©2025 Travis McGaha.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Pennsylvania
 * CIT 5950 for use solely during Spring Semester 2025 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <arpa/inet.h>   // for inet_ntop()
#include <netdb.h>       // for getaddrinfo()
#include <sys/socket.h>  // for socket(), getaddrinfo(), etc.
#include <sys/types.h>   // for socket(), getaddrinfo(), etc.
#include <unistd.h>      // for close(), fcntl()
#include <cerrno>        // for errno, used by strerror()
#include <cstring>       // for memset, strerror()
#include <iostream>      // for std::cerr, etc.
#include <stdexcept>

#include "./ServerSocket.hpp"

using namespace std;

namespace searchserver {

ServerSocket::ServerSocket(sa_family_t family,
                           const string& address,
                           uint16_t port)
    : port_(port), listen_sock_fd_() {
  // TODO:
  // - create a stream socket
  // - set the socket option to enable re-use of the port number
  // - bind the socket to the specified address and port
  // - mark the socket as listening

  // since the address and port are given to you, you must populate the
  // address structures necessary yourself. You only need to set the
  // port, address and family field of those structs.
  // The rest can be zero'd out.

  if (family != AF_INET6 && family != AF_INET) {
    throw invalid_argument("Specified family is is not IPv4 nor IPv6");
  }

  listen_sock_fd_ = socket(family, SOCK_STREAM, 0);
  if (listen_sock_fd_ == -1) {
    throw runtime_error("socket() failed: " + string(strerror(errno)));
  }

  int optval = 1;
  setsockopt(listen_sock_fd_, SOL_SOCKET, SO_REUSEADDR, &optval,
             sizeof(optval));

  if (family == AF_INET) {
    struct sockaddr_in sa4{};
    memset(&sa4, 0, sizeof(sa4));
    sa4.sin_family = AF_INET;
    sa4.sin_port = htons(port);

    if (address.empty()) {
      sa4.sin_addr.s_addr = INADDR_ANY;
    } else {
      if (inet_pton(AF_INET, address.c_str(), &(sa4.sin_addr)) != 1) {
        close(listen_sock_fd_);
        throw runtime_error("IPv4 address passed in is invalid");
      }
    }

    if (bind(listen_sock_fd_, reinterpret_cast<struct sockaddr*>(&sa4), sizeof(sa4)) == -1) {
      close(listen_sock_fd_);
      throw runtime_error("bind() failed: " + string(strerror(errno)));
    }
  } else {
    struct sockaddr_in6 sa6{};
    memset(&sa6, 0, sizeof(sa6));
    sa6.sin6_family = AF_INET6;
    sa6.sin6_port = htons(port);

    if (address.empty()) {
      sa6.sin6_addr = in6addr_any;
    } else {
      if (inet_pton(AF_INET6, address.c_str(), &(sa6.sin6_addr)) != 1) {
        close(listen_sock_fd_);
        throw runtime_error("IPv6 address passed in is invalid");
      }
    }

    if (bind(listen_sock_fd_, reinterpret_cast<struct sockaddr*>(&sa6), sizeof(sa6)) == -1) {
      close(listen_sock_fd_);
      throw runtime_error("bind() failed: " + string(strerror(errno)));
    }
  }

  if (listen(listen_sock_fd_, SOMAXCONN) != 0) {
    close(listen_sock_fd_);
    throw runtime_error("listen() failed: " + string(strerror(errno)));
  }
}

ServerSocket::~ServerSocket() {
  // Close the listening socket if it's not zero.  The rest of this
  // class will make sure to zero out the socket if it is closed
  // elsewhere.
  if (listen_sock_fd_ != -1)
    close(listen_sock_fd_);
  listen_sock_fd_ = -1;
}

optional<HttpSocket> ServerSocket::accept_client() const {
  // TODO accept the next client connection and return it as an HttpSocket
  // object nullopt on error
  struct sockaddr_storage caddr{};
  socklen_t caddr_len = sizeof(caddr);
  int client_fd = accept(
      listen_sock_fd_, reinterpret_cast<struct sockaddr*>(&caddr), &caddr_len);

  if (client_fd < 0) {
    return nullopt;
  }

  return HttpSocket(client_fd, caddr_len,
                    reinterpret_cast<struct sockaddr*>(&caddr));
}

}  // namespace searchserver