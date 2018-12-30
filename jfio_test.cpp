#include <cstdio>

#include "jfio/jfio.h"
#include "jfio/file2.h"

namespace fs = std::filesystem;

using namespace std;
using namespace jfio;

void check(bool x, const char* msg) {
  if (!x) {
    fputs(msg, stderr);
    throw msg;
  }
}

JFile createTestFile() {
  std::string filePath(1024, '\0');
  tmpnam_s(filePath.data(), filePath.length());
  std::string journalPath(1024, '\0');
  tmpnam_s(journalPath.data(), journalPath.length());

  return jfopen(
    filesystem::path(filePath),
    filesystem::path(journalPath),
    "rb+",
    "wb+"
  );
}

void testSimpleWrite() {
  auto file = createTestFile();
  jfputc('H', file);
  jfputs("ello", file);

  check(jfgetc(file) == EOF, "jfgetc() != EOF");
  jfflush(file);

  check(jftell(file) == 5, "jftell() != 5");
  jfseek(file, 0, SEEK_SET);

  std::string s(6, '\0');
  check(jfgetn(s.data(), 5, file) == 5, "Num chars read mismatch");
  check(strcmp(s.c_str(), "Hello") == 0, "File content mismatch");

  jfclose(file);
}

void testWrite() {
  auto file = createTestFile();
  jfputs("roses are red.\r\n", file);
  jfseek(file, 0, SEEK_SET);
  jfputc('R', file);
  jfseek(file, 0, SEEK_END);

  jfflush(file);

  jfputs("violets are green", file);
  jfclear(file);

  jfputs("Violets!!", 7, file);
  jfputs("are_blue...", file);
  jfseek(file, -11, SEEK_CUR);
  jfputs(" are blue!", file);

  jfflush(file);

  std::string s(1024, '\0');

  jfseek(file, 0, SEEK_SET);
  check('R' == jfgetc(file), "jfgetc() != R");
  check(jfgetn(s.data(), 1000, file) < 1000, "Bytes read check failed");
  check(strcmp(s.c_str(), "oses are red.\r\nViolets are blue!.") == 0, "File content mismatch");

  jfclose(file);
}

void testNumbers() {
  auto file = createTestFile();

  const int32_t i32 = 12345;
  const int64_t i64 = -999888777666L;

  jfputi32(i32, file);
  jfputi64(i64, file);
  jfflush(file);

  check(jfseek(file, 0, SEEK_END) == 12, "fseek() return 12");
  check(jftell(file) == 12, "File size");

  check(jfseek(file, -12, SEEK_CUR) == 0, "fseek() return 0");
  check(i32 == jfgeti32(file), "i32 mismatch");
  check(i64 == jfgeti64(file), "i64 mismatch");

  jfclose(file);
}

int main() {
  testSimpleWrite();
  testWrite();
  testNumbers();
}