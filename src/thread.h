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

typedef void (*ThreadRoutine)(void *args);

class ThreadSystemInfo;

// Thread encapsulates operating system's threading APIs and provides very
// basic threading capabilities. Each thread is associated with a function
// that is executed when you Run() the thread. It is also possible to be
// somewhat more OOP-ish and inherit from Thread overriding its Start() method
// which by default simply calls the associated function. 
//
// NOTE: Remember that a Thread object must not be destroyed while the thread
// is running! You can use Join() to wait for the thread to exit or allocate
// the Thread on the heap, but never EVER let it die too early!
class Thread {
public:
	Thread(ThreadRoutine func);
	virtual ~Thread();

	void Run(void *args = 0);
	void Finish(); // user code shouldn't call this
	void Join();

	virtual void Start(void *args);

protected:
	Thread();

private:
	Thread(const Thread &);
	void operator=(const Thread &);

private:
	ThreadRoutine    func_;
	ThreadSystemInfo *info_;
};
