#include "TestGeneratedEvents.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mozilla/GeckoTrace.h"

#include "Common.h"

using namespace ::testing;

class GeckoTraceGeneratedEventsTest : public GeckoTraceTestFixture {};

TEST_F(GeckoTraceGeneratedEventsTest, Simple) {
  {
    GECKO_TRACE_SCOPE("gtests", "Test simple event")

    mozilla::gecko_trace::event::TestSimple()
        .WithTestString("Hello mom")
        .WithTestInteger(161)
        .WithTestBoolean(true)
        .Emit();
  }

  const auto spans = GetSpans();
  ASSERT_EQ(spans.size(), 1u);
  const auto events = spans[0]->GetEvents();
  ASSERT_EQ(events.size(), 1u);
  const auto attrs = events[0].GetAttributes();
  EXPECT_EQ(attrs.size(), 3u);
  EXPECT_EQ(std::get<std::string>(attrs.at("test_string")), "Hello mom");
  EXPECT_EQ(std::get<int64_t>(attrs.at("test_integer")), 161u);
  EXPECT_EQ(std::get<bool>(attrs.at("test_boolean")), true);
}

TEST_F(GeckoTraceGeneratedEventsTest, WithDisplayName) {
  {
    GECKO_TRACE_SCOPE("gtests", "Test event with display name")

    mozilla::gecko_trace::event::TestWithDisplayName()
        .WithBasicAttr("display test")
        .Emit();
  }

  const auto spans = GetSpans();
  ASSERT_EQ(spans.size(), 1u);
  const auto events = spans[0]->GetEvents();
  ASSERT_EQ(events.size(), 1u);
  const auto attrs = events[0].GetAttributes();
  EXPECT_EQ(attrs.size(), 1u);
  EXPECT_EQ(std::get<std::string>(attrs.at("basic_attr")), "display test");
}

TEST_F(GeckoTraceGeneratedEventsTest, Arrays) {
  constexpr std::string_view strings[]{"hello", "world", "test"};
  constexpr int64_t integers[]{1, 42, 5, 100};
  constexpr bool booleans[]{true, false, true};

  {
    GECKO_TRACE_SCOPE("gtests", "Test arrays event")

    mozilla::gecko_trace::event::TestArrays()
        .WithStringArray(strings)
        .WithIntegerArray(integers)
        .WithBooleanArray(booleans)
        .Emit();
  }

  const auto spans = GetSpans();
  ASSERT_EQ(spans.size(), 1u);
  const auto events = spans[0]->GetEvents();
  ASSERT_EQ(events.size(), 1u);
  const auto attrs = events[0].GetAttributes();
  EXPECT_EQ(attrs.size(), 3u);
  EXPECT_THAT(std::get<std::vector<std::string>>(attrs.at("string_array")),
              ElementsAreArray(strings));
  EXPECT_THAT(std::get<std::vector<int64_t>>(attrs.at("integer_array")),
              ElementsAreArray(integers));
  EXPECT_THAT(std::get<std::vector<bool>>(attrs.at("boolean_array")),
              ElementsAreArray(booleans));
}

TEST_F(GeckoTraceGeneratedEventsTest, NamingPatterns) {
  {
    GECKO_TRACE_SCOPE("gtests", "Test naming patterns event")

    mozilla::gecko_trace::event::TestNamingPatterns()
        .WithSimpleName("simple")
        .WithDottedName(123)
        .WithNameWithNumbers123(false)
        .WithComplexDottedName456End("complex")
        .Emit();
  }

  const auto spans = GetSpans();
  ASSERT_EQ(spans.size(), 1u);
  const auto events = spans[0]->GetEvents();
  ASSERT_EQ(events.size(), 1u);
  const auto attrs = events[0].GetAttributes();
  EXPECT_EQ(attrs.size(), 4u);
  EXPECT_EQ(std::get<std::string>(attrs.at("simple_name")), "simple");
  EXPECT_EQ(std::get<int64_t>(attrs.at("dotted.name")), 123u);
  EXPECT_EQ(std::get<bool>(attrs.at("name_with_numbers123")), false);
  EXPECT_EQ(std::get<std::string>(attrs.at("complex.dotted_name456.end")),
            "complex");
}

TEST_F(GeckoTraceGeneratedEventsTest, SingleInheritance) {
  {
    GECKO_TRACE_SCOPE("gtests", "Test single inheritance event")

    mozilla::gecko_trace::event::TestSingleInheritance()
        .WithTestString("inherited")
        .WithTestInteger(999)
        .WithTestBoolean(false)
        .WithAdditionalData("extra data")
        .Emit();
  }

  const auto spans = GetSpans();
  ASSERT_EQ(spans.size(), 1u);
  const auto events = spans[0]->GetEvents();
  ASSERT_EQ(events.size(), 1u);
  const auto attrs = events[0].GetAttributes();
  EXPECT_EQ(attrs.size(), 4u);
  EXPECT_EQ(std::get<std::string>(attrs.at("test_string")), "inherited");
  EXPECT_EQ(std::get<int64_t>(attrs.at("test_integer")), 999u);
  EXPECT_EQ(std::get<bool>(attrs.at("test_boolean")), false);
  EXPECT_EQ(std::get<std::string>(attrs.at("additional_data")), "extra data");
}

