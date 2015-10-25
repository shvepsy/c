#include <stdio.h>

int x,y;

typedef struct  {
  int sum;
  int mn;
} calc;

// int calc_mn(calc mn)

calc sum(x,y) {
  int s = x+y ;
  return s;
}
calc mn(x,y) {
    int m = x*y ;
    return m;
  }

void main() {
  int a,b;
  printf("%s\n","Please enter digits for calculate: ");
  scanf("%d\n%d", &a, &b );
  int ssum = sum(a,b);
  int mmn = mn(a,b);
  printf("%d %d", ssum, mmn );
}
