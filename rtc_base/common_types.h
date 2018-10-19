/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef COMMON_TYPES_H_
#define COMMON_TYPES_H_

#include <stddef.h>
#include <string.h>
#include <ostream>
#include <string>
#include <vector>
#include "rtc_base/checks.h"
#include "rtc_base/deprecation.h"
#include "typedefs.h"  // NOLINT(build/include)

#if defined(_MSC_VER)
// Disable "new behavior: elements of array will be default initialized"
// warning. Affects OverUseDetectorOptions.
#pragma warning(disable : 4351)
#endif

#if defined(WEBRTC_EXPORT)
#define WEBRTC_DLLEXPORT _declspec(dllexport)
#elif defined(WEBRTC_DLL)
#define WEBRTC_DLLEXPORT _declspec(dllimport)
#else
#define WEBRTC_DLLEXPORT
#endif

#ifndef NULL
#define NULL 0
#endif

#define RTP_PAYLOAD_NAME_SIZE 32u

#if defined(WEBRTC_WIN) || defined(WIN32)
// Compares two strings without regard to case.
#define STR_CASE_CMP(s1, s2) ::_stricmp(s1, s2)
// Compares characters of two strings without regard to case.
#define STR_NCASE_CMP(s1, s2, n) ::_strnicmp(s1, s2, n)
#else
#define STR_CASE_CMP(s1, s2) ::strcasecmp(s1, s2)
#define STR_NCASE_CMP(s1, s2, n) ::strncasecmp(s1, s2, n)
#endif

namespace webrtc {

class RewindableStream {
 public:
  virtual ~RewindableStream() {}
  virtual int Rewind() = 0;
};

class InStream : public RewindableStream {
 public:
  // Reads |len| bytes from file to |buf|. Returns the number of bytes read
  // or -1 on error.
  virtual int Read(void* buf, size_t len) = 0;
};

class OutStream : public RewindableStream {
 public:
  // Writes |len| bytes from |buf| to file. The actual writing may happen
  // some time later. Call Flush() to force a write.
  virtual bool Write(const void* buf, size_t len) = 0;
};

enum FileFormats {
  kFileFormatWavFile = 1,
  kFileFormatCompressedFile = 2,
  kFileFormatPreencodedFile = 4,
  kFileFormatPcm16kHzFile = 7,
  kFileFormatPcm8kHzFile = 8,
  kFileFormatPcm32kHzFile = 9,
  kFileFormatPcm48kHzFile = 10
};


}  // namespace webrtc

#endif  // COMMON_TYPES_H_