TEST_F(GeckoTraceGeneratedEventsTest, MultipleInheritance) {
  {
    GECKO_TRACE_SCOPE("gtests", "Test multiple inheritance event")

    mozilla::gecko_trace::event::TestMultipleInheritance()
        .WithTestString("multi inherited")
        .WithTestInteger(777)
        .WithTestBoolean(true)
        .WithBasicAttr("from display name event")
        .WithMultiInheritAttr(555)
        .Emit();
  }

  const auto spans = GetSpans();
  ASSERT_EQ(spans.size(), 1u);
  const auto events = spans[0]->GetEvents();
  ASSERT_EQ(events.size(), 1u);
  const auto attrs = events[0].GetAttributes();
  EXPECT_EQ(attrs.size(), 5u);
  EXPECT_EQ(std::get<std::string>(attrs.at("test_string")), "multi inherited");
  EXPECT_EQ(std::get<int64_t>(attrs.at("test_integer")), 777u);
  EXPECT_EQ(std::get<bool>(attrs.at("test_boolean")), true);
  EXPECT_EQ(std::get<std::string>(attrs.at("basic_attr")),
            "from display name event");
  EXPECT_EQ(std::get<int64_t>(attrs.at("multi_inherit_attr")), 555u);
}

TEST_F(GeckoTraceGeneratedEventsTest, Complex) {
  constexpr int64_t errorCodes[] = {404, 500, 503};

  {
    GECKO_TRACE_SCOPE("gtests", "Test complex event")

    mozilla::gecko_trace::event::TestComplex()
        .WithTestString("complex test")
        .WithTestInteger(42)
        .WithTestBoolean(true)
        .WithTestData("additional data")
        .WithRetryCount(3)
        .WithErrorCodes(errorCodes)
        .Emit();
  }

  const auto spans = GetSpans();
  ASSERT_EQ(spans.size(), 1u);
  const auto events = spans[0]->GetEvents();
  ASSERT_EQ(events.size(), 1u);
  const auto attrs = events[0].GetAttributes();
  EXPECT_EQ(attrs.size(), 6u);
  EXPECT_EQ(std::get<std::string>(attrs.at("test_string")), "complex test");
  EXPECT_EQ(std::get<int64_t>(attrs.at("test_integer")), 42u);
  EXPECT_EQ(std::get<bool>(attrs.at("test_boolean")), true);
  EXPECT_EQ(std::get<std::string>(attrs.at("test_data")), "additional data");
  EXPECT_EQ(std::get<int64_t>(attrs.at("retry.count")), 3u);
  EXPECT_THAT(std::get<std::vector<int64_t>>(attrs.at("error_codes")),
              ElementsAreArray(errorCodes));
}

TEST_F(GeckoTraceGeneratedEventsTest, DeepInheritance) {
  constexpr int64_t errorCodes[] = {200, 201};
  constexpr bool debugFlags[] = {true, false, true, true};

  {
    GECKO_TRACE_SCOPE("gtests", "Test deep inheritance event")

    mozilla::gecko_trace::event::TestDeepInheritance()
        .WithTestString("deep inheritance")
        .WithTestInteger(1337)
        .WithTestBoolean(false)
        .WithTestData("deep data")
        .WithRetryCount(10)
        .WithErrorCodes(errorCodes)
        .WithExtraInfo("deep inheritance info")
        .WithDebugFlags(debugFlags)
        .Emit();
  }

  const auto spans = GetSpans();
  ASSERT_EQ(spans.size(), 1u);
  const auto events = spans[0]->GetEvents();
  ASSERT_EQ(events.size(), 1u);
  const auto attrs = events[0].GetAttributes();
  EXPECT_EQ(attrs.size(), 8u);
  EXPECT_EQ(std::get<std::string>(attrs.at("test_string")), "deep inheritance");
  EXPECT_EQ(std::get<int64_t>(attrs.at("test_integer")), 1337u);
  EXPECT_EQ(std::get<bool>(attrs.at("test_boolean")), false);
  EXPECT_EQ(std::get<std::string>(attrs.at("test_data")), "deep data");
  EXPECT_EQ(std::get<int64_t>(attrs.at("retry.count")), 10u);
  EXPECT_THAT(std::get<std::vector<int64_t>>(attrs.at("error_codes")),
              ElementsAreArray(errorCodes));
  EXPECT_EQ(std::get<std::string>(attrs.at("extra.info")),
            "deep inheritance info");
  EXPECT_THAT(std::get<std::vector<bool>>(attrs.at("debug_flags")),
              ElementsAreArray(debugFlags));
}

