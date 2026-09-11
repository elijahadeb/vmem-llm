#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " <model.gguf>\n";
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

  uint32_t version;
  if (!f.read(reinterpret_cast<char *>(&version), sizeof(version))) {
    std::cerr << "short read on version\n";
    return 1;
  }
  std::cout << "version: " << version << "         (cursor now " << f.tellg()
            << ")\n";

  uint64_t tensor_count;
  if (!f.read(reinterpret_cast<char *>(&tensor_count), sizeof(tensor_count))) {
    std::cerr << "short read on tensor_count\n";
    return 1;
  }
  std::cout << "tensor_count: " << tensor_count << "    (cursor now "
            << f.tellg() << ")\n";

  uint64_t kv_count;
  if (!f.read(reinterpret_cast<char *>(&kv_count), sizeof(kv_count))) {
    std::cerr << "short read on kv_count\n";
    return 1;
  }
  std::cout << "kv_count: " << kv_count << "        (cursor now " << f.tellg()
            << ")\n";

  return 0;
}
