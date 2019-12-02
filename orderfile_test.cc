#include <random>

#include "orderfile_instrumentation.h"

static int v;

__attribute__((noinline)) void foo() {
  v++;
}

__attribute__((noinline)) void bar() {
  v--;
}

int main() {
  std::mt19937_64 random_generator(0);
  while ((random_generator() & 0xffff) != 0xffff) {
    if ((random_generator() & 1) == 1) {
      foo();
    } else {
      bar();
    }
  }
  printf("%d\n", v);
  orderfile::Dump("foo");
}
