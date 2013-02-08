#include <dirent.h>
#include <iostream>
#include <sstream>
#include "test-macros.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

void
test2 (void) // Stolen from man
{
  struct dirent **namelist;
  int n;

  n = scandir (".", &namelist, 0, alphasort);
  TEST_ASSERT (n >= 0);
  while (n--)
    {
      struct dirent dd;

      memcpy (&dd, namelist[n], sizeof (struct dirent));

      printf ("%s\n", namelist[n]->d_name);
      free (namelist[n]);
    }
  free (namelist);
}

void
test1 (void)
{
  std::ostringstream s;

  s << "/var/log/" << getpid () << "/";

  std::string outPath = s.str ();

  DIR *dp = 0;
  struct dirent *dirp = 0;
  int cpt1, cpt2;

  cpt1 = cpt2 = 0;
  dp = opendir (outPath.c_str ());

  TEST_ASSERT (dp != 0);

  while ((dirp = readdir (dp)) != NULL)
    {
      std::cout <<  dirp->d_name << std::endl;
      cpt1++;
    }
  TEST_ASSERT (cpt1 > 0);

  rewinddir (dp);

  std::cout <<  "Rewind then redo ! " << std::endl;

  while ((dirp = readdir (dp)) != NULL)
    {
      std::cout <<  dirp->d_name << std::endl;
      cpt2++;
    }
  TEST_ASSERT (cpt2 > 0);
  TEST_ASSERT_EQUAL (cpt1, cpt2);

  int st = closedir (dp);

  TEST_ASSERT_EQUAL (st, 0);

}

int main (int c, char **v)
{
  // test1 ();
  test2 ();

  return 0;
}
