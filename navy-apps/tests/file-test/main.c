#include <stdio.h>
#include <assert.h>
#include <unistd.h>

static void mark(const char *s) {
  write(1, s, 2);   // 例如 "A\n"
}

int main() {
  mark("A\n");
  FILE *fp = fopen("/share/files/num", "r+");
  mark(fp ? "B\n" : "b\n");
  assert(fp);

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  mark("C\n");
  assert(size == 5000);

  fseek(fp, 500 * 5, SEEK_SET);
  mark("D\n");

  int i, n;
  for (i = 500; i < 1000; i ++) {
    fscanf(fp, "%d", &n);
    if (n != i + 1) {
      mark("E\n");
    }
    assert(n == i + 1);
  }

  fseek(fp, 0, SEEK_SET);
  mark("F\n");

  for (i = 0; i < 500; i ++) {
    fprintf(fp, "%4d\n", i + 1 + 1000);
  }

  mark("G\n");

  for (i = 500; i < 1000; i ++) {
    fscanf(fp, "%d", &n);
    if (n != i + 1) {
      mark("H\n");
    }
    assert(n == i + 1);
  }

  fseek(fp, 0, SEEK_SET);
  mark("I\n");

  for (i = 0; i < 500; i ++) {
    fscanf(fp, "%d", &n);
    if (n != i + 1 + 1000) {
      mark("J\n");
    }
    assert(n == i + 1 + 1000);
  }

  fclose(fp);
  mark("K\n");
  return 0;
}
