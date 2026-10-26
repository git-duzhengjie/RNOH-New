/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import type { BorderMetrics, ColorSegments, ColorValue } from './DescriptorBase';

/**
 * Represents the visual state of a React Native surface. Controls how visual
 * effects are handled.
 * 
 * @actor RNOH_APP
 * @enum {number}
 */
export enum DisplayMode {
  /*
   * The surface is running normally. All visual side-effects will be rendered
   * on the screen.
   */
  Visible = 0,

  /*
   * The surface is `Suspended`. All new (committed after switching to the
   * mode) visual side-effects will *not* be mounted on the screen (the screen
   * will stop updating).
   *
   * The mode can be used for preparing a surface for possible future use.
   * The surface will be prepared without spending computing resources
   * on mounting, and then can be instantly mounted if needed.
   */
  Suspended = 1,

  /*
   * The surface is `Hidden`. All previously mounted visual side-effects
   * will be unmounted, and all new (committed after switching to the mode)
   * visual side-effects will *not* be mounted on the screen until the mode is
   * switched back to `normal`.
   *
   * The mode can be used for temporarily freeing computing resources of
   * off-the-screen surfaces.
   */
  Hidden = 2,
}

/**
 * Converts normalized color segments to RGBA string representation.
 * @note Input values are expected to be in 0-1 range
 * 
 * @actor RNOH_LIBRARY
 * @param {ColorSegments} colorSegments - Array of [r,g,b,a] in [0.0, 1.0] range
 * @returns {string|undefined} Format: "rgba(r,g,b,a)" or undefined if no input
 */
export function convertColorSegmentsToString(colorSegments?: ColorSegments) {
  if (!colorSegments) {
    return undefined;
  }
  const [r, g, b, a] = colorSegments;
  return `rgba(${Math.round(r * 255)}, ${Math.round(g * 255)}, ${Math.round(
    b * 255
  )}, ${a})`;
}

/**
 * Creates a color transformation matrix for applying tint effects.
 * 
 * @actor RNOH_LIBRARY
 * @param {ColorSegments} colorSegments - Array of [r,g,b,a] in 0-1 range
 * @returns {number[]} 4x5 matrix for color transformation
 */
export function getTintColorMatrix(colorSegments?: ColorSegments) {
  if (!colorSegments || colorSegments.every((element) => element === 0)) {
    return [
      1, 0, 0, 0, 0,
      0, 1, 0, 0, 0,
      0, 0, 1, 0, 0,
      0, 0, 0, 1, 0,
    ];
  }
  const [r, g, b, a] = colorSegments;
  return [
    0, 0, 0, r, 0,
    0, 0, 0, g, 0,
    0, 0, 0, b, 0,
    0, 0, 0, 1, 0,
  ];
}

/**
 * 0-255
 */
type ColorChannel = number;

/**
 * Manages color values and provides conversion between different formats.
 * 
 * @actor RNOH_LIBRARY
 */
export class Color {
  /**
   * Creates Color from numeric color value.
   *
   * @param {ColorValue} colorValue - ARGB color as number
   * @returns {Color} New Color instance
   */
  static fromColorValue(colorValue: ColorValue) {
    return Color.fromColorSegments(convertColorValueToColorSegments(colorValue));
  }

  /**
   * Creates Color from normalized color segments.
   *
   * @param {ColorSegments} [r,g,b,a] - Color values in 0-1 range
   * @returns {Color} New Color instance
   */
  static fromColorSegments([r, g, b, a]: ColorSegments) {
    return new Color({
      r: r * 255,
      g: g * 255,
      b: b * 255,
      a: a * 255,
    });
  }

  constructor(private rgba: {
    r: ColorChannel,
    g: ColorChannel,
    b: ColorChannel,
    a: ColorChannel;
  }) {
  }

  toRGBAString() {
    return `rgba(${this.rgba.r}, ${this.rgba.g}, ${this.rgba.b}, ${this.rgba.a / 255})`;
  }

  /**
   * Converts to normalized color segments.
   *
   * @returns {ColorSegments} Array of [r,g,b,a] values in 0-1 range
   */
  toSegments(): ColorSegments {
    return [
      this.rgba.r / 255,
      this.rgba.g / 255,
      this.rgba.b / 255,
      this.rgba.a / 255,
    ];
  }
}

/**
 * Converts numeric ARGB color value to RGBA string format.
 *
 * @actor RNOH_LIBRARY
 * @param {ColorValue} colorValue - ARGB color as number
 * @param {string} defaultColor - Color to use if input undefined
 * @returns {string} Format: "rgba(r,g,b,a)" with rgb in 0-255, a in 0-1
 */
export function convertColorValueToRGBA(
  colorValue?: ColorValue,
  defaultColor: string = 'rgba(0, 0, 0, 0.0)',
) {
  if (colorValue === undefined) {
    return defaultColor;
  }
  const rgba = {
    a: ((colorValue >> 24) & 0xff) / 255,
    r: (colorValue >> 16) & 0xff,
    g: (colorValue >> 8) & 0xff,
    b: (colorValue >> 0) & 0xff,
  };
  return `rgba(${rgba.r}, ${rgba.g}, ${rgba.b}, ${rgba.a})`;
}

/**
 * Converts numeric ARGB color value to 8-digit hex string.
 *
 * @actor RNOH_LIBRARY
 * @param {ColorValue} colorValue - ARGB color as number
 * @param {string} defaultColor - Color to use if input undefined
 * @returns {string} Format: "#AARRGGBB" where each component is 2 hex digits
 */
