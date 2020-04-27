struct simple {
  int a;
  int b;
};

int main() {
  struct simple s;
  return &s == 0 ;
}
