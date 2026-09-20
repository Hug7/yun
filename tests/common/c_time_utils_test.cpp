/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "c_time_utils.h"

#include <gtest/gtest.h>

#include <string>

namespace {

TEST(TimeParseTest, FormatSecToTm) {
  // chrono_util 默认时区为 Asia/Shanghai(+08:00)
  const long sec = TimeParse::parse_tm_to_sec("2026-09-20 08:30", TimeParse::fmt_yyyymmddhhmm_1);
  EXPECT_EQ("2026-09-20 08:30", TimeParse::format_sec_to_tm(sec, TimeParse::fmt_yyyymmddhhmm_1));
  EXPECT_EQ("202609200830", TimeParse::format_sec_to_tm(sec, TimeParse::fmt_yyyymmddhhmm_2));
}

TEST(TimeParseTest, FormatSecToTmRoundTrip) {
  const std::string time_str = "2025-12-31 23:59";
  const long sec = TimeParse::parse_tm_to_sec(time_str, TimeParse::fmt_yyyymmddhhmm_1);
  const std::string formatted = TimeParse::format_sec_to_tm(sec, TimeParse::fmt_yyyymmddhhmm_2);
  EXPECT_EQ("202512312359", formatted);
  EXPECT_EQ(sec, TimeParse::parse_tm_to_sec(formatted, TimeParse::fmt_yyyymmddhhmm_2));
  // 跨年进位
  EXPECT_EQ("2026-01-01 00:00",
            TimeParse::format_sec_to_tm(sec + 60, TimeParse::fmt_yyyymmddhhmm_1));
}

}  // namespace
