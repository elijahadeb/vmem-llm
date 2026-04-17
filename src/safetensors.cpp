// step 1: open the .safetensors file with open()

#include <cstddef>
#include <cstdint>
#include <fcntl.h> //posix for open()
#include <filesystem>
#include <iostream>
#include <print>
#include <safetensors.h>
#include <span>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

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

  uint64_t json_size = 0;

  for (int i = 0; i < 8; i++) {
    json_size |= static_cast<uint64_t>(data_ptr[i]) << (i * 8);
  }

  std::print("\njson metadate size = {}", json_size);

  // to read the json metadata we'd need to start from byte offset 8
  // we'd use the object -> std::span - so that the cpu is aware out of the box
  // - the boundary of what I expect. it packages the memory address with a
  // boundary. instead of passing a blind pointer and a separate number and
  // hoping the CPU math stays correct. it's called memory bookkeeping.

  std::span<const uint8_t> raw_json_data(data_ptr + 8, json_size);
  std::string_view json_metadata(
      reinterpret_cast<const char *>(raw_json_data.data()),
      raw_json_data.size());

  std::print("\njson metadata:\n{}\n", json_metadata);

  close(fd);
  return 0;
}

// TODO: step 3: parse the json metadata

// TODO: step 4: mmap the weight data section

// TODO: step 5: use offset from the metadata to find each tensor
