// Copyright (c) 2013 Zeex
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <cstring>

#include <netdb.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "tcpsocket.h"

struct TCPSocketSystemInfo {
  int      fd;
  addrinfo hints;
};

TCPSocket::TCPSocket()
 : is_open_(false),
   is_connected_(false),
   info_(new TCPSocketSystemInfo)
{
  info_->fd = -1;

  std::memset(&info_->hints, 0, sizeof(info_->hints));
  info_->hints.ai_family = AF_INET;
  info_->hints.ai_socktype = SOCK_STREAM;
  info_->hints.ai_protocol = IPPROTO_TCP;

  Open();
}

TCPSocket::~TCPSocket() {
  Close();
  delete info_;
}

bool TCPSocket::Open() {
  info_->fd = socket(info_->hints.ai_family, info_->hints.ai_socktype,
                     info_->hints.ai_protocol);
  is_open_ = info_->fd != -1;
  return is_open_;
}

bool TCPSocket::Connect(const char *host, const char *port) {
  addrinfo *result;
  if (getaddrinfo(host, port, &info_->hints, &result) != 0) {
    return false;
  }

  addrinfo *cur;
  for (cur = result; cur != 0; cur = cur->ai_next) {
    if (connect(info_->fd, cur->ai_addr, cur->ai_addrlen) == 0) {
      break;
    }
  }

  freeaddrinfo(result);

  is_connected_ = cur != 0;
  return is_connected_;
}

int TCPSocket::Send(const char *buffer, int length) const {
  return send(info_->fd, buffer, length, 0);
}

int TCPSocket::Receive(char *buffer, int length) const {
  return recv(info_->fd, buffer, length, 0);
}

bool TCPSocket::Shutdown(ShutdownOperation what) {
  switch (what) {
    case SHUTDOWN_SEND:
      return shutdown(info_->fd, SHUT_WR) == 0;
    case SHUTDOWN_RECEIVE:
      return shutdown(info_->fd, SHUT_RD) == 0;
    case SHUTDOWN_BOTH:
      return shutdown(info_->fd, SHUT_RDWR) == 0;
  }
  return false;
}

bool TCPSocket::Close() {
  bool result = false;

  if (is_connected_) {
    Shutdown(SHUTDOWN_BOTH);
    is_connected_ = false;
  }

  if (is_open_) {
    result = close(info_->fd) == 0;
    info_->fd = -1;
    is_open_ = false;
  }

  return result;
}

bool TCPSocket::SetReceiveTimeout(int timeout) {
  timeval timeout_tv;
  timeout_tv.tv_sec = timeout / 1000;
  timeout_tv.tv_usec = (timeout - timeout_tv.tv_sec) * 1000;
  return setsockopt(info_->fd, SOL_SOCKET, SO_RCVTIMEO, &timeout_tv,
                    sizeof(timeout)) == 0;
}
