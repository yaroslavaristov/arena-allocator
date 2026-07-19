#include "../include/arena.hpp"
#include <benchmark/benchmark.h>
#include <cstdlib>
#include <fcntl.h>
#include <vector>

struct Small {
  char data[8];
};
struct Medium {
  char data[64];
};
struct Large {
  char data[256];
};

// Sequential allocation benchmark: arena vs malloc, N objects of Medium size
static void BM_Arena_Sequential(benchmark::State &state) {
  const std::size_t N = state.range(0);
  ArenaAllocator arena((1u << 20) * 64);
  for (auto _ : state) {
    for (std::size_t i = 0; i < N; i++) {
      auto *p = arena.allocate<Medium>();
      benchmark::DoNotOptimize(p);
    }
    arena.reset();
  }
  state.SetItemsProcessed(state.iterations() * N);
}

// Sequential malloc benchmark for comparison
static void BM_Malloc_Sequential(benchmark::State &state) {
  const std::size_t N = state.range(0);
  std::vector<void *> ptrs(N);
  for (auto _ : state) {
    for (std::size_t i = 0; i < N; i++) {
      ptrs[i] = malloc(sizeof(Medium));
      benchmark::DoNotOptimize(ptrs[i]);
    }
    for (std::size_t i = 0; i < N; i++)
      free(ptrs[i]);
  }
  state.SetItemsProcessed(state.iterations() * N);
}

// Mixed allocation: Small + Medium + Large in one batch
static void BM_Arena_Mixed(benchmark::State &state) {
  ArenaAllocator arena((1u << 20) * 64);
  for (auto _ : state) {
    auto *s = arena.allocate<Small>();
    auto *m = arena.allocate<Medium>();
    auto *l = arena.allocate<Large>();
    benchmark::DoNotOptimize(s);
    benchmark::DoNotOptimize(m);
    benchmark::DoNotOptimize(l);
    arena.reset();
  }
}

// Mixed malloc benchmark for comparison
static void BM_Malloc_Mixed(benchmark::State &state) {
  for (auto _ : state) {
    auto *s = malloc(sizeof(Small));
    auto *m = malloc(sizeof(Medium));
    auto *l = malloc(sizeof(Large));
    benchmark::DoNotOptimize(s);
    benchmark::DoNotOptimize(m);
    benchmark::DoNotOptimize(l);
    free(s);
    free(m);
    free(l);
  }
}

BENCHMARK(BM_Arena_Sequential)->Range(1, 1024);
BENCHMARK(BM_Malloc_Sequential)->Range(1, 1024);
BENCHMARK(BM_Arena_Mixed);
BENCHMARK(BM_Malloc_Mixed);

int main(int argc, char **argv) {
  int dev_null = open("/dev/null", O_WRONLY);
  dup2(dev_null, STDERR_FILENO);
  close(dev_null);
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
  return 0;
}
