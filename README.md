# ArenaAllocator

A high-performance bump-pointer arena allocator for zero-allocation critical paths in low-latency systems.

Allocates a large memory region upfront via `mmap` (with HugePage support) and serves allocations by bumping a pointer. No heap fragmentation, no lock contention, no dynamic memory overhead in the hot path.

---

## Features

- **O(1) allocation** — single pointer increment, no heap involvement
- **HugePage support** — `MAP_HUGETLB` with automatic fallback to 4KB pages
- **Memory locking** — `mlock` prevents pages from being swapped out
- **Custom alignment** — compile-time alignment via template parameter
- **Overflow protection** — returns `nullptr` on exhaustion, no UB
- **Move semantics** — movable, non-copyable (owns a unique mmap region)
- **C++23** — `std::print`, compiler hints for branch prediction and inlining

---

## Benchmark Results

Tested on two platforms: **AMD EPYC 7763** (GitHub Codespaces, x86_64) and **Apple M1** (aarch64, OrbStack virtualization — not native hardware).

### Sequential Allocation — Arena vs `malloc` (AMD EPYC 7763)

| N objects | Arena (ns) | malloc (ns) | Speedup |
|-----------|-----------|-------------|---------|
| 1         | 1.28      | 12.5        | **10x** |
| 8         | 6.12      | 87.6        | **14x** |
| 64        | 43.6      | 1096        | **25x** |
| 512       | 337       | 8435        | **25x** |
| 1024      | 661       | 17594       | **27x** |

> Release build, `-O3 -march=native -funroll-loops -Wall -Wextra -Werror -DNDEBUG`

### Mixed Allocation — Small + Medium + Large in one batch (AMD EPYC 7763)

| Allocator | Time (ns) | Speedup |
|-----------|-----------|---------|
| Arena     | 1.97      | —       |
| malloc    | 29.9      | **15x** |

### Apple M1 (aarch64, OrbStack virtualization)

| N objects | Arena (ns) | malloc (ns) | Speedup |
|-----------|-----------|-------------|---------|
| 1         | 1.25      | 39.6        | **32x** |
| 8         | 6.29      | 687         | **109x** |
| 64        | 47.9      | 4651        | **97x** |
| 512       | 330       | 45226       | **137x** |
| 1024      | 652       | 222747      | **342x** |

### Mixed Allocation — Apple M1

| Allocator | Time (ns) | Speedup |
|-----------|-----------|---------|
| Arena     | 1.57      | —       |
| malloc    | 23.0      | **15x** |

> malloc degrades significantly on large N due to heap fragmentation and internal locking. Arena stays flat — it is just a pointer increment.

---

## Usage

```cpp
#include "arena.hpp"

struct Order {
    double price;
    int quantity;
};

int main() {
    // Allocate 1MB arena upfront — no heap involvement after this point
    ArenaAllocator arena(1u << 20);

    // O(1) bump pointer allocation
    auto* order = arena.allocate<Order>();
    order->price = 100.5;
    order->quantity = 42;

    // Allocate array of 100 orders
    auto* orders = arena.allocate<Order>(100);

    // Custom alignment
    auto* aligned = arena.allocate<Order, 64>(); // cache-line aligned

    // Check ownership
    assert(arena.owns(order));

    // Reset — all pointers invalidated, memory reused
    arena.reset();

    return 0;
}
```

---

## API

```cpp
// Construction — capacity must be a power of two
explicit ArenaAllocator(std::size_t capacity);

// Allocation — returns nullptr on overflow
template <typename T, std::size_t Alignment = alignof(T)>
T* allocate(std::size_t count = 1) noexcept;

// Reset — O(1), invalidates all previously allocated pointers
void reset() noexcept;

// Diagnostics
std::size_t get_used_memory() const noexcept;
std::size_t remaining() const noexcept;

// Ownership check
bool owns(const T* ptr) const noexcept;
```

---

## Design Decisions

**Why `mmap` instead of `new`?**
`mmap` gives direct control over memory mapping flags. `MAP_HUGETLB` reduces TLB pressure on large arenas. `MAP_POPULATE` pre-faults pages at construction time so there are no page faults in the hot path.

**Why `mlock`?**
Prevents the OS from swapping arena pages to disk. In latency-sensitive systems, a page fault at the wrong moment causes microsecond-level spikes.

**Why power-of-two capacity?**
Enables future optimizations with bitwise masking. Validates at construction time via `(capacity & (capacity - 1)) == 0`.

**Why no `free`?**
Arena allocators trade individual deallocation for allocation speed. Objects are freed in bulk via `reset()`. This matches the HFT usage pattern: allocate for one market event, process, reset, repeat.

**Why compiler hints on overflow branch?**
In a correctly sized arena, overflow never happens in the hot path. Marking it `[[unlikely]]` tells the branch predictor to optimize for the fast path.

---

## Build

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

**Run benchmarks:**
```bash
./bench
```

**Run stress tests:**
```bash
./stress_test
```

**Requirements:** C++23, GCC 14+ or Clang 17+, Linux (for `mmap`/`mlock`)

---

## Project Structure

```
arena-allocator/
├── include/
│   └── arena.hpp          # Single-header implementation
├── src/
│   └── main.cpp           # Usage example
├── tests/
│   └── stress_test.cpp    # Correctness tests
├── benchmarks/
│   └── bench.cpp          # Google Benchmark suite
├── Doxyfile               # Doxygen configuration
├── LICENSE
└── CMakeLists.txt
```
