#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

using u32 = std::uint32_t;
using u64 = std::uint64_t;

std::string read_string(std::ifstream &f) {
  uint64_t len;

  if (!f.read(reinterpret_cast<char *>(&len), sizeof(len))) {
    throw std::runtime_error(
        "failed to read the prefix of the string length. reached eof early.");
  }

  // safety boundary

  const uint64_t MAX_SAFE_LENGTH = 50 * 1024 * 1024;

  if (len > MAX_SAFE_LENGTH) {
    std::streampos cursor = f.tellg();

    std::cerr << "\n[fatal error] read_string sanity check failed!\n"
              << "the read length is: " << len << " bytes.\n"
              << "file cursor misaligned at byte offset: " << cursor << " ("
              << std::hex << cursor << std::dec << ")\n"
              << "aborting to prevent allocation hang.\n\n";

    std::abort();
  }
  // string buffer
  std::string s(len, '\0');

  if (len > 0) {
    if (!f.read(&s[0], len)) {
      throw std::runtime_error(
          "failed to read string data characters. file truncated.");
    }
  }

  return s;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "this is what's expected: " << argv[0] << " <model.gguf>\n";
    return 1;
  }

  std::ifstream f(argv[1], std::ios::binary);
  if (!f) {
    std::cerr << "fopen error\n";
    return 1;
  }

  char magic[4];
  if (!f.read(magic, 4)) {
    std::cerr << "short read on magic\n";
    return 1;
  }
  if (std::memcmp(magic, "GGUF", 4) != 0) {
    std::cerr << "not a gguf file\n";
    return 1;
  }
  std::cout << "magic:  " << std::string(magic, 4) << "        (cursor now "
            << f.tellg() << ")\n";

  u32 version;
  if (!f.read(reinterpret_cast<char *>(&version), sizeof(version))) {
    std::cerr << "short read on version\n";
    return 1;
  }
  std::cout << "version: " << version << "         (cursor now " << f.tellg()
            << ")\n";

  u64 tensor_count;
  if (!f.read(reinterpret_cast<char *>(&tensor_count), sizeof(tensor_count))) {
    std::cerr << "short read on tensor_count\n";
    return 1;
  }
  std::cout << "tensor_count: " << tensor_count << "    (cursor now "
            << f.tellg() << ")\n";

  u64 kv_count;
  if (!f.read(reinterpret_cast<char *>(&kv_count), sizeof(kv_count))) {
    std::cerr << "short read on kv_count\n";
    return 1;
  }
  std::cout << "kv_count: " << kv_count << "        (cursor now " << f.tellg()
            << ")\n";

  return 0;
}
