#pragma once

#include <ios>
#include <cctype>

namespace jfio {

struct JFile {
  std::FILE* f = nullptr;
  std::FILE* jf = nullptr;

  // Current position of the main file.
  long pos = 0;

  // Max position of the main file that we can seek to.
  long maxPos = 0;

  long lastPersistedPos = 0;
  long lastPersistedMaxPos = 0;

  // The ending position of the journal.
  // The actual file might be larger than this,
  // since we don't clear the journal after flushing.
  long journalEndPos = 0;

  // Number of completed blocks
  int64_t numCompletedBlocks = 0;

  // Starting position of the current block
  // in the journal file
  long journalBlockStartPos = 0;

  // Length of the current block, start counting
  // from journal block start (i.e. including block header bytes).
  long currentBlockLength = 0;
};
}