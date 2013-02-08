#include <stdio.h>
#include <stdlib.h>

int main (int c, char **v)
{
  setenv ("FRED", "O", 0);

  printf ("Hello from %s ! FRED=%s argc=%d\n", v[0], getenv ("FRED"), c);


  for (int i = 0; i < c; i++)
    {
      printf ("Arg[%d]=%s\n", i, v[i]);
    }

  return 0;
}
