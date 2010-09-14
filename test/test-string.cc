#include <string.h>
#include <stdlib.h>
#include "test-macros.h"

void test_strdup (void)
{
  const char *s = "test";
  char *copy = strdup (s);
  free (copy);
  copy = strndup (s, 2);
  TEST_ASSERT (strcmp (copy, "te") == 0);
  free (copy);
}

int main (int argc, char *argv[])
{
  test_strdup ();
  return 0;
}
