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

#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include <cstddef>

struct TCPSocketSystemInfo;

class TCPSocket {
public:
	enum ShutdownOperation {
		SHUTDOWN_SEND,
		SHUTDOWN_RECEIVE,
		SHUTDOWN_BOTH
	};

public:
	TCPSocket();
	~TCPSocket();

public:
	bool Connect(const char *host, const char *port);
	bool Shutdown(ShutdownOperation what = SHUTDOWN_BOTH);

	bool is_open() const { return is_open_; }
	bool is_connected() const { return is_connected_; }

	int Send(const char *buffer, int length) const;
	int Receive(char *buffer, int length) const;

	template<std::size_t N> int Send(const char (&buffer)[N]) const {
		return Send(buffer, N);
	}
	template<std::size_t N> int Receive(char (&buffer)[N]) const {
		return Receive(buffer, N);
	}

	// Sets timeout for Receive() calls in milliseconds.
	bool SetReceiveTimeout(int timeout);
	
protected:
	bool Open();
	bool Close();

private:
	TCPSocket(const TCPSocket &);
	void operator=(const TCPSocket &);

private:
	bool is_open_;
	bool is_connected_;
	TCPSocketSystemInfo *info_;
};

#endif // !TCPSOCKET_H