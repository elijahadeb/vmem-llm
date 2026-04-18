// step 1: open the .safetensors file with open()

#include <cstddef>
#include <cstdint>
#include <fcntl.h> //posix for open()
#include <filesystem>
#include <iostream>
#include <print>
#include <safetensors.h>
// #include <span>
#include "../include/nlohmann/json.hpp"
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>

namespace fs = std::filesystem;

int init_tensor_loader() {
  fs::path file_path = "../models/model.safetensors";
  int fd = open(file_path.c_str(), O_RDONLY);

  if (fd == -1) {
    std::print(stderr, "failed to open file");
    return 1;
  }

  // step 2: read the header - the first 8 bytes reveals the header size

  struct stat file_info;

  if (fstat(fd, &file_info) == -1) {
    std::print(stderr, "failed to fetch file info");
    close(fd);
    return 1;
  }

  size_t file_size = file_info.st_size;

  std::print("file size = {} bytes. which translates to ({:.2f} MB)\n",
             file_size, static_cast<double>(file_size) / (1024 * 1024));

  // to read the header - we'd first have to make a direct system call using
  // mmap

  void *mapped = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);

  if (mapped == MAP_FAILED) {
    std::print(stderr, "mmap failed");
    close(fd);
    return 1;
  }

  // demand paging...

  std::print("mmap succeeded. click enter to trigger page fault.");
  std::cin.get();

  // once the page fault occur... we'd hv to read the first few bytes -
  // per byte. char is 1byte let's use that. and also make use of static
  // casting.

  unsigned char *data_ptr = static_cast<unsigned char *>(mapped);

  if (file_size < 8) {
    std::print("\nfile is too small to have a header");
    munmap(mapped, file_size);
    close(fd);
    return 1;
  }

  uint64_t json_size = 0;

  for (int i = 0; i < 8; i++) {
    json_size |=
        static_cast<uint64_t>(data_ptr[i])
        << (i * 8); // i preformed a bitwise operation here. to shift per 8bits.
  }

  if (file_size < 8 + json_size) {
    std::print("\nfile is corrupted: json size exceeds file bounds");
    close(fd);
    munmap(mapped, file_size);
    return 1;
  }

  std::print("\njson metadate size = {}\n", json_size);

  // to read the json metadata we'd need to start from byte offset 8

  std::string_view json_metadata(reinterpret_cast<const char *>(data_ptr + 8),
                                 json_size);

  std::print("\njson metadata:\n{}\n", json_metadata);

  // TODO: step 3: parse the json metadata

  // type alias - nlohmann is the namespace while json is the class - yes a
  // type - hence - type alias.
  using json = nlohmann::json;
  json parsed = json::parse(json_metadata);
  std::print("\nparsed json:\n{}\n", parsed.dump(1, '\t'));

  //"model.layers.9.self_attn.q_proj.weight": {
  //  "data_offsets": [2190655488, 2199044096],
  //  "dtype": "BF16",
  //  "shape": [2048, 2048]

  struct TensorInfo {
    std::string dtype;
    uint64_t offset_start;
    uint64_t offset_end;
    std::vector<uint64_t> shape;
  };

  std::unordered_map<std::string, TensorInfo> tensors; // a hash map.

  for (auto &[name, info] : parsed.items()) {

    if (name == "__metadata__")
      continue;

    TensorInfo t;
    t.dtype = info["dtype"]; // operator overloading.
    t.shape =
        info["shape"].get<std::vector<uint64_t>>(); // explicit type conversion
    t.offset_start = info["data_offsets"][0];
    t.offset_end = info["data_offsets"][1];

    tensors[name] = t; // hash map insertion
  }

  std::print("\nloaded {} tensors", tensors.size());

  munmap(mapped, file_size);
  close(fd);
  return 0;
}

// TODO: step 4: mmap the weight data section

// TODO: step 5: use offset from the metadata to find each tensor
