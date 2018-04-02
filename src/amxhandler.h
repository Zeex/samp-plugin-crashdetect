// Copyright (c) 2012-2015 Zeex
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

#ifndef AMXHANDLER_H
#define AMXHANDLER_H

#include <map>
#include <amx/amx.h>

template<typename T>
class AMXHandler {
 public:
  AMXHandler(AMX *amx) : amx_(amx) {}

  AMX *amx() const { return amx_; }

 public:
  static T *CreateHandler(AMX *amx);
  static T *GetHandler(AMX *amx);
  static void DestroyHandler(AMX *amx);

 private:
  AMX *amx_;

 private:
  typedef std::map<AMX*, T*> HandlerMap;
  static HandlerMap handlers_;
};

template<typename T>
typename AMXHandler<T>::HandlerMap AMXHandler<T>::handlers_;

// static
template<typename T>
T *AMXHandler<T>::CreateHandler(AMX *amx) {
  T *handler = new T(amx);
  handlers_.insert(std::make_pair(amx, handler));
  return handler;
}

// static
template<typename T>
T *AMXHandler<T>::GetHandler(AMX *amx) {
  typename HandlerMap::const_iterator iterator = handlers_.find(amx);
  if (iterator != handlers_.end()) {
    return iterator->second;
  }
  return 0;
}

// static
template<typename T>
void AMXHandler<T>::DestroyHandler(AMX *amx) {
  typename HandlerMap::iterator iterator = handlers_.find(amx);
  if (iterator != handlers_.end()) {
    T *handler = iterator->second;
    handlers_.erase(iterator);
    delete handler;
  }
}

#endif // !AMXHANDLER_H
