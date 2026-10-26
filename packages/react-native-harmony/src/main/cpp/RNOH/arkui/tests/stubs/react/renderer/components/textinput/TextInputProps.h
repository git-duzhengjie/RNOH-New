/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
#pragma once
// STUB: react/renderer/components/textinput/TextInputProps.h
// The real TextInputProps.h only exists for iOS/Android platform; no
// cxx variant ships. This stub includes the real attributedstring
// headers (which DO exist for cxx) so that TextInputNodeBase.h —
// which transitively depends on TextAttributes / TextAlignment /
// KeyboardType / ReturnKeyType — compiles on the host.
#include <react/renderer/attributedstring/TextAttributes.h>
#include <react/renderer/attributedstring/primitives.h>
#include "react/renderer/components/textinput/primitives.h"
namespace facebook::react {}
