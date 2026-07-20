#include "../include/arena.hpp"

#include <print>

struct Order {
    double price;
    int    quantity;
};

int main() {
    // Allocate 1MB arena upfront via mmap — no heap involvement
    ArenaAllocator arena(1u << 20);

    // O(1) bump pointer allocation — no malloc, no fragmentation
    auto* order     = arena.allocate<Order>();
    order->price    = 100.5;
    order->quantity = 42;

    std::print("Price: {}, Quantity: {}\n", order->price, order->quantity);
    std::print("Used: {} bytes\n", arena.get_used_memory());
    std::print("Remaining: {} bytes\n", arena.remaining());

    return 0;
}