TEST_F(GeckoTraceGeneratedEventsTest, ComplexMultipleInheritance) {
  constexpr std::string_view strings[] = {"test1", "test2"};
  constexpr int64_t integers[] = {10, 20, 30};
  constexpr bool booleans[] = {false, true};

  {
    GECKO_TRACE_SCOPE("gtests", "Test complex multiple inheritance event")

    mozilla::gecko_trace::event::TestComplexMultipleInheritance()
        .WithStringArray(strings)
        .WithIntegerArray(integers)
        .WithBooleanArray(booleans)
        .WithSimpleName("inherited simple")
        .WithDottedName(456)
        .WithNameWithNumbers123(true)
        .WithComplexDottedName456End("inherited complex")
        .WithFinalAttr("final attribute")
        .Emit();
  }

  const auto spans = GetSpans();
  ASSERT_EQ(spans.size(), 1u);
  const auto events = spans[0]->GetEvents();
  ASSERT_EQ(events.size(), 1u);
  const auto attrs = events[0].GetAttributes();
  EXPECT_EQ(attrs.size(), 8u);
  EXPECT_THAT(std::get<std::vector<std::string>>(attrs.at("string_array")),
              ElementsAreArray(strings));
  EXPECT_THAT(std::get<std::vector<int64_t>>(attrs.at("integer_array")),
              ElementsAreArray(integers));
  EXPECT_THAT(std::get<std::vector<bool>>(attrs.at("boolean_array")),
              ElementsAreArray(booleans));
  EXPECT_EQ(std::get<std::string>(attrs.at("simple_name")), "inherited simple");
  EXPECT_EQ(std::get<int64_t>(attrs.at("dotted.name")), 456u);
  EXPECT_EQ(std::get<bool>(attrs.at("name_with_numbers123")), true);
  EXPECT_EQ(std::get<std::string>(attrs.at("complex.dotted_name456.end")),
            "inherited complex");
  EXPECT_EQ(std::get<std::string>(attrs.at("final_attr")), "final attribute");
}

TEST_F(GeckoTraceGeneratedEventsTest, NumberedEvent) {
  {
    GECKO_TRACE_SCOPE("gtests", "Test numbered event")

    mozilla::gecko_trace::event::Test123NumberedEvent().WithAttr456(789).Emit();
  }

  const auto spans = GetSpans();
  ASSERT_EQ(spans.size(), 1u);
  const auto events = spans[0]->GetEvents();
  ASSERT_EQ(events.size(), 1u);
  const auto attrs = events[0].GetAttributes();
  EXPECT_EQ(attrs.size(), 1u);
  EXPECT_EQ(std::get<int64_t>(attrs.at("attr456")), 789u);
}

TEST_F(GeckoTraceGeneratedEventsTest, DeepNestedEventName) {
  {
    GECKO_TRACE_SCOPE("gtests", "Test deeply nested event name")

    mozilla::gecko_trace::event::TestDeepNestedEventName()
        .WithDeeplyNestedAttr("nested value")
        .WithSimpleAttr(true)
        .Emit();
  }

  const auto spans = GetSpans();
  ASSERT_EQ(spans.size(), 1u);
  const auto events = spans[0]->GetEvents();
  ASSERT_EQ(events.size(), 1u);
  const auto attrs = events[0].GetAttributes();
  EXPECT_EQ(attrs.size(), 2u);
  EXPECT_EQ(std::get<std::string>(attrs.at("deeply.nested.attr")),
            "nested value");
  EXPECT_EQ(std::get<bool>(attrs.at("simple_attr")), true);
}

TEST_F(GeckoTraceGeneratedEventsTest, PartialAttributes) {
  {
    GECKO_TRACE_SCOPE("gtests", "Test partial attributes")

    mozilla::gecko_trace::event::TestSimple()
        .WithTestString("only string")
        .Emit();
  }

  const auto spans = GetSpans();
  ASSERT_EQ(spans.size(), 1u);
  const auto events = spans[0]->GetEvents();
  ASSERT_EQ(events.size(), 1u);
  const auto attrs = events[0].GetAttributes();
  EXPECT_EQ(attrs.size(), 1u);
  EXPECT_EQ(std::get<std::string>(attrs.at("test_string")), "only string");
}

TEST_F(GeckoTraceGeneratedEventsTest, EmptyEvent) {
  {
    GECKO_TRACE_SCOPE("gtests", "Test empty event")

    mozilla::gecko_trace::event::TestSimple().Emit();
  }

  const auto spans = GetSpans();
  ASSERT_EQ(spans.size(), 1u);
  const auto events = spans[0]->GetEvents();
  ASSERT_EQ(events.size(), 1u);
  const auto attrs = events[0].GetAttributes();
  EXPECT_EQ(attrs.size(), 0u);
}
