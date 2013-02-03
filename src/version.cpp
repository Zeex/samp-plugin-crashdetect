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

#include "version.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

static void SplitString(const std::string &s, char delim,
                        std::vector<std::string> &elems)
{
    std::stringstream stream(s);
    std::string elem;

    while(std::getline(stream, elem, delim)) {
        elems.push_back(elem);
    }
}

Version::Version()
	: major_(0)
	, minor_(0)
	, patch_(0)
{
}

Version::Version(const std::string &string)
	: major_(0)
	, minor_(0)
	, patch_(0)
{
	FromString(string);
}

Version::Version(int major, int minor, int patch, std::string suffix)
	: major_(major)
	, minor_(minor)
	, patch_(patch)
	, suffix_(suffix)
{
}

void Version::FromString(const std::string &s) {
	std::vector<std::string> components;
	SplitString(s, '.', components);

	if (components.size() >= 1) {
		major_ = std::atoi(components[0].c_str());
	}
	if (components.size() >= 2) {
		minor_ = std::atoi(components[1].c_str());
	}
	if (components.size() >= 3) {
		patch_ = std::atoi(components[2].c_str());
	}

	const std::string &last = components.back();
	std::string::size_type suffix_start = last.find_first_not_of("0123456789");

	if (suffix_start != std::string::npos) {
		std::copy(last.begin() + suffix_start,
		          last.end(),
		          std::back_inserter(suffix_));
	}
}

std::string Version::AsString(bool include_trailing_zero) const {
	std::stringstream stream;

	stream << major_ << '.' << minor_;

	if (patch_ != 0 || (patch_ == 0 && include_trailing_zero)) {
		stream << '.' << patch_;
	}

	stream << suffix_;

	return stream.str();
}

int Version::Compare(const Version &rhs, bool with_suffix) const {
	if (major_ < rhs.major_) {
		return -1;
	} else if (major_ == rhs.major_) {
		if (minor_ < rhs.minor_) {
			return -1;
		} else if (minor_ == rhs.minor_) {
			if (patch_ < rhs.patch_) {
				return -1;
			} else if (patch_ == rhs.patch_) {
				return suffix_.compare(rhs.suffix_);
			}
		}
	}
	return 1;
}

bool Version::operator<(const Version &rhs) const {
	return Compare(rhs) < 0;
}

bool Version::operator==(const Version &rhs) const {
	return Compare(rhs) == 0;
}

bool Version::operator!=(const Version &rhs) const {
	return Compare(rhs) != 0;
}
