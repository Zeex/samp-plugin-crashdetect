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

#include <winsock2.h>
#include <ws2tcpip.h>

#include "tcpsocket.h"

struct TCPSocketSystemInfo {
	SOCKET   socket;
	addrinfo hints;
	WSADATA  wsdata;
};

TCPSocket::TCPSocket()
	: is_open_(false)
	, is_connected_(false)
	, info_(new TCPSocketSystemInfo)
{
	info_->socket = INVALID_SOCKET;

	ZeroMemory(&info_->hints, sizeof(info_->hints));
	info_->hints.ai_family = AF_INET;
	info_->hints.ai_socktype = SOCK_STREAM;
	info_->hints.ai_protocol = IPPROTO_TCP;

	if (WSAStartup(WINSOCK_VERSION, &info_->wsdata) == 0) {
		Open();
	}
}

TCPSocket::~TCPSocket() {
	Close();
	WSACleanup();
	delete info_;
}

bool TCPSocket::Open() {
	info_->socket = socket(info_->hints.ai_family, info_->hints.ai_socktype,
	                       info_->hints.ai_protocol);
	is_open_ = info_->socket != INVALID_SOCKET;
	return is_open_;
}

bool TCPSocket::Connect(const char *host, const char *port) {
	addrinfo *result;
	if (getaddrinfo(host, port, &info_->hints, &result) != 0) {
		return false;
	}

	addrinfo *cur;
	for (cur = result; cur != 0; cur = cur->ai_next) {
		if (connect(info_->socket, cur->ai_addr, cur->ai_addrlen) == 0) {
			break;
		}
	}

	freeaddrinfo(result);

	is_connected_ = cur != 0;
	return is_connected_;
}

int TCPSocket::Send(const char *buffer, int length) const {
	int nbytes = send(info_->socket, buffer, length, 0);
	return nbytes == SOCKET_ERROR ? -1 : nbytes;
}

int TCPSocket::Receive(char *buffer, int length) const {
	int nbytes = recv(info_->socket, buffer, length, 0);
	return nbytes == SOCKET_ERROR ? -1 : nbytes;
}

bool TCPSocket::Shutdown(ShutdownOperation what) {
	switch (what) {
		case SHUTDOWN_SEND:
			return shutdown(info_->socket, SD_RECEIVE) == 0;
		case SHUTDOWN_RECEIVE:
			return shutdown(info_->socket, SD_RECEIVE) == 0;
		case SHUTDOWN_BOTH:
			return shutdown(info_->socket, SD_BOTH) == 0;
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
		result = closesocket(info_->socket) == 0;
		info_->socket = INVALID_SOCKET;
		is_open_ = false;
	}

	return result;
}

bool TCPSocket::SetReceiveTimeout(int timeout) {
	return setsockopt(info_->socket, SOL_SOCKET, SO_RCVTIMEO,
		                reinterpret_cast<char*>(&timeout), sizeof(timeout)) == 0;
}
