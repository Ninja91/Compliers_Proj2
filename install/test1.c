int c = 1;

//int b = 2;
//static int y = 1;
int y = 0;
void foo(int i)
{
   int a;
   float b;
   int x;
   //a = x + 1;
   b = 3.0f;
   //y = 1;
   //y += 1;
    
   //scanf("%f", &b);
   
   if (i) {
       x = 1;
       a = a + 1;
    y = 1;
   }
   else {
       x = 2;
    y = 1;
    printf("b = %d", b);
    printf("y = %d", y);
   }

   printf("x = %d", x);
   printf("a = %d", a);
   printf("b = %d", b);
}
main()
{
    //int b;
   foo(1);
   //return b;
}

