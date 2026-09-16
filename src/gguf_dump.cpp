#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

using u32 = std::uint32_t;
using u64 = std::uint64_t;

u32 read_u32(std::ifstream &f, const char *what) {
  u32 value;

  if (!f.read(reinterpret_cast<char *>(&value), sizeof(value))) {
    throw std::runtime_error(std::string("failed to read ") + what);
  }

  return value;
}

u64 read_u64(std::ifstream &f, const char *what) {
  u64 value;

  if (!f.read(reinterpret_cast<char *>(&value), sizeof(value))) {
    throw std::runtime_error(std::string("failed to read ") + what);
  }

  return value;
}

std::string read_string(std::ifstream &f) {
  u64 len;

  if (!f.read(reinterpret_cast<char *>(&len), sizeof(len))) {
    throw std::runtime_error(
        "failed to read the prefix of the string length. reached eof early.");
  }

  // safety boundary

  const u64 MAX_SAFE_LENGTH = 50 * 1024 * 1024;

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

enum : u32 {
  GGUF_TYPE_UINT8 = 0,
  GGUF_TYPE_INT8 = 1,
  GGUF_TYPE_UINT16 = 2,
  GGUF_TYPE_INT16 = 3,
  GGUF_TYPE_UINT32 = 4,
  GGUF_TYPE_INT32 = 5,
  GGUF_TYPE_FLOAT32 = 6,
  GGUF_TYPE_BOOL = 7,
  GGUF_TYPE_STRING = 8,
  GGUF_TYPE_ARRAY = 9,
  GGUF_TYPE_UINT64 = 10,
  GGUF_TYPE_INT64 = 11,
  GGUF_TYPE_FLOAT64 = 12,
};

void skip_metadata_value(std::ifstream &f, u32 type) {
  switch (type) {
  case GGUF_TYPE_UINT8:
  case GGUF_TYPE_INT8:
  case GGUF_TYPE_BOOL: {
    char byte;
    if (!f.read(&byte, 1)) {
      throw std::runtime_error("failed to skip 1-byte metadata value");
    }
    break;
  }

  case GGUF_TYPE_UINT16:
  case GGUF_TYPE_INT16: {
    std::uint16_t value;
    if (!f.read(reinterpret_cast<char *>(&value), sizeof(value))) {
      throw std::runtime_error("failed to skip 16-bit metadata value");
    }
    break;
  }

  case GGUF_TYPE_UINT32:
  case GGUF_TYPE_INT32:
  case GGUF_TYPE_FLOAT32: {
    u32 value;
    if (!f.read(reinterpret_cast<char *>(&value), sizeof(value))) {
      throw std::runtime_error("failed to skip 32-bit metadata value");
    }
    break;
  }

  case GGUF_TYPE_UINT64:
  case GGUF_TYPE_INT64:
  case GGUF_TYPE_FLOAT64: {
    u64 value;
    if (!f.read(reinterpret_cast<char *>(&value), sizeof(value))) {
      throw std::runtime_error("failed to skip 64-bit metadata value");
    }
    break;
  }

  case GGUF_TYPE_STRING: {
    // read_string consumes the u64 length + string bytes.
    (void)read_string(f);
    break;
  }

  case GGUF_TYPE_ARRAY: {
    u32 element_type = read_u32(f, "metadata array element type");
    u64 element_count = read_u64(f, "metadata array element count");

    for (u64 i = 0; i < element_count; ++i) {
      skip_metadata_value(f, element_type);
    }

    break;
  }

  default:
    throw std::runtime_error("unknown GGUF metadata value type: " +
                             std::to_string(type));
  }
}

u64 checked_add(u64 a, u64 b) {
  if (a > std::numeric_limits<u64>::max() - b) {
    throw std::runtime_error("u64 addition overflow");
  }

  return a + b;
}

u64 checked_multiply(u64 a, u64 b) {
  if (a != 0 && b > std::numeric_limits<u64>::max() / a) {
    throw std::runtime_error("u64 multiplication overflow");
  }

  return a * b;
}

struct BlockSpec {
  u64 elements_per_block;
  u64 bytes_per_block;
};

const std::unordered_map<u32, BlockSpec> TYPE_BLOCKS = {
    {0, {1, 4}},      // F32
    {12, {256, 144}}, // Q4_K
    {14, {256, 210}}, // Q6_K
};

u64 align_up(u64 position, u64 alignment) {
  if (alignment == 0) {
    throw std::runtime_error("alignment cannot be zero");
  }

  u64 remainder = position % alignment;

  if (remainder == 0) {
    return position;
  }

  return checked_add(position, alignment - remainder);
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
  try {
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
    if (!f.read(reinterpret_cast<char *>(&tensor_count),
                sizeof(tensor_count))) {
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

    u32 alignment = 32; // GGUF default

    // std::cout << "\n--- METADATA ---\n";

    for (u64 i = 0; i < kv_count; ++i) {
      std::string key = read_string(f);

      u32 value_type = read_u32(f, "metadata value type");

      std::cout << "metadata[" << i << "] key=\"" << key
                << "\" type=" << value_type << '\n';

      if (key == "general.alignment" && value_type == GGUF_TYPE_UINT32) {
        alignment = read_u32(f, "general.alignment");

        std::cout << "  alignment = " << alignment << '\n';
      } else {
        skip_metadata_value(f, value_type);
      }

      std::cout << "  cursor now " << f.tellg() << '\n';
    }

    std::cout << "\nmetadata finished at cursor " << f.tellg() << "\n";

    // std::cout << "\n--- TENSOR INFO TABLE ---\n";

    u64 total_tensor_bytes = 0;

    for (u64 i = 0; i < tensor_count; ++i) {

      std::string name = read_string(f);

      u32 n_dims = read_u32(f, "tensor n_dims");

      u64 element_count = 1;

      std::cout << "tensor[" << i << "] "
                << "name=\"" << name << "\" "
                << "dims=[";

      for (u32 d = 0; d < n_dims; ++d) {
        u64 dim = read_u64(f, "tensor dimension");

        if (d > 0) {
          std::cout << ", ";
        }

        std::cout << dim;

        element_count = checked_multiply(element_count, dim);
      }

      std::cout << "] ";
      u32 ggml_type = read_u32(f, "tensor ggml type");
      u64 offset = read_u64(f, "tensor offset");

      std::streampos cursor = f.tellg();

      std::cout << "type=" << ggml_type << " offset=" << offset
                << " elements=" << element_count << " cursor=" << cursor
                << '\n';

      auto it = TYPE_BLOCKS.find(ggml_type);

      if (it == TYPE_BLOCKS.end()) {
        throw std::runtime_error(
            "unsupported ggml type " + std::to_string(ggml_type) +
            " while calculating tensor bytes for: " + name);
      }

      const BlockSpec &spec = it->second;

      if (element_count % spec.elements_per_block != 0) {
        throw std::runtime_error(
            "tensor element count is not divisible by block size for: " + name);
      }

      u64 block_count = element_count / spec.elements_per_block;

      u64 tensor_bytes = checked_multiply(block_count, spec.bytes_per_block);

      total_tensor_bytes = checked_add(total_tensor_bytes, tensor_bytes);

      std::cout << "    blocks=" << block_count << " bytes=" << tensor_bytes
                << '\n';
    }

    u64 tensor_info_end = static_cast<u64>(f.tellg());

    u64 data_offset = align_up(tensor_info_end, alignment);

    // std::cout << "\n--- SUMMARY ---\n";
    std::cout << "alignment: " << alignment << '\n';
    std::cout << "tensor info end: " << tensor_info_end << '\n';
    std::cout << "data offset: " << data_offset << '\n';
    std::cout << "total tensor bytes: " << total_tensor_bytes << '\n';

    return 0;

  } catch (const std::exception &e) {
    std::cerr << "\n[fatal] " << e.what() << '\n';
    return 1;
  }
}
