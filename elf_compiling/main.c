extern int gvar;
int foo(int, int);
int main()
{
  return foo(gvar + 100, 200);
}
