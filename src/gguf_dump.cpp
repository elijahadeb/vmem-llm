#include <cstdint>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

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

struct GGUFValue;

using GGUFArray = std::vector<GGUFValue>;

struct GGUFValue {
  std::variant<uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, float,
               bool, std::string, uint64_t, int64_t, double, GGUFArray>
      data;
};

struct GGUFTensorInfo {
  std::string name;
  std::vector<uint64_t> dimensions;
  uint32_t type;
  uint64_t offset;
};

class GGUFParser {
public:
  struct Header {
    char magic[4];
    uint32_t version;
    uint64_t tensor_count;
    uint64_t metadata_kv_count;
  } header;

  std::map<std::string, GGUFValue> metadata;
  std::vector<GGUFTensorInfo> tensors;

  explicit GGUFParser(const std::string &filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
      throw std::runtime_error("failed to open file: " + filepath);
    }
  }
};

int main() { return 0; }
