#pragma once

#include <string>
#include <filesystem>
#include "jfile.h"

namespace jfio {

/**
 * Opens the main file and the journal file.
 *
 * This function will also try to recover and flush any existing journal data.
 * This could happen when journalling finished previously, but failed to
 * write to the main file.
 */
JFile jfopen(
  const std::filesystem::path& mainFilePath,
  const std::filesystem::path& journalFilePath,
  // Try to open the main file in this mode first.
  const std::string& mainFileModeA,
  // If mode A fails, then mode B will be used to retry.
  // If mode B fails, then a runtime_error will be thrown.
  const std::string& mainFileModeB
);

/**
 * Similar to fseek. Throw if the seek is invalid (e.g. seek(1, SEEK_END)).
 * Returns the new position.
 * Note: This function does not actually perform an fseek on the main file,
 * since the main file content might not exist before the journal got flushed.
 */
int64_t jfseek(JFile& file, int64_t offset, int origin);

/**
 * Returns the current position.
 */
int64_t jftell(const JFile& file);

/**
 * Writes one character to the journal at the jftell() position.
 */
void jfputc(int ch, JFile& file);

/**
 * Writes one the null-terminated string to the journal at the jftell() position.
 */
void jfputs(const char* str, JFile& file);

/**
 * Writes n characters to the journal at the jftell() position.
 */
void jfputs(const char* str, int64_t n, JFile& file);

/**
 * Writes n bytes to the journal at the jftell() position.
 */
void jfputs(const unsigned char* str, int64_t n, JFile& file);

/**
 * Writes a 32 bit integer to the file as 4 bytes.
 */
void jfputi32(int32_t i, JFile& file);

/**
 * Writes as 64 bit integer to the file as 8 bytes.
 */
void jfputi64(int64_t i, JFile& file);

/**
 * Reads a charater from the main file at jftell() position.
 * Before the journal is flushed, this function will return EOF.
 */
int jfgetc(JFile& file);

/**
 * Reads at most n characters from the main file at jftell() position.
 * Returns the number of characters read.
 * Before the journal is flushed, this function will return EOF.
 */
int jfgetn(char* s, uint32_t count, JFile& file);

/**
 * Reads a 32 bit number (4 bytes) from the main file at jftell() position.
 * Before the journal is flushed, of the main file does not have at least 4 bytes,
 * a runtime_error will be thrown.
 */
int32_t jfgeti32(JFile& file);

/**
 * Reads a 64 bit number (8 bytes) from the main file at jftell() position.
 * Before the journal is flushed, of the main file does not have at least 4 bytes,
 * a runtime_error will be thrown.
 */
int64_t jfgeti64(JFile& file);

/**
 * Commits the writes in the journal to the main file.
 */
void jfflush(JFile& file);

/**
 * Clear all the (unflushed) journal progress,
 * and restores the file position to before the
 * current journaling session starts.
 */
void jfclear(JFile& file);

/**
 * Closes all the file handles of the file.
 */
void jfclose(JFile& file);
}