/*
 *  Copyright (c) 2010 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "latebindingsymboltable_linux.h"

#include <dlfcn.h>
#include <string>

inline static const char* GetDllError() {
  char* err = dlerror();
  if (err) {
    return err;
  } else {
    return "No error";
  }
}

DllHandle InternalLoadDll(const char dll_name[]) {
  DllHandle handle = dlopen(dll_name, RTLD_NOW);
  if (handle == kInvalidDllHandle) {
     std::string error = GetDllError();
  }
  return handle;
}

void InternalUnloadDll(DllHandle handle) {
// TODO(pbos): Remove this dlclose() exclusion when leaks and suppressions from
// here are gone (or AddressSanitizer can display them properly).
//
// Skip dlclose() on AddressSanitizer as leaks including this module in the
// stack trace gets displayed as <unknown module> instead of the actual library
// -> it can not be suppressed.
// https://code.google.com/p/address-sanitizer/issues/detail?id=89
#if !defined(ADDRESS_SANITIZER)
  if (dlclose(handle) != 0) {
   std::string error = GetDllError();
  }
#endif  // !defined(ADDRESS_SANITIZER)
}

static bool LoadSymbol(DllHandle handle,
                       const char* symbol_name,
                       void** symbol) {
  *symbol = dlsym(handle, symbol_name);
  char* err = dlerror();
  if (err) {
    return false;
  } else if (!*symbol) {
    return false;
  }
  return true;
}

// This routine MUST assign SOME value for every symbol, even if that value is
// NULL, or else some symbols may be left with uninitialized data that the
// caller may later interpret as a valid address.
bool InternalLoadSymbols(DllHandle handle,
                         int num_symbols,
                         const char* const symbol_names[],
                         void* symbols[]) {
  dlerror();
  for (int i = 0; i < num_symbols; ++i) {
    if (!LoadSymbol(handle, symbol_names[i], &symbols[i])) {
      return false;
    }
  }
  return true;
}

