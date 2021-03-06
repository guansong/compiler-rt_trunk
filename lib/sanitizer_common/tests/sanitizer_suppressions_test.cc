//===-- sanitizer_suppressions_test.cc ------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer/AddressSanitizer runtime.
//
//===----------------------------------------------------------------------===//
#include "sanitizer_common/sanitizer_suppressions.h"
#include "gtest/gtest.h"

#include <string.h>

namespace __sanitizer {

static bool MyMatch(const char *templ, const char *func) {
  char tmp[1024];
  strcpy(tmp, templ);  // NOLINT
  return TemplateMatch(tmp, func);
}

TEST(Suppressions, Match) {
  EXPECT_TRUE(MyMatch("foobar$", "foobar"));

  EXPECT_TRUE(MyMatch("foobar", "foobar"));
  EXPECT_TRUE(MyMatch("*foobar*", "foobar"));
  EXPECT_TRUE(MyMatch("foobar", "prefix_foobar_postfix"));
  EXPECT_TRUE(MyMatch("*foobar*", "prefix_foobar_postfix"));
  EXPECT_TRUE(MyMatch("foo*bar", "foo_middle_bar"));
  EXPECT_TRUE(MyMatch("foo*bar", "foobar"));
  EXPECT_TRUE(MyMatch("foo*bar*baz", "foo_middle_bar_another_baz"));
  EXPECT_TRUE(MyMatch("foo*bar*baz", "foo_middle_barbaz"));
  EXPECT_TRUE(MyMatch("^foobar", "foobar"));
  EXPECT_TRUE(MyMatch("^foobar", "foobar_postfix"));
  EXPECT_TRUE(MyMatch("^*foobar", "foobar"));
  EXPECT_TRUE(MyMatch("^*foobar", "prefix_foobar"));
  EXPECT_TRUE(MyMatch("foobar$", "foobar"));
  EXPECT_TRUE(MyMatch("foobar$", "prefix_foobar"));
  EXPECT_TRUE(MyMatch("*foobar*$", "foobar"));
  EXPECT_TRUE(MyMatch("*foobar*$", "foobar_postfix"));
  EXPECT_TRUE(MyMatch("^foobar$", "foobar"));

  EXPECT_FALSE(MyMatch("foo", "baz"));
  EXPECT_FALSE(MyMatch("foobarbaz", "foobar"));
  EXPECT_FALSE(MyMatch("foobarbaz", "barbaz"));
  EXPECT_FALSE(MyMatch("foo*bar", "foobaz"));
  EXPECT_FALSE(MyMatch("foo*bar", "foo_baz"));
  EXPECT_FALSE(MyMatch("^foobar", "prefix_foobar"));
  EXPECT_FALSE(MyMatch("foobar$", "foobar_postfix"));
  EXPECT_FALSE(MyMatch("^foobar$", "prefix_foobar"));
  EXPECT_FALSE(MyMatch("^foobar$", "foobar_postfix"));
  EXPECT_FALSE(MyMatch("foo^bar", "foobar"));
  EXPECT_FALSE(MyMatch("foo$bar", "foobar"));
  EXPECT_FALSE(MyMatch("foo$^bar", "foobar"));
}

static const char *kTestSuppressionTypes[] = {"race", "thread", "mutex",
                                              "signal"};

class SuppressionContextTest : public ::testing::Test {
 public:
  SuppressionContextTest()
      : ctx_(kTestSuppressionTypes, ARRAY_SIZE(kTestSuppressionTypes)) {}

 protected:
  SuppressionContext ctx_;

  void CheckSuppressions(unsigned count, std::vector<const char *> types,
                         std::vector<const char *> templs) const {
    EXPECT_EQ(count, ctx_.SuppressionCount());
    for (unsigned i = 0; i < count; i++) {
      const Suppression *s = ctx_.SuppressionAt(i);
      EXPECT_STREQ(types[i], s->type);
      EXPECT_STREQ(templs[i], s->templ);
    }
  }
};

TEST_F(SuppressionContextTest, Parse) {
  ctx_.Parse("race:foo\n"
             " 	race:bar\n"  // NOLINT
             "race:baz	 \n" // NOLINT
             "# a comment\n"
             "race:quz\n"); // NOLINT
  CheckSuppressions(4, {"race", "race", "race", "race"},
                    {"foo", "bar", "baz", "quz"});
}

TEST_F(SuppressionContextTest, Parse2) {
  ctx_.Parse(
    "  	# first line comment\n"  // NOLINT
    " 	race:bar 	\n"  // NOLINT
    "race:baz* *baz\n"
    "# a comment\n"
    "# last line comment\n"
  );  // NOLINT
  CheckSuppressions(2, {"race", "race"}, {"bar", "baz* *baz"});
}

TEST_F(SuppressionContextTest, Parse3) {
  ctx_.Parse(
    "# last suppression w/o line-feed\n"
    "race:foo\n"
    "race:bar"
  );  // NOLINT
  CheckSuppressions(2, {"race", "race"}, {"foo", "bar"});
}

TEST_F(SuppressionContextTest, ParseType) {
  ctx_.Parse(
    "race:foo\n"
    "thread:bar\n"
    "mutex:baz\n"
    "signal:quz\n"
  );  // NOLINT
  CheckSuppressions(4, {"race", "thread", "mutex", "signal"},
                    {"foo", "bar", "baz", "quz"});
}

TEST_F(SuppressionContextTest, HasSuppressionType) {
  ctx_.Parse(
    "race:foo\n"
    "thread:bar\n");
  EXPECT_TRUE(ctx_.HasSuppressionType("race"));
  EXPECT_TRUE(ctx_.HasSuppressionType("thread"));
  EXPECT_FALSE(ctx_.HasSuppressionType("mutex"));
  EXPECT_FALSE(ctx_.HasSuppressionType("signal"));
}

}  // namespace __sanitizer
