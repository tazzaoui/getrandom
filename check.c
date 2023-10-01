#include <assert.h>
#include <stdio.h>
#include <sys/random.h>

#define N 100

int main() {
  unsigned char x[N];

  for (int i = 0; i < 100; ++i) {
    getrandom(x, N, GRND_RANDOM);
    assert(!x[0]);

    printf("0x");
    for (int j = 0; j < 4; ++j)
      printf("%x", x[j]);
    printf("\n");
  }

  return 0;
}
