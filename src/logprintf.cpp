// Copyright (c) 2011-2012, Zeex
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met: 
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer. 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution. 
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// // LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <cstdarg>
#include <cstdio>
#include <ctime>

#include "configreader.h"

static const char *kServerCfgPath = "server.cfg";
static const char *kServerLogPath = "server_log.txt"; 

static const int kMaxTimeStampLength = 256;
static const char *kDefaultTimeFormat = "[%H:%M:%S]";

class Log {
public:
	explicit Log();
	~Log();

	std::string GetTimeStamp() const;

	void vprintf(const char *format, va_list args);

private:
	std::FILE *fp_;

	// Same as "logtimeformat" option of "server.cfg".
	std::string time_format_;

	// Duplicate log messages in stdout?
	bool dup_stdout_;
};

Log::Log() 
	: fp_(std::fopen(kServerLogPath, "a"))
	, dup_stdout_(false)
{
	ConfigReader server_cfg(kServerCfgPath);

	time_format_.assign(server_cfg.GetOption("logtimeformat",
			std::string(kDefaultTimeFormat)));

	#if defined LINUX
		// On Linux logprintf doesn't print stuff to console (stdout) unless 
		// otherwise specified.
		if (server_cfg.GetOption("output", false)) {
			dup_stdout_ = true;
		}
	#endif
}

Log::~Log() {
	if (fp_ != 0) {
		std::fclose(fp_);
	}
}

std::string Log::GetTimeStamp() const {
	std::time_t time;
	std::time(&time);

	struct tm *time_info = std::localtime(&time);

	char buffer[kMaxTimeStampLength];
	std::strftime(buffer, sizeof(buffer), time_format_.c_str(), time_info);

	return std::string(buffer);
}

void Log::vprintf(const char *format, va_list args) {
	std::string time = GetTimeStamp();

	if (fp_ != 0) {
		if (!time.empty()) {
			std::fputs(time.c_str(), fp_);
			std::fputs(" ", fp_);
		}
		std::vfprintf(fp_, format, args);
		std::fputs("\n", fp_);
		std::fflush(fp_);
	}

	std::vfprintf(stdout, format, args);
	std::fputs("\n", stdout);
	std::fflush(stdout);
}

void vlogprintf(const char *format, std::va_list args) {
	static Log server_log;
	server_log.vprintf(format, args);
}

void logprintf(const char *format, ...) {
	std::va_list args;
	va_start(args, format);
	::vlogprintf(format, args);
	va_end(args);
}
