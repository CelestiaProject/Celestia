#include <stdio.h>

int main(int argc, char *argv[])
{
   FILE *fp = stdin;
   char buf[256];

   for (;;) {
      fgets(buf, sizeof(buf), fp);
      
   }
}

