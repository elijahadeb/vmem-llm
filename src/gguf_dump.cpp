#include <cstdint>
// #include <iostream>

// header parser

struct gguf_header_t {
  char magic[4];
  uint32_t version;
  uint64_t tensor_count;
  uint64_t metadata_kv_count;
};

enum class GGUFType : uint32_t {
  UINT8 = 0,
  INT8 = 1,
  UINT16 = 2,
  INT16 = 3,
  UINT32 = 4,
  INT32 = 5,
  FLOAT32 = 6,
  BOOL = 7,
  STRING = 8,
  ARRAY = 9,
  UINT64 = 10,
  INT64 = 11,
};

int main() { return 0; }
