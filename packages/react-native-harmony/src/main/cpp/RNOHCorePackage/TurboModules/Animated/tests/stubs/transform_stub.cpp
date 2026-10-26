/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Minimal host stub for `facebook::react::Transform`'s out-of-line
 * methods. The production `Transform.cpp` pulls in
 * `<react/debug/react_native_assert.h>` (which transitively includes
 * `<android/log.h>`, unavailable on macOS/Linux) and tracks a parallel
 * `operations` vector that `TransformAnimatedNode` does not read.
 *
 * The stub here implements only the static factory methods
 * (`Translate`/`Scale`/`Rotate`/`RotateX`/`RotateY`/`RotateZ`/
 * `Perspective`) and the binary `operator*` (matrix multiplication),
 * matching the row-major 4×4 layout used by the production code. Tests
 * never inspect `operations` and the `operator==` short-circuit in
 * `operator*` falls back to a matrix-only comparison, which is
 * sufficient for the matrices constructed in the Animated tests.
 */

#include <react/renderer/graphics/Transform.h>

#include <cmath>

namespace facebook::react {

/* static */ Transform Transform::Perspective(Float perspective) noexcept {
  Transform t{};
  if (perspective != 0) {
    t.matrix[11] = -1.0f / perspective;
  }
  return t;
}

/* static */ Transform Transform::Scale(Float x, Float y, Float z) noexcept {
  Transform t{};
  t.matrix[0] = x;
  t.matrix[5] = y;
  t.matrix[10] = z;
  return t;
}

/* static */ Transform
Transform::Translate(Float x, Float y, Float z) noexcept {
  Transform t{};
  t.matrix[12] = x;
  t.matrix[13] = y;
  t.matrix[14] = z;
  return t;
}

/* static */ Transform Transform::RotateX(Float radians) noexcept {
  Transform t{};
  float c = std::cos(radians);
  float s = std::sin(radians);
  t.matrix[5] = c;
  t.matrix[6] = s;
  t.matrix[9] = -s;
  t.matrix[10] = c;
  return t;
}

/* static */ Transform Transform::RotateY(Float radians) noexcept {
  Transform t{};
  float c = std::cos(radians);
  float s = std::sin(radians);
  t.matrix[0] = c;
  t.matrix[2] = -s;
  t.matrix[8] = s;
  t.matrix[10] = c;
  return t;
}

/* static */ Transform Transform::RotateZ(Float radians) noexcept {
  Transform t{};
  float c = std::cos(radians);
  float s = std::sin(radians);
  t.matrix[0] = c;
  t.matrix[1] = s;
  t.matrix[4] = -s;
  t.matrix[5] = c;
  return t;
}

/* static */ Transform Transform::Rotate(Float x, Float y, Float z) noexcept {
  Transform t{};
  if (x != 0.0f) {
    t = t * RotateX(x);
  }
  if (y != 0.0f) {
    t = t * RotateY(y);
  }
  if (z != 0.0f) {
    t = t * RotateZ(z);
  }
  return t;
}

bool Transform::operator==(Transform const& rhs) const noexcept {
  for (int i = 0; i < 16; ++i) {
    if (matrix[i] != rhs.matrix[i]) {
      return false;
    }
  }
  // The stub does not track operations; treat empty-equal-empty as true
  // and otherwise fall back to matrix-only equality. This is sufficient
  // for the test code paths (no test inspects operations).
  return true;
}

bool Transform::operator!=(Transform const& rhs) const noexcept {
  return !(*this == rhs);
}

Transform Transform::operator*(Transform const& rhs) const {
  // Short-circuit when the left operand is the identity matrix.
  Transform identity{};
  if (*this == identity) {
    return rhs;
  }

  // Production `Transform::operator*` returns the standard row-major
  // matrix product `lhs * rhs`, which for row-vector points `P` means
  // "apply lhs first, then rhs" (i.e. `P * lhs * rhs = (P * lhs) * rhs`).
  // This matches the production usage in `TransformAnimatedNode`:
  //   transform = transform * operation
  // where the accumulator applies operations left-to-right.
  Transform result{};
  auto const& lhs = *this;
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      Float sum = 0;
      for (int k = 0; k < 4; ++k) {
        sum += lhs.matrix[r * 4 + k] * rhs.matrix[k * 4 + c];
      }
      result.matrix[r * 4 + c] = sum;
    }
  }
  return result;
}

} // namespace facebook::react
