#include <stdlib.h>
#include <errno.h>
#include "test-macros.h"

static void test_strtod (void)
{
  double retval;
  retval = strtod ("55", NULL);
  TEST_ASSERT_EQUAL (retval, 55);
  retval = strtod ("-127", NULL);
  TEST_ASSERT_EQUAL (retval, -127);
  retval = strtod ("122.233", NULL);
  TEST_ASSERT_EQUAL (retval, 122.233);
  retval = strtod ("0x122.3", NULL);
  TEST_ASSERT_EQUAL (retval, 290.1875);
  retval = strtod ("2e3", NULL);
  TEST_ASSERT_EQUAL (retval, 2000);
}

static void test_mkstemp(void)
{
    /* dce_mkstemp */
    int ret = 0;
    std::string suffix = "XXXXXX";
    std::string prefix = "test";
    std::string oldFilename = prefix + suffix;
    const std::size_t filenamelen = 30;
    char filename[filenamelen];
    strncpy (filename, oldFilename.c_str(), filenamelen - 1);
    filename[oldFilename.size() - 1] = '\0';
    ret = mkstemp (filename);
    std::string newFilename (filename);
    std::clog << "filename " << filename << std::endl;

    /* test that the suffix part was actually changed and the prefix untouched */
    TEST_ASSERT_EQUAL (newFilename.substr (0,4), prefix);
    TEST_ASSERT_UNEQUAL (newFilename.substr (prefix.size()), suffix);

}

int main (int argc, char *argv[])
{
  test_strtod ();
  test_mkstemp ();
  return 0;
}
