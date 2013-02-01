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

#ifndef VERSION_H
#define VERSION_H

#include <string>

class Version {
public:
	Version();
	Version(const std::string &string);
	Version(int major, int minor = 0, int patch = 0,
	        std::string suffix = std::string());

	void FromString(const std::string &s);
	std::string AsString(bool include_trailing_zero = false) const;

	int Compare(const Version &rhs, bool with_suffix = true) const;

	bool operator<(const Version &rhs) const;
	bool operator==(const Version &rhs) const;
	bool operator!=(const Version &rhs) const;

	int major() const { return major_; }
	int minor() const { return minor_; }
	int patch() const { return patch_; }
	std::string suffix() const { return suffix_; }

private:
	int major_;
	int minor_;
	int patch_;
	std::string suffix_;
};

#endif // !VERSION_H
