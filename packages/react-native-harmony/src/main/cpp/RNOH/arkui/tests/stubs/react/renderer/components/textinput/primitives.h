/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
#pragma once
namespace facebook::react {
enum class KeyboardType {
  Default,
  EmailAddress,
  Numeric,
  NumberPad,
  DecimalPad,
  PhonePad
};
enum class ReturnKeyType { Default, Done, Go, Next, Search, Send, Previous };
} // namespace facebook::react
