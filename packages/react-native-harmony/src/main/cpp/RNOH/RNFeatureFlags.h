#include <react/featureflags/ReactNativeFeatureFlagsDefaults.h>

namespace rnoh {
class RNFeatureFlags : public facebook::react::ReactNativeFeatureFlagsDefaults {
 public:
  bool enableBridgelessArchitecture() override {
    return true;
  }
  bool enableFabricRenderer() override {
    return true;
  }
  bool useTurboModules() override {
    return true;
  }
  bool useNativeViewConfigsInBridgelessMode() override {
    return true;
  }
};
} // namespace rnoh