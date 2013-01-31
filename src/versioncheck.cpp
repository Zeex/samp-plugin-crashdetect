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

#include <cctype>
#include <string>

#include "tcpsocket.h"
#include "version.h"

Version QueryLatestVersion() {
	Version version;

	TCPSocket socket;
	socket.SetReceiveTimeout(3000);

	if (socket.Connect("zeex.github.com", "80")) {
		char send_buffer[] =
			"GET /samp-plugin-crashdetect/version HTTP/1.1\r\n"
			"Host: zeex.github.com\r\n"
			"\r\n";
		socket.Send(send_buffer);

		std::string response;
		char receive_buffer[1024];
		int nbytes;

		while ((nbytes = socket.Receive(receive_buffer)) > 0) {
			response.append(receive_buffer, nbytes);
		}

		if (!response.empty()) {
			std::string version_string;
			std::string::size_type pos = response.find("<html>") - 1;

			while (pos >= 0 && std::isspace(response[pos])) {
				pos--;
			}

			while (pos >= 0 && !std::isspace(response[pos])) {
				version_string.insert(0, &response[pos], 1);
				pos--;
			}

			version.FromString(version_string);
		}
	}

	return version;
}
