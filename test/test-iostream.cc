#include <sstream>
#include <string>
#include <ios>
#include "test-macros.h"

template<typename T>
static std::string
to_string_via_ostream (T value)
{
  std::ostringstream oss;
  oss << value;             // real libstdc++ num_put facet dispatch
  return oss.str ();
}

int
main (int argc, char *argv[])
{
  // Integer insertion (the exact operation that used to SIGSEGV in
  // codecvt::do_unshift via std::ostream::_M_insert<long>).
  TEST_ASSERT_EQUAL (to_string_via_ostream<int> (12345), std::string ("12345"));
  TEST_ASSERT_EQUAL (to_string_via_ostream<long> (-9876543210L), std::string ("-9876543210"));
  TEST_ASSERT_EQUAL (to_string_via_ostream<unsigned> (4000000000U), std::string ("4000000000"));

  // Hex / formatting flags pull in the numpunct/num_put facets too.
  {
    std::ostringstream oss;
    oss << std::hex << std::showbase << 255;
    TEST_ASSERT_EQUAL (oss.str (), std::string ("0xff"));
  }

  // Floating point uses num_put<char>::do_put(double) as well.
  {
    std::ostringstream oss;
    oss << 3.5;
    TEST_ASSERT_EQUAL (oss.str (), std::string ("3.5"));
  }

  // Mixed string + number insertion, the common real-world pattern (and the
  // shape of the OUTPUT() macro in test-macros.h that every TEST_ASSERT uses).
  {
    std::ostringstream oss;
    oss << "value=" << 42 << " done";
    TEST_ASSERT_EQUAL (oss.str (), std::string ("value=42 done"));
  }

  return 0;
}
