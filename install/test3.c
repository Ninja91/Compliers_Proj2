struct Temp{
  int a;
  float b;
  char c[10][20];
};

struct Temp myTemp[10];

void foo1()
{
  int a[10][20][10];
  int p;
  int q;
  int k;
  k=10;
  printf("%d\n", a[p][q][k+3-5]);
}

void foo0(int a, float b)
{
  int i;
  i = 0;
  while(i<10){ 
    myTemp[i].a=a;
    myTemp[i].b=b;
    i = i+1;
  }
}

int main()
{
  int i1, i2;
  foo0(10,20.0);
  foo1();
  printf("%c, %s\n",myTemp[0].c[0][i1],myTemp[0].c[myTemp[i2].a]);
  return;
}


