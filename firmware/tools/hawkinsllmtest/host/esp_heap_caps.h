// Port de test hôte repris de REZOR/cardputer-ai (MIT).
// Les capacités matérielles sont ramenées à malloc/posix_memalign/free.
#pragma once
#include <cstdlib>
#define MALLOC_CAP_INTERNAL 0
#define MALLOC_CAP_8BIT 0
inline void* heap_caps_malloc(size_t n, int) { return malloc(n); }
inline void* heap_caps_aligned_alloc(size_t align, size_t n, int) {
  void* p = nullptr;
  if (posix_memalign(&p, align, n) != 0) return nullptr;
  return p;
}
inline void heap_caps_free(void* p) { free(p); }
