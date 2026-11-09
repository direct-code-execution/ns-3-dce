#include <stdio.h>
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

// A fortified sprintf into a buffer whose size the compiler cannot see
// passes (size_t) -1 as its size, as OpenSSL's TLS key log does; it once
// wrote a single character under DCE.
void test_sprintf_chk (void)
{
  char *buf = (char *) malloc (16);
  char *cursor = buf;
  const unsigned char bytes[] = { 0x7a, 0x1b, 0xc3 };
  for (int i = 0; i < 3; i++)
    {
      __builtin___sprintf_chk (cursor, 1, (size_t) -1, "%02x", bytes[i]);
      cursor += 2;
    }
  TEST_ASSERT (strcmp (buf, "7a1bc3") == 0);
  free (buf);
}

int main (int argc, char *argv[])
{
  test_strdup ();
  test_sprintf_chk ();
  return 0;
}
