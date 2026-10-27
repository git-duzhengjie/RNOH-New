#include <react/featureflags/ReactNativeFeatureFlagsDefaults.h>

namespace rnoh {
// RN 0.88: enableFabricRenderer / useTurboModules 在
// ReactNativeFeatureFlagsDefaults 中已不是虚函数（取值固定），不能 override，
// 因此仅保留仍可覆盖的项。
class RNFeatureFlags : public facebook::react::ReactNativeFeatureFlagsDefaults {
 public:
  bool enableBridgelessArchitecture() override {
    return true;
  }
  bool useNativeViewConfigsInBridgelessMode() override {
    return true;
  }
};
} // namespace rnoh