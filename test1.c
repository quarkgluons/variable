int i = 10;

int k;
int j = i;
// i = k
void foo(int i) {
  int a_foo = i;
  i = j;
}
