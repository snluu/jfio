#pragma once

#include <ios>
#include <cctype>

namespace jfio {

struct JFile {
  std::FILE* f = nullptr;
  std::FILE* jf = nullptr;

  // Current position of the main file.
  int64_t pos = 0;

  // Max position of the main file that we can seek to.
  int64_t maxPos = 0;

  int64_t lastPersistedPos = 0;
  int64_t lastPersistedMaxPos = 0;

  // The ending position of the journal.
  // The actual file might be larger than this,
  // since we don't clear the journal after flushing.
  int64_t journalEndPos = 0;

  // Number of completed blocks
  size_t numCompletedBlocks = 0;

  // Starting position of the current block
  // in the journal file
  int64_t journalBlockStartPos = 0;

  // Length of the current block, start counting
  // from journal block start (i.e. including block header bytes).
  int64_t currentBlockLength = 0;
};
}