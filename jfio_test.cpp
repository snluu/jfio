#include "jfio/jfio.h"
#include <cstdio>

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

void readWholeFile(char* c, FILE* f) {
  const auto x = c;
  int ch = 0;
  fseek(f, 0, SEEK_SET);
  while ((ch = fgetc(f)) != EOF) {
    *c = ch;
    c++;
  }

  fputs(x, stdout);
  fputs("\r\n", stdout);
}

void testSimpleWrite() {
  auto file = createTestFile();
  jfputc('H', file);
  jfputs("ello", file);
  jfflush(file);

  fseek(file.f, 0, SEEK_END);
  check(ftell(file.f) == 5, "File size");

  std::string s(6, '\0');
  readWholeFile(s.data(), file.f);
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
  readWholeFile(s.data(), file.f);
  check(strcmp(s.c_str(), "Roses are red.\r\nViolets are blue!.") == 0, "File content mismatch");

  jfclose(file);
}

int main() {
  testSimpleWrite();
  testWrite();
}