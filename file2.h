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

#else
#include <unistd.h>
#define __fseek64 fseeko
#deinfe __ftell64 ftello
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
  const std::string& modeB
) {
  std::FILE* f = nullptr;
  errno_t err;
  const auto& pathStr = path.string();
  if ((err = fopen_s(&f, pathStr.c_str(), modeA.c_str())) != 0) {
    if (
      modeB.empty() ||
      modeA == modeB ||
      (err = fopen_s(&f, pathStr.c_str(), modeB.c_str())) != 0
      ) {
      throw std::runtime_error(
        "Cannot open file " + pathStr + ": " + std::to_string(err)
      );
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
  if (std::fputc(ch, f) != ch) {
    throw std::runtime_error("Write failed. Error code: " + std::to_string(ferror(f)));
  }
}

static inline size_t fputs2(const char* str, std::FILE* f) {
  size_t count = 0;
  while (*str) {
    fputc2(*str, f);
    str++;
    count++;
  }

  return count;
}

static inline size_t fputs2(const char* str, size_t n, std::FILE* f) {
  for (size_t i = 0; i < n; i++, str++) {
    fputc2(*str, f);
  }

  return n;
}

static inline int64_t fgeti64(std::FILE* f) {
  int ch = 0;
  int64_t result = 0;
  size_t bytesRead = 0;

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
  fputc2(int(i64 & 0xFF00000000000000L), f);
  fputc2(int(i64 & 0x00FF000000000000L), f);
  fputc2(int(i64 & 0x0000FF0000000000L), f);
  fputc2(int(i64 & 0x000000FF00000000L), f);
  fputc2(int(i64 & 0x00000000FF000000L), f);
  fputc2(int(i64 & 0x0000000000FF0000L), f);
  fputc2(int(i64 & 0x000000000000FF00L), f);
  fputc2(int(i64 & 0x00000000000000FFL), f);
}

static inline int64_t fgeti32(std::FILE* f) {
  int ch = 0;
  int32_t result = 0;
  size_t bytesRead = 0;

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
  fputc2(int(i32 & 0xFF000000), f);
  fputc2(int(i32 & 0x00FF0000), f);
  fputc2(int(i32 & 0x0000FF00), f);
  fputc2(int(i32 & 0x000000FF), f);
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