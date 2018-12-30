#include "jfio.h"

#include <cassert>
#include "file2.h"

using namespace std;

namespace fs = filesystem;

constexpr int kJournaling = 'J';
constexpr int kJournalReady = 'R';
constexpr int kJournalCleared = 'C';
constexpr int kFlagBytes = 1;
constexpr int kVersionBytes = 4;

namespace jfio {

static inline bool flushJournalFile(JFile& file) {
  fseek2(file.jf, 0, SEEK_SET);
  
  const auto ch = fgetc(file.jf);
  if (ch != kJournalReady) {
    return false;
  }

  bool flushed = false;

  const auto version = fgeti32(file.jf);
  assert(version == 1);

  auto numBlocks = fgeti64(file.jf);
  if (numBlocks > 0) {
    while (numBlocks-- > 0) {
      const auto blockLength = fgeti64(file.jf);
      auto contentLength = blockLength - 16;

      if (contentLength == 0) {
        continue;
      }

      const auto pos = fgeti64(file.jf);
      fseek2(file.f, int64_t(pos), SEEK_SET);

      while (contentLength-- > 0) {
        const auto ch = fgetc(file.jf);
        if (ch == EOF) {
          throw runtime_error("Unexpected EOF while flushing journal content");
        }

        fputc2(ch, file.f);
      }
    }

    fsync2(file.f);
    flushed = true;
  }

  // Mark the journal flush completed
  fseek2(file.jf, 0, SEEK_SET);
  fputc2(kJournalCleared, file.jf);

  file.lastPersistedPos = file.pos;
  file.lastPersistedMaxPos = file.maxPos;
  jfclear(file);

  return flushed;
}

static inline void jfseekEnd(JFile& file, int64_t offset) {
  if (offset > 0) {
    throw runtime_error("Cannot seek past SEEK_END");
  }

  const auto pos = file.maxPos + offset;
  if (pos < 0) {
    throw runtime_error("Cannot seek to before zero");
  }

  file.pos = pos;
}

static inline void jfseekCur(JFile& file, int64_t offset) {
  const auto pos = file.maxPos + offset;
  if (pos < 0) {
    throw runtime_error("Cannot seek to before zero");
  }

  if (pos > file.maxPos) {
    throw runtime_error("Cannot seek past the end of file");
  }

  file.pos = pos;
}

static inline void jfseekSet(JFile& file, int64_t offset) {
  if (offset < 0) {
    throw runtime_error("Cannot seek to before zero");
  }

  if (offset > file.maxPos) {
    throw runtime_error("Cannot seek past the end of file");
  }

  file.pos = offset;
}

static inline void initJournal(JFile& file, bool force = false) {
  if (!force && file.journalEndPos != 0) {
    return;
  }

  int64_t numBytes = 0;

  fseek2(file.jf, 0, SEEK_SET);
  // Flag: 1 byte
  fputc2(kJournaling, file.jf);
  numBytes += 1;
  // Version: 4 bytes
  fputi32(1, file.jf);
  numBytes += 4;
  // Number of completed blocks: 8 bytes
  fputi64(0, file.jf);
  numBytes += 8;

  file.journalEndPos = numBytes;
}

static inline void initBlock(JFile& file) {
  if (file.currentBlockLength != 0) {
    return;
  }

  file.journalBlockStartPos = file.journalEndPos;

  // Block length: 8 bytes
  // Set to zero for now. We will come back to set
  // this length when the block closes.
  fputi64(0, file.jf);
  file.currentBlockLength += 8;
  // Content position: 8 bytes
  fputi64(file.pos, file.jf);
  file.currentBlockLength += 8;

  file.journalEndPos += file.currentBlockLength;
}

static inline void closeBlock(JFile& file) {
  if (file.currentBlockLength < 1) {
    return;
  }

  fseek2(file.jf, file.journalBlockStartPos, SEEK_SET);
  fputi64(file.currentBlockLength, file.jf);

  file.numCompletedBlocks++;
  fseek2(file.jf, kFlagBytes + kVersionBytes, SEEK_SET);
  fputi64(file.numCompletedBlocks, file.jf);

  // Note: don't use SEEK_END here, as the actual journal
  // file size might be larger than journalEndPos
  fseek2(file.jf, file.journalEndPos, SEEK_SET);
  file.currentBlockLength = 0;
}

static inline void incMainPos(JFile& file, int64_t count) {
  file.pos += count;
  if (file.pos > file.maxPos) {
    file.maxPos = file.pos;
  }
}

static inline bool isWriting(const JFile& file) {
  return file.journalEndPos != 0 || file.currentBlockLength != 0;
}

JFile jfopen(
  const fs::path& mainFilePath,
  const fs::path& journalFilePath,
  const string& mainFileModeA,
  const string& mainFileModeB
) {
  JFile file{};
  file.f = fopen2(mainFilePath, mainFileModeA, mainFileModeB);

  try {
    file.jf = fopen2(journalFilePath, "rb+", "wb+");
  } catch (runtime_error&) {
    jfclose(file);
    throw;
  }

  if (flushJournalFile(file)) {
    // We have modified the main file, we want to close and open it again.
    fclose(file.f);
    try {
      file.f = fopen2(mainFilePath, mainFileModeA, mainFileModeB);
    } catch (runtime_error&) {
      jfclose(file);
      throw;
    }    
  }

  file.pos = ftell2(file.f);

  // determine the maximum pos (file length)
  fseek2(file.f, 0, SEEK_END);
  file.maxPos = ftell2(file.f);

  fseek2(file.f, file.pos, SEEK_SET);

  return file;
}

int64_t jfseek(JFile& file, int64_t offset, int origin) {
  if (!isWriting(file)) {
    // read mode
    fseek2(file.f, offset, origin);
    file.pos = ftell2(file.f);
    return file.pos;
  }

  initJournal(file);
  closeBlock(file);

  switch (origin) {
  case SEEK_END:
    jfseekEnd(file, offset);
    break;
  case SEEK_CUR:
    jfseekCur(file, offset);
    break;
  case SEEK_SET:
    jfseekSet(file, offset);
    break;
  default:
    throw runtime_error("jfseek: origin must be either SEEK_SET, SEEK_CUR, or SEEK_END");
    break;
  }

  initBlock(file);

  return file.pos;
}

int64_t jftell(const JFile & file) {
  return file.pos;
}

void jfputc(int ch, JFile& file) {
  initJournal(file);
  initBlock(file);

  fputc2(ch, file.jf);
  file.currentBlockLength++;
  file.journalEndPos++;
  incMainPos(file, 1);
}

void jfputs(const char* str, JFile& file) {
  initJournal(file);
  initBlock(file);

  const auto count = fputs2(str, file.jf);

  file.currentBlockLength += count;
  file.journalEndPos += count;
  incMainPos(file, count);
}

void jfputs(const char* str, int64_t n, JFile& file) {
  initJournal(file);
  initBlock(file);

  fputs2(str, n, file.jf);

  file.currentBlockLength += n;
  file.journalEndPos += n;
  incMainPos(file, n);
}

void jfputi32(int32_t i32, JFile& file)
{
  initJournal(file);
  initBlock(file);

  fputi32(i32, file.jf);

  file.currentBlockLength += 4;
  file.journalEndPos += 4;
  incMainPos(file, 4);
}

void jfputi64(int64_t i64, JFile& file)
{
  initJournal(file);
  initBlock(file);

  fputi64(i64, file.jf);

  file.currentBlockLength += 8;
  file.journalEndPos += 8;
  incMainPos(file, 8);
}

int jfgetc(JFile& file)
{
  if (isWriting(file)) {
    return EOF;
  }

  const int ch = fgetc(file.f);
  file.pos++;

  return ch;
}

int jfgetn(char* s, uint32_t count, JFile& file)
{
  if (isWriting(file)) {
    return EOF;
  }

  int ch = EOF;
  int bytesRead = 0;
  while ((ch = fgetc(file.f)) != EOF) {
    *s = ch;
    bytesRead++;
    s++;
  }

  file.pos += bytesRead;

  return bytesRead;
}

int32_t jfgeti32(JFile& file)
{
  if (isWriting(file)) {
    throw runtime_error("jfgeti32: cannot read during writing mode");
  }

  const auto n = fgeti32(file.f);
  file.pos += 4;
  return n;
}

int64_t jfgeti64(JFile & file)
{
  if (isWriting(file)) {
    throw runtime_error("jfgeti32: cannot read during writing mode");
  }

  const auto n = fgeti64(file.f);
  file.pos += 8;
  return n;
}

void jfflush(JFile& file) {
  if (file.journalEndPos == 0) {
    return;
  }
  
  closeBlock(file);
  fseek2(file.jf, 0, SEEK_SET);
  fputc2(kJournalReady, file.jf);
  fsync2(file.jf);

  flushJournalFile(file);

  // Go back to where the main file was
  fseek2(file.f, file.pos, SEEK_SET);
  jfclear(file);
}

void jfclear(JFile & file) {
  file.numCompletedBlocks = 0;
  file.journalEndPos = 0;
  file.currentBlockLength = 0;
  file.journalBlockStartPos = 0;
  file.pos = file.lastPersistedPos;
  file.maxPos = file.lastPersistedMaxPos;
}

void jfclose(JFile& file) {
  if (file.f) {
    fclose(file.f);
    file.f = nullptr;
  }

  if (file.jf) {
    fclose(file.jf);
    file.jf = nullptr;
  }
}

}