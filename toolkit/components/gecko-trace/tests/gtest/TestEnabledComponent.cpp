#include "gtest/gtest.h"

#include "mozilla/GeckoTrace.h"

TEST(GeckoTraceEnabledComponent, Foo)
{
  {
    GECKO_TRACE_SCOPE("test", "Foo");
  }
}
