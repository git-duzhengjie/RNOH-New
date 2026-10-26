/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import { RNPackage } from './RNPackage';
import { ComponentBuilderContext } from './EtsRNOHContext';

/**
 * @actor RNOH_LIBRARY
 * Base class of packages, which provide custom native components
 */
export class RNOHPackage extends RNPackage {
  /**
   * Maps component names to their wrapped builders.
   * 
   * Override this method to register custom native components.
   * 
   * @returns map where:
   *   - key: Name of the custom component
   *   - value: WrappedBuilder that constructs the component with provided
   *     ComponentBuilderContext
   * 
   * RNOH calls the builders to create instances of corresponding native 
   * components.
   */
  createWrappedCustomRNComponentBuilderByComponentNameMap(): Map<string, WrappedBuilder<[ComponentBuilderContext]>> {
    return new Map();
  }
}