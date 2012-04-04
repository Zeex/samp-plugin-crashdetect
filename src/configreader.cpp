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

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <locale>
#include <stdexcept>
#include <sstream>
#include <string>

#include "configreader.h"

ConfigReader::ConfigReader() 
	: loaded_(false)
{
}

ConfigReader::ConfigReader(const std::string &filename) 
	: loaded_(false)
{
	LoadFile(filename);
}

struct is_not_space {
	bool operator()(char c) {
		return !(c == ' ' || c == '\r' || c == '\n' || c == '\t');
	}
};

static inline std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), is_not_space()));
        return s;
}

static inline std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), is_not_space()).base(), s.end());
        return s;
}

static inline std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
}

bool ConfigReader::LoadFile(const std::string &filename) {
	std::ifstream cfg(filename.c_str());

	if (cfg.is_open()) {
		loaded_ = true;

		std::string line;
		while (std::getline(cfg, line, '\n')) {
			std::stringstream linestream(line);

			// Get first word in the line
			std::string name;
			std::getline(linestream, name, ' ');
			trim(name);

			// Get the rest
			std::string value;
			std::getline(linestream, value, '\n');
			trim(value);

			options_[name] = value;
		}
	}

	return loaded_;
}

template<>
std::string ConfigReader::GetOption<std::string>(const std::string &name, const std::string &defaultValue) const {
	std::map<std::string, std::string>::const_iterator it = options_.find(name);
	if (it == options_.end()) {
		return defaultValue;
	}
	return it->second;
}
