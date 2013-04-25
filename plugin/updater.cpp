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

#include <string>

#include "tcpsocket.h"
#include "thread.h"
#include "updater.h"
#include "version.h"

namespace {

const std::string kCrLf = "\r\n";
const std::string kHttpHeaderDelim = kCrLf + kCrLf;

const std::string kHostString = "zeex.github.io";
const std::string kPortString = "80";
const std::string kRequestUriString = "/samp-plugin-crashdetect/version";

const std::string kRequestString =
  "GET " + kRequestUriString + " HTTP/1.1" + kCrLf +
  "Host: " + kHostString + kHttpHeaderDelim; 

const int kReceiveTimeoutMs = 60000;
const int kReceiveBufferSize = 1024;

void FetchVersionThread(void *args) {
  Updater::FetchVersion();
}

} // anonymous namespace

Thread Updater::fetch_thread_(FetchVersionThread);
bool Updater::version_fetched_ = true;
Version Updater::latest_version_;

// static
void Updater::InitiateVersionFetch() {
  fetch_thread_.Run();
}

// static
void Updater::FetchVersion() {
  Version version;
  version_fetched_ = false;

  TCPSocket socket;
  socket.SetReceiveTimeout(kReceiveTimeoutMs);

  if (socket.Connect(kHostString.c_str(), kPortString.c_str())) {
    socket.Send(kRequestString);

    std::string response;
    char receive_buffer[kReceiveBufferSize];
    int bytes_received;

    while ((bytes_received = socket.Receive(receive_buffer)) > 0) {
      response.append(receive_buffer, bytes_received);
    }

    if (!response.empty()) {
      // Strip the HTTP header of reponse. It ends with a double CRLF followed
      // by the message body (which is a version string).
      std::string::size_type pos = response.find(kHttpHeaderDelim);
      if (pos != std::string::npos) {
        response.erase(response.begin(),
                       response.begin() + pos + kHttpHeaderDelim.length());
      }

      // The version is a single line, so remove anything after the newline.
      std::string::size_type newline = response.find_first_of(kCrLf);
      response.erase(response.begin() + newline, response.end());

      version.FromString(response);
    }
  }

  latest_version_ = version;
  version_fetched_ = true;
}
