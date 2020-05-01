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
  struct simple *t;
  s.c = 1;
  s.d.a = 2;
  s.d.b = 3;
  t = &s.d;
  return t->a;
}
