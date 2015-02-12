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

#ifndef AMXSERVICE_H
#define AMXSERVICE_H

#include <map>

#include <amx/amx.h>

#include "amxscript.h"

template<typename T>
class AMXService {
 public:
  AMXService(AMXScript amx) : amx_(amx) {}

  AMXScript amx() const { return amx_; }

 public:
  static T *CreateInstance(AMXScript amx);
  static T *GetInstance(AMXScript amx);
  static void DestroyInstance(AMXScript amx);

 private:
  AMXScript amx_;

 private:
  typedef std::map<AMX*, T*> ServiceMap;
  static ServiceMap service_map_;
};

template<typename T>
typename AMXService<T>::ServiceMap AMXService<T>::service_map_;

// static
template<typename T>
T *AMXService<T>::CreateInstance(AMXScript amx) {
  T *service = new T(amx);
  service_map_.insert(std::make_pair(amx, service));
  return service;
}

// static
template<typename T>
T *AMXService<T>::GetInstance(AMXScript amx) {
  typename ServiceMap::const_iterator iterator = service_map_.find(amx);
  if (iterator != service_map_.end()) {
    return iterator->second;
  }
  return CreateInstance(amx);
}

// static
template<typename T>
void AMXService<T>::DestroyInstance(AMXScript amx) {
  typename ServiceMap::iterator iterator = service_map_.find(amx);
  if (iterator != service_map_.end()) {
    T *service = iterator->second;
    service_map_.erase(iterator);
    delete service;
  }
}

#endif // !AMXSERVICE_H
