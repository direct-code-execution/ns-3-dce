#include "test-macros.h"

extern "C" void ns3_report_test_error (const char *s);

void ns3_report_test_error (const char *s)
{
  std::cerr << s;
}

Ns3ReportTestError g_ns3_report_test_error = ns3_report_test_error;

