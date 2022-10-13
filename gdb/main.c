#include <stdlib.h>

static int idx = 0;

typedef struct request_s {
int a;
int b;
int c;
void* pool;

} request_t;

void func(request_t* r)
{
 r->a++;
 if (idx == 123)
   r->pool = 0;
}

void func1(request_t* r)
{
 r->a++;
 r->b++;
 if (idx > 102 && idx < 150) 
 {
   r->a++;
   func(r);
   r->b++;
 }
 r->c++;
}

int main()
{
  for (; idx < 200; idx++)
  {
  request_t* r = malloc(sizeof(request_t));
  r->pool = idx;
  func1(r);
  free(r);
  }
  return 0;
}
