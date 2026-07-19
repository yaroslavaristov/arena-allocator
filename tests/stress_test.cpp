#include "../include/arena.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>

struct Order {
  double price;
  int quantity;
};

// Custom alignment to verify alignas support
struct alignas(32) AlignedStruct {
  char data[32];
};

int main() {
  // Test 1: alignment guarantee and owns()
  ArenaAllocator arena(1u << 20);
  [[maybe_unused]] auto *o = arena.allocate<Order>();
  assert(reinterpret_cast<std::uintptr_t>(o) % alignof(Order) == 0);
  assert(arena.owns(o));

  // Test 2: reset zeroes the used counter
  arena.reset();
  assert(arena.get_used_memory() == 0);

  // Test 3: after reset, first allocation returns the same address
  [[maybe_unused]] auto *first = arena.allocate<Order>();
  arena.reset();
  [[maybe_unused]] auto *second = arena.allocate<Order>();
  assert(first == second);

  // Test 4: 1000 sequential allocations — no overlap, correct alignment
  arena.reset();
  [[maybe_unused]] Order *prev = arena.allocate<Order>();
  for (int i = 0; i < 999; i++) {
    Order *curr = arena.allocate<Order>();
    assert(curr != nullptr);
    assert(arena.owns(curr));
    assert(reinterpret_cast<std::uintptr_t>(curr) % alignof(Order) == 0);
    // Adjacent objects must not overlap
    assert(reinterpret_cast<std::uintptr_t>(curr) >=
           reinterpret_cast<std::uintptr_t>(prev) + sizeof(Order));
    prev = curr;
  }

  // Test 5: custom alignment (alignas(32)) is respected
  arena.reset();
  [[maybe_unused]] auto *aligned = arena.allocate<AlignedStruct>();
  assert(reinterpret_cast<std::uintptr_t>(aligned) % 32 == 0);

  // Test 6: data written to arena is readable and correct
  arena.reset();
  auto *order = arena.allocate<Order>();
  order->price = 100.5;
  order->quantity = 42;
  assert(order->price == 100.5);
  assert(order->quantity == 42);

  // Test 7: overflow returns nullptr — arena must not write out of bounds
  ArenaAllocator small(64);
  [[maybe_unused]] bool got_null = false;
  for (int i = 0; i < 100; i++) {
    auto *p = small.allocate<Order>();
    if (p == nullptr) {
      got_null = true;
      break;
    }
  }
  assert(got_null);

  // Test 8: remaining() decreases after allocation
  arena.reset();
  [[maybe_unused]] std::size_t before = arena.remaining();
  [[maybe_unused]] auto *allocated = arena.allocate<Order>();
  assert(arena.remaining() < before);

  return 0;
}
