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
  return s.d.a + s.d.b + s.c;
}
