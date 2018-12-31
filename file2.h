#pragma once

#define _FILE_OFFSET_BITS 64

#include <stdio.h>

#include <filesystem>
#include <ios>
#include <stdexcept>
#include <cctype>

#ifdef WIN32
#include <io.h>
#include <Windows.h>

#define __fseek64 _fseeki64
#define __ftell64 _ftelli64 

#define SHARE_MODE_EXCLUSIVE _SH_DENYRW
#define SHARE_MODE_WRITING_SHARE_READ _SH_DENYWR
#define SHARE_MODE_READ_ONLY _SH_DENYNO
#define __fopen(name, mode, sharedMode) _fsopen(name, mode, sharedMode)
#else
#include <unistd.h>
#define __fseek64 fseeko
#define __ftell64 ftello
#define SHARED_MODE_EXCLUSIVE 0
#define SHARED_MODE_WRITING_SHARED_READ 0
#define SHARED_MODE_READ_ONLY 0
#define __fopen(name, mode, sharedMode) fopen(name, mode)
#endif

namespace jfio {

// This file contains "throwy" versions of the
// good old fopen, fseek, etc. functions.

/**
 * Try opening the file in mode A.
 * If mode A fails, then will try again in mode B (only if A != B).
 * If both modes fail then a runtime_error is thrown.
 */
static inline std::FILE* fopen2(
  const std::filesystem::path& path,
  const std::string& modeA,
  const std::string& modeB,
  const int sharedMode = SHARE_MODE_EXCLUSIVE
) {
  std::FILE* f = nullptr;
  const auto& pathStr = path.string();
  if ((f = __fopen(pathStr.c_str(), modeA.c_str(), sharedMode)) == nullptr) {
    if (
      modeB.empty() ||
      modeA == modeB ||
      (f = __fopen(pathStr.c_str(), modeB.c_str(), sharedMode)) == nullptr
      ) {
      throw std::runtime_error("Cannot open file " + pathStr);
    }
  }

  return f;
}

static inline int64_t ftell2(std::FILE* f) {
  const auto pos = __ftell64(f);
  if (pos < 0) {
    throw std::runtime_error("ftell failed");
  }

  return pos;
}

static inline void fseek2(std::FILE* f, int64_t offset, int origin) {
  if (__fseek64(f, offset, origin) != 0) {
    throw std::runtime_error("Seek failed. Error code: " + std::to_string(ferror(f)));
  }
}

static inline void fputc2(int ch, std::FILE* f) {
  int x = EOF;
  if ((x = std::fputc(ch, f)) != ch) {
    throw std::runtime_error("Write failed. Error code: " + std::to_string(ferror(f)));
  }
}

static inline uint64_t fputs2(const char* str, std::FILE* f) {
  uint64_t count = 0;
  while (*str) {
    fputc2(*str, f);
    str++;
    count++;
  }

  return count;
}

template<typename _t_buff>
static inline uint64_t fputs2(const _t_buff* buff, uint64_t n, std::FILE* f) {
  for (uint64_t i = 0; i < n; i++, buff++) {
    fputc2(*buff, f);
  }

  return n;
}

/**
 * Read at most n bytes.
 * Returns the number of bytes read.
 */
template<typename _t_buff>
static inline uint64_t fgetn(_t_buff* buff, const uint64_t n, std::FILE* f) {
  uint64_t bytesRead = 0;
  for (bytesRead = 0; bytesRead < n; bytesRead++, buff++) {
    int c = fgetc(f);
    if (c == EOF) {
      break;
    }

    *buff = _t_buff(c);
  }

  return bytesRead;
}

static inline int64_t fgeti64(std::FILE* f) {
  int ch = 0;
  int64_t result = 0;
  uint64_t bytesRead = 0;

  while (bytesRead < 8 && (ch = fgetc(f)) != EOF) {
    result <<= 8;
    result |= ch;
    bytesRead++;
  }

  if (bytesRead != 8) {
    throw std::runtime_error("Failed to read int64");
  }

  return result;
}

static inline void fputi64(int64_t i64, FILE* f) {
  fputc2(unsigned char((i64 >> 56) & 0xFF), f);
  fputc2(unsigned char((i64 >> 48) & 0xFF), f);
  fputc2(unsigned char((i64 >> 40) & 0xFF), f);
  fputc2(unsigned char((i64 >> 32) & 0xFF), f);
  fputc2(unsigned char((i64 >> 24) & 0xFF), f);
  fputc2(unsigned char((i64 >> 16) & 0xFF), f);
  fputc2(unsigned char((i64 >> 8) & 0xFF), f);
  fputc2(unsigned char((i64 >> 0) & 0xFF), f);
}

static inline int32_t fgeti32(std::FILE* f) {
  int ch = 0;
  int32_t result = 0;
  int bytesRead = 0;

  while (bytesRead < 4 && (ch = fgetc(f)) != EOF) {
    result <<= 8;
    result |= ch;
    bytesRead++;
  }

  if (bytesRead != 4) {
    throw std::runtime_error("Failed to read int32");
  }

  return result;
}

static inline void fputi32(int32_t i32, FILE* f) {
  fputc2(unsigned char((i32 >> 24) & 0xFF), f);
  fputc2(unsigned char((i32 >> 16) & 0xFF), f);
  fputc2(unsigned char((i32 >> 8) & 0xFF), f);
  fputc2(unsigned char((i32 >> 0) & 0xFF), f);
}

static inline void fflush2(std::FILE* f) {
  if (std::fflush(f) != 0) {
    throw std::runtime_error("fflush failed. Error code: " + std::to_string(ferror(f)));
  }
}

/**
 * Call FlushFileBuffers on Windows, or fsync on other platforms.
 * There is no fsync1.
 * Just keeping the 2 postfix to be consistet with other functions.
 */
static inline void fsync2(std::FILE* f) {
  #ifdef WIN32
  const auto fileNum = _get_osfhandle(_fileno(f));
  #else
  const auto fileNum = fileno(f);
  #endif

  if (fileNum < 0) {
    throw std::runtime_error("Fail to get file number");
}

  #ifdef WIN32
  const auto syncResult = FlushFileBuffers((HANDLE)fileNum) != 0;
  #else
  const auto syncResult = fsync(fileNum) == 0;
  #endif

  if (!syncResult) {
    throw std::runtime_error("Fail to commit file");
}
}

}