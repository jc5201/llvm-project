struct simple {
  int a;
  int b;
};

struct second {
  int c;
  struct simple d;
};

int main() {
  struct second s;
  s.c = 1;
  s.d.a = 2;
  s.d.b = 3;
  return s.d.a + s.d.b + s.c;
}