export function convertColorValueToHex(
  colorValue?: ColorValue,
  defaultColor: string = '#00000000',
) {
  if (colorValue === undefined) {
    return defaultColor;
  }
  const toHex = (num, padding) => num.toString(16).padStart(padding, '0');
  const argb = {
    a: (colorValue >> 24) & 0xff,
    r: (colorValue >> 16) & 0xff,
    g: (colorValue >> 8) & 0xff,
    b: (colorValue >> 0) & 0xff,
  };
  return `#${toHex(argb.a, 2)}${toHex(argb.r, 2)}${toHex(argb.g, 2)}${toHex(argb.b, 2)}`;
}

/**
 * Converts numeric ARGB color value to normalized color segments.
 *
 * @actor RNOH_LIBRARY
 * @param {ColorValue} colorValue - ARGB color as number
 * @returns {ColorSegments|undefined} Array of [r,g,b,a] in 0-1 range or undefined
 */
export function convertColorValueToColorSegments(colorValue?: ColorValue): ColorSegments | undefined {
  if (colorValue === undefined) {
    return undefined;
  }
  const rgba = {
    a: ((colorValue >> 24) & 0xff) / 255,
    r: ((colorValue >> 16) & 0xff) / 255,
    g: ((colorValue >> 8) & 0xff) / 255,
    b: ((colorValue >> 0) & 0xff) / 255,
  };
  return [rgba.r, rgba.g, rgba.b, rgba.a];
}

/**
 * A flattened array that represents a 4x4 3D geometric transformation matrix.
 * Used for geometric transformations in 3D space.
 * Matrix elements are stored in row-major order.
 * 
 * @actor RNOH_LIBRARY
 */
export type TransformMatrix = [
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number
];

/**
 * Immutable version of TransformMatrix.
 * Matrix elements are stored in row-major order.
 * 
 * @actor RNOH_LIBRARY
 */
export type ReadonlyTransformationMatrix = readonly [
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number
];

/**
 * @actor RNOH_LIBRARY
 * @deprecated Use ViewDescriptorWrapperBase::resolveEdges instead.
 * It was deprecated when preparing 0.77 branch for release.
 */
export enum BorderEdgePropsType {
  COLOR = 'Color',
  WIDTH = 'Width',
}

/**
 * @actor RNOH_LIBRARY
 * @deprecated Use ViewDescriptorWrapperBase::resolveEdges and ViewDescriptorWrapperBase::resolveCorners instead.
 * It was deprecated when preparing 0.77 branch for release.
 */
export function resolveBorderMetrics(props: BorderMetrics, isRTL: boolean): BorderMetrics {
  const colorProps = resolveBorderEdgeProps(props, BorderEdgePropsType.COLOR, isRTL);
  const widthProps = resolveBorderEdgeProps(props, BorderEdgePropsType.WIDTH, isRTL);
  const radiusProps = resolveBorderRadius(props);
  return { ...colorProps, ...widthProps, ...radiusProps, borderStyle: props.borderStyle };
}

/**
 * @actor RNOH_LIBRARY
 * @deprecated Use ViewDescriptorWrapperBase::resolveCorners instead.
 * It was deprecated when preparing 0.77 branch for release.
 */
export function resolveBorderRadius(props: BorderMetrics): BorderMetrics {
  const topLeft = props.borderTopLeftRadius;
  const topRight = props.borderTopRightRadius;
  const bottomLeft = props.borderBottomLeftRadius;
  const bottomRight = props.borderBottomRightRadius;
  const all = props.borderRadius;
  const resolvedProps = {
    borderTopLeftRadius: topLeft ?? all,
    borderTopRightRadius: topRight ?? all,
    borderBottomLeftRadius: bottomLeft ?? all,
    borderBottomRightRadius: bottomRight ?? all,
  };
  return resolvedProps;
}

/**
 * @actor RNOH_LIBRARY
 * @deprecated Use ViewDescriptorWrapperBase::resolveEdges instead.
 * It was deprecated when preparing 0.77 branch for release.
 */
export function resolveBorderEdgeProps(props: BorderMetrics, type: BorderEdgePropsType,
  isRTL: boolean): BorderMetrics {
  const left = props[`borderLeft${type}`];
  const top = props[`borderTop${type}`];
  const right = props[`borderRight${type}`];
  const bottom = props[`borderBottom${type}`];
  const all = props[`border${type}`];
  const start = props[`borderStart${type}`];
  const end = props[`borderEnd${type}`];

  const resolvedProps = {
    [`borderLeft${type}`]: left ?? ((isRTL ? end : start) ?? all),
    [`borderTop${type}`]: top ?? all,
    [`borderRight${type}`]: right ?? ((isRTL ? start : end) ?? all),
    [`borderBottom${type}`]: bottom ?? all,
  };

  return resolvedProps;
}

/**
 * Transform a 4D vector by a 4x4 transformation matrix.
 * 
 * @actor RNOH_LIBRARY
 * @param {ReadonlyTransformationMatrix} transformMatrix - 4x4 matrix in row-major order
 * @param {Array<number>} vector - Input 4D vector [x, y, z, w], treated as a
 * 4D column vector
 * @returns {Array<number>} Transformed 4D vector [x', y', z', w']
 * @example
 * // Identity transform
 * const matrix = [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1]
 * const vector = [1,2,3,1]
 * getTransformedVector(matrix, vector) // Returns [1,2,3,1]
 */
export function getTransformedVector(transformMatrix: ReadonlyTransformationMatrix,
  vector: Array<number>): Array<number> {
  const resultVector = [0, 0, 0, 0];
  for (let i = 0; i < 4; ++i) {
    for (let j = 0; j < 4; ++j) {
      resultVector[i] += transformMatrix[i * 4 + j] * vector[j];
    }
  }
  return resultVector;
}