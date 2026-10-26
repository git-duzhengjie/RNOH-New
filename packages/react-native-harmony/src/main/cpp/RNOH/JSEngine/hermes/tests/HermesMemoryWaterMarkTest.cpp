#include "HermesMemoryWaterMark.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace rnoh;
using ::testing::HasSubstr;

TEST(HermesMemoryWaterMark, ConvertBytesToMB_ZeroBytes_ReturnsZero) {
  EXPECT_DOUBLE_EQ(convertBytesToMB(0), 0.0);
}

TEST(HermesMemoryWaterMark, ConvertBytesToMB_OneMB_ReturnsOne) {
  EXPECT_DOUBLE_EQ(convertBytesToMB(BYTES_PER_MB), 1.0);
}

TEST(HermesMemoryWaterMark, ConvertBytesToMB_LargeValue_ReturnsCorrectMB) {
  constexpr int64_t oneGB = 1024LL * BYTES_PER_MB;
  EXPECT_NEAR(convertBytesToMB(oneGB), 1024.0, 1e-6);
}

TEST(HermesMemoryWaterMark, ConvertBytesToMB_NegativeBytes_ReturnsNegativeMB) {
  EXPECT_DOUBLE_EQ(convertBytesToMB(-BYTES_PER_MB), -1.0);
}

TEST(HermesMemoryWaterMark, FormatBytesToMBString_ZeroBytes_ReturnsZeroString) {
  EXPECT_EQ(formatBytesToMBString(0), "0.00");
}

TEST(HermesMemoryWaterMark, FormatBytesToMBString_OneMB_ReturnsOneString) {
  EXPECT_EQ(formatBytesToMBString(BYTES_PER_MB), "1.00");
}

TEST(HermesMemoryWaterMark, FormatBytesToMBString_HalfMB_ReturnsHalfString) {
  EXPECT_EQ(formatBytesToMBString(BYTES_PER_MB / 2), "0.50");
}

TEST(
    HermesMemoryWaterMark,
    BuildHermesWaterMarkDescription_BothNonZero_ReturnsFormattedString) {
  auto result = buildHermesWaterMarkDescription(2 * BYTES_PER_MB, BYTES_PER_MB);
  EXPECT_THAT(result, HasSubstr("2.00 MB"));
  EXPECT_THAT(result, HasSubstr("1.00 MB"));
  EXPECT_THAT(result, HasSubstr("2097152"));
  EXPECT_THAT(result, HasSubstr("1048576"));
}

TEST(
    HermesMemoryWaterMark,
    BuildHermesWaterMarkDescription_BothZero_ReturnsZeroValues) {
  auto result = buildHermesWaterMarkDescription(0, 0);
  EXPECT_THAT(result, HasSubstr("0.00 MB"));
}

class HermesMemoryWaterMarkStateTest : public ::testing::Test {
 protected:
  void SetUp() override {
    setHermesMemoryWaterMarkMB(DEFAULT_HERMES_MEMORY_WATER_MARK_MB);
  }
};

TEST_F(
    HermesMemoryWaterMarkStateTest,
    SetAndGetHermesMemoryWaterMarkMB_SetValue_GetReturnsSameValue) {
  setHermesMemoryWaterMarkMB(512);
  EXPECT_EQ(getHermesMemoryWaterMarkMB(), 512);
}

TEST_F(
    HermesMemoryWaterMarkStateTest,
    GetHermesMemoryWaterMarkBytes_SetValue_ReturnsValueInBytes) {
  setHermesMemoryWaterMarkMB(100);
  EXPECT_EQ(getHermesMemoryWaterMarkBytes(), 100 * BYTES_PER_MB);
}

TEST_F(
    HermesMemoryWaterMarkStateTest,
    GetHermesMemoryWaterMarkBytes_DefaultValue_ReturnsDefaultInBytes) {
  EXPECT_EQ(
      getHermesMemoryWaterMarkBytes(),
      DEFAULT_HERMES_MEMORY_WATER_MARK_MB * BYTES_PER_MB);
}

TEST(
    HermesMemoryWaterMark,
    TryMarkHermesWaterMark_FirstCallTrueSecondCallFalse) {
  EXPECT_TRUE(tryMarkHermesWaterMarkExceededForProcessLifetime());
  EXPECT_FALSE(tryMarkHermesWaterMarkExceededForProcessLifetime());
}

TEST(HermesMemoryWaterMark, GetHermesFrameworkVersion_ReturnsVersionString) {
  EXPECT_EQ(getHermesFrameworkVersion(), std::string(RNOH_FRAMEWORK_VERSION));
}
