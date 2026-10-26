/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import type { ComponentManager } from './ComponentManager';
import type { Tag } from './DescriptorBase';
import type { RNOHLogger } from './RNOHLogger';

interface ComponentManagerRegistryEntry {
  componentManager: ComponentManager,
  refCounter: number,
}

/**
 * @actor RNOH_LIBRARY
 * 
 * @brief Stores ComponentManagers. Check ComponentManager documentation for
 * more information.
 */
export class ComponentManagerRegistry {
  private componentManagersByTag: Map<Tag, ComponentManager[]>;
  private entryByTag: Map<Tag, ComponentManagerRegistryEntry> = new Map();
  private logger: RNOHLogger;

  constructor(logger: RNOHLogger) {
    this.componentManagersByTag = new Map();
    this.logger = logger.clone('ComponentManagerRegistry');
  }

  /**
   * @brief Retrieves the ComponentManager associated with the specified tag.
   * First checks the reference-counted `entryByTag`.
   * If not found, falls back to the legacy `componentManagersByTag` list.
   * If multiple managers are associated with the same tag, a warning is
   * logged, and the last one in the list is returned.
   * 
   * @param tag Unique identifier for the component.
   * @returns The corresponding ComponentManager instance, or undefined if
   * not registered.
   */
  public getComponentManager(tag: Tag): ComponentManager | undefined {
    const entry = this.entryByTag.get(tag);
    if (entry) {
      return entry.componentManager;
    }
    const componentManagers = this.componentManagersByTag.get(tag);
    if (!componentManagers || componentManagers.length === 0) {
      return undefined;
    }
    if (componentManagers.length > 1) {
      this.logger.clone('getComponentManager')
        .warn(`Found ${componentManagers.length} component managers with the same tag (${tag})`);
    }
    return componentManagers[componentManagers.length - 1];
  }

  /**
   * @brief Registers a new ComponentManager for the specified tag.
   * Registers a ComponentManager for the specified tag.
   * This approach allows multiple managers for the same tag, which may
   * cause ambiguity and potential memory leaks.
   * @deprecated Use findOrCreateComponentManager, and
   * releaseComponentManager instead. (latestRNOHVersion: 0.72.40).
   *
   * NOTE: This method was actually deprecated in 0.72.27, but the removal
   * is postponed.
   * 
   * @param tag Tag to register
   * @param manager The ComponentManager to bind
   * @returns A cleanup function to unregister the ComponentManager (calls
   * manager.onDestroy)
   */
  public registerComponentManager(tag: Tag, manager: ComponentManager) {
    const componentManagers = this.componentManagersByTag.get(tag);
    if (!componentManagers) {
      this.componentManagersByTag.set(tag, []);
    }
    this.componentManagersByTag.get(tag)!.push(manager);

    return () => {
      const componentManagers = this.componentManagersByTag.get(tag);
      if (componentManagers) {
        manager.onDestroy();
        const filteredComponentManagers = componentManagers.filter(cm => cm !== manager);
        this.componentManagersByTag.set(tag, filteredComponentManagers);
        if (filteredComponentManagers.length === 0) {
          this.componentManagersByTag.delete(tag);
        }
      }
    };
  }

  /**
   * @brief Returns the existing `ComponentManager` or creates a new one for
   * the given tag.
   *
   * Each call to `findOrCreateComponentManager` must be matched 1:1
   * with a call to `releaseComponentManager` for the same `tag`
   * 
   * @param tag Unique tag identifying the component
   * @param createComponentManager Factory method to create a new
   * ComponentManager if needed
   * @returns An existing or newly created ComponentManager instance.
   */
  public findOrCreateComponentManager<TComponentManager extends ComponentManager>(
    tag: Tag,
    createComponentManager: () => TComponentManager
  ): TComponentManager {
    const entry = this.entryByTag.get(tag);
    if (entry !== undefined) {
      entry.refCounter += 1;
      return entry.componentManager as TComponentManager;
    }
    const componentManager = createComponentManager();
    this.entryByTag.set(tag, { componentManager, refCounter: 1 });
    return componentManager;
  }

  /**
   * @brief Releases a ComponentManager previously obtained from
   * findOrCreateComponentManager.
   *
   * This method must be called to properly release resources associated
   * with a ComponentManager.
   * It's crucial to maintain a 1:1 relationship between
   * findOrCreateComponentManager and releaseComponentManager calls for the
   * same tag.
   * 
   * @param tag The tag associated with the ComponentManager to release.
   */
  public releaseComponentManager(tag: Tag) {
    const entry = this.entryByTag.get(tag);
    if (entry === undefined) {
      this.logger.warn(`Tried to release already disposed component manager for tag: ${tag}`);
      return;
    }

    entry.refCounter -= 1;
    if (entry.refCounter === 0) {
      entry.componentManager.onDestroy();
      this.entryByTag.delete(tag);
    }
  }
  
  /**
   * @brief Returns all nodes, from root the component to the one with given tag.
   * 
   * @param tag The starting component tag
   * @returns An array of ComponentManagers, ordered from root to current.
   */
  public getComponentManagerLineage(tag: Tag): ComponentManager[] {
    const results: ComponentManager[] = [];
    let currentTag: Tag | undefined = tag;
    do {
      const componentManager = this.getComponentManager(currentTag);
      if (!componentManager) {
        // https://gl.swmansion.com/rnoh/react-native-harmony/-/issues/707
        break;
      }
      currentTag = componentManager.getParentTag();
      results.push(componentManager);
    } while (currentTag !== undefined);
    return results.reverse();
  }
}