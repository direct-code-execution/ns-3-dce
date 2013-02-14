#ifndef TEST_MACROS_H
#define TEST_MACROS_H

#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>
#include <iostream>

typedef void (*Ns3ReportTestError)(const char *);
//extern "C" Ns3ReportTestError g_ns3_report_test_error __attribute__((visibility("default")));

#define OUTPUT(x)                                                       \
  {                                                                     \
    std::ostringstream oss;                                             \
    oss << "file=" << __FILE__ << " line=" << __LINE__ << " "           \
        << x << std::endl;                                              \
    std::string s = oss.str ();                                         \
  }


#define TEST_ASSERT_EQUAL(a,b)                  \
  if ((a) != (b))                               \
    {                                           \
      OUTPUT ("assert failed " << "\"" << (a) << "\" == \"" << (b) << "\""); \
      exit (1);                                 \
    }

#define TEST_ASSERT_UNEQUAL(a,b)                \
  if ((a) == (b))                               \
    {                                           \
      OUTPUT ("assert failed " << "\"" << (a) << "\" != \"" << (b) << "\""); \
      exit (1);                                 \
    }

#define TEST_ASSERT(v)                          \
  if (!(v))                                     \
    {                                           \
      OUTPUT ("assert failed " << "(" << (v) << ")"); \
      exit (1);                                 \
    }


#endif /* TEST_MACROS_H */
