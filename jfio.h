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
long jfseek(JFile& file, long offset, int origin);

/**
 * Returns the current position.
 */
long jftell(const JFile& file);

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
void jfputs(const char* str, long n, JFile& file);

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