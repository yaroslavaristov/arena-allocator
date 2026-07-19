#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <print>
#include <stdexcept>
#include <sys/mman.h>
#include <utility>

class ArenaAllocator {
public:
  explicit ArenaAllocator(std::size_t capacity) : capacity_{capacity} {
    if (capacity_ <= 0 || (capacity_ & (capacity_ - 1)) != 0) {
      throw std::invalid_argument("Capacity must be greater than 0 and must be "
                                  "a power of two! Passed: " +
                                  std::to_string(capacity));
    }

    void *mmap_result{mmap(nullptr, capacity_, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANON | MAP_HUGETLB | MAP_POPULATE,
                           -1, 0)};

    if (mmap_result == MAP_FAILED && (errno == EINVAL || errno == ENOMEM)) {
      std::print(stderr,
                 "Warning: HugePages are not available (errno: {}). Failing "
                 "back to standard 4KB pages.\n",
                 errno);

      mmap_result = mmap(nullptr, capacity_, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANON | MAP_POPULATE, -1, 0);
    }

    if (mmap_result == MAP_FAILED) {
      int error_code{errno};
      std::print(stderr,
                 "Fatal: Troubles with allocating memory: {}\nERRNO: {}\n",
                 std::strerror(error_code), error_code);
      throw std::bad_alloc();
    }

    mem_start_ = reinterpret_cast<std::byte *>(mmap_result);
    curr_addr_ = mem_start_;

    if (mlock2(mem_start_, capacity_, MCL_CURRENT) != 0) {
      int error_code{errno};
      std::print(stderr,
                 "Warning: Failed to lock memory with mlock2: {}\nERRNO: {}",
                 std::strerror(error_code), error_code);
    }
  }

  ~ArenaAllocator() noexcept {
    if (mem_start_) {
      munlock(mem_start_, capacity_);
      if (munmap(mem_start_, capacity_) != 0) {
        int error_code{errno};
        std::print(stderr,
                   "Fatal: munmap failed during destruction: {}\nERRNO: {}\n",
                   std::strerror(error_code), error_code);
      }
    }
  }

  ArenaAllocator(const ArenaAllocator &) = delete;
  ArenaAllocator &operator=(const ArenaAllocator &) = delete;

  friend void swap(ArenaAllocator &src, ArenaAllocator &dest) noexcept {
    std::swap(src.mem_start_, dest.mem_start_);
    std::swap(src.curr_addr_, dest.curr_addr_);
    std::swap(src.capacity_, dest.capacity_);
  }

  ArenaAllocator(ArenaAllocator &&other) noexcept : ArenaAllocator() {
    swap(*this, other);
  }

  ArenaAllocator &operator=(ArenaAllocator &&other) noexcept {
    ArenaAllocator temp{std::move(other)};
    swap(*this, temp);
    return *this;
  }

  template <typename T, std::size_t Alignment = alignof(T)>
  T *allocate(std::size_t alloc_obj) noexcept {
    static_assert(((Alignment & (Alignment - 1)) == 0),
                  "Aligment must be a power of two!");

    std::size_t alloc_size{sizeof(T) * alloc_obj};

    std::uintptr_t raw_addr{reinterpret_cast<std::uintptr_t>(curr_addr_)};
    std::uintptr_t aligned_addr{(raw_addr + Alignment - 1) & ~(Alignment - 1)};
    std::uintptr_t next_addr{aligned_addr + alloc_size};

    std::uintptr_t start_addr{reinterpret_cast<std::uintptr_t>(mem_start_)};

    if (next_addr < aligned_addr || next_addr > start_addr + capacity_)
      return nullptr;

    curr_addr_ = reinterpret_cast<std::byte *>(next_addr);
    return reinterpret_cast<T *>(aligned_addr);
  }

  void reset() noexcept { curr_addr_ = mem_start_; }

  std::size_t get_used_memory() const noexcept {
    return static_cast<std::size_t>(curr_addr_ - mem_start_);
  }

  std::size_t remaining() const noexcept {
    return capacity_ - get_used_memory();
  }

  template <typename T> bool owns(const T *ptr) {
    auto p{reinterpret_cast<std::byte *>(ptr)};
    return p >= mem_start_ && p < mem_start_ + capacity_;
  }

private:
  std::size_t capacity_{0};

  std::byte *mem_start_{nullptr};
  std::byte *curr_addr_{nullptr};

  ArenaAllocator() = default;
};
