/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import { Tag } from './DescriptorBase';

/**
 * @actor RNOH_LIBRARY
 * 
 * @brief Component(Instance)Manager. Unlike (ComponentInstance)Descriptor,
 * exposes behavior and encapsulates data.
 * Abstract base class representing a Component Manager.
 * A ComponentManager is responsible for managing the lifecycle, behavior,
 * and hierarchical relationships of a component instance at runtime.
 */
export abstract class ComponentManager {
  /**
   * @brief Lifecycle hook called when the component is destroyed.
   * Subclasses may override this method to release resources, unsubscribe
   * listeners, or perform other cleanup logic.
   */
  onDestroy() {}

  /**
   * @brief Returns the tag (ID) of the parent component.
   */
  abstract getParentTag(): Tag;

  /**
   * @brief Returns the tag (ID) associated with this component.
   */
  abstract getTag(): Tag;
}
