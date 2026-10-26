/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import { Descriptor, DescriptorWrapper, NativeId, Tag } from './DescriptorBase';
import { Mutation, MutationType } from './Mutation';
import type { RNInstanceImpl } from './RNInstance';
import type { RNOHLogger } from './RNOHLogger';

type RootDescriptor = Descriptor<'RootView', any>;

/**
 * @actor RNOH_LIBRARY
 * @brief Monitor structural changes under a specified node in the component tree,
 * including operations such as insertion, deletion, and update of child nodes.
 */
type SubtreeListener = () => void;

/**
 * @brief Set the state of a specific JS-side component to the native component.
 * @param componentName - The name of the native component to operate on.
 * @param tag - The unique identifier of the component.
 * @param state - The state object to be updated to the native side.
 */
type SetNativeStateFn = (componentName: string, tag: Tag, state: unknown) => void;

/**
 * @actor RNOH_LIBRARY
 * @brief A factory class used to uniformly create descriptorWrapper objects.
 * @param ctx - Descriptor object.
 */
export type DescriptorWrapperFactory = (ctx: { descriptor: Descriptor }) => DescriptorWrapper;

/**
 * @actor RNOH_LIBRARY
 * @brief Listening to component tree structure changes.
 * @param descriptor - Describes and manages the properties, structure, and behavior of component elements.
 * @param descriptorWrapper - Encapsulates and manages the lifecycle and access logic of descriptor objects.
 */
type DescriptorChangeListener = (descriptor: Descriptor, descriptorWrapper: DescriptorWrapper | null) => void;

/**
 * @actor RNOH_LIBRARY
 */
type InsertMutation = {
  descriptorMutationType: 'INSERT_CHILD',
  childIndex: number,
  childTag: Tag;
};

/**
 * @actor RNOH_LIBRARY
 */
type UpdateMutation = {
  descriptorMutationType: 'UPDATE_CHILD',
  childIndex: number,
  childTag: Tag;
};

/**
 * @actor RNOH_LIBRARY
 */
type RemoveMutation = {
  descriptorMutationType: 'REMOVE_CHILD',
  childIndex: number,
  childTag: Tag;
};

/**
 * @actor RNOH_LIBRARY
 */
export type DescriptorMutation = InsertMutation | UpdateMutation | RemoveMutation;

/**
 * @actor RNOH_LIBRARY
 * @brief Listening to mutation events such as node insertion, update, and 
 * removal in the component tree.
 */
export type DescriptorMutationListener = (args: DescriptorMutation) => void;

/**
 * @actor RNOH_LIBRARY
 * Stores (ComponentInstance)Descriptors. When Descriptors change, send a
 * notification.
 */
export class DescriptorRegistry {
  static readonly ANIMATED_NON_RAW_PROP_KEYS = ['transform'];
  private descriptorByTag: Map<Tag, Descriptor> = new Map();
  private descriptorWrapperByTag: Map<Tag, DescriptorWrapper> = new Map();
  private descriptorTagById: Map<NativeId, Tag> = new Map();
  private descriptorListenersSetByTag: Map<Tag, Set<DescriptorChangeListener>> = new Map();
  private descriptorMutationListenersByTag: Map<Tag, Set<DescriptorMutationListener>> = new Map();
  private subtreeListenersSetByTag: Map<Tag, Set<SubtreeListener>> = new Map();
  private animatedRawPropsByTag: Map<Tag, Set<string>> = new Map();
  private updatedUnnotifiedTags = new Set<Tag>();
  private cleanUpCallbacks: (() => void)[] = [];
  private logger: RNOHLogger;

  constructor(
    descriptorByTag: Record<Tag, Descriptor>,
    private setNativeStateFn: SetNativeStateFn,
    private rnInstance: RNInstanceImpl,
    private descriptorWrapperFactoryByDescriptorType: Map<string, DescriptorWrapperFactory>,
    logger: RNOHLogger,
  ) {
    this.logger = logger.clone('DescriptorRegistry');
    const stopTracing = this.logger.clone('constructor').startTracing();
    for (const tag in descriptorByTag) {
      this.descriptorByTag.set(parseInt(tag), descriptorByTag[tag]);
      this.descriptorWrapperByTag.set(parseInt(tag), new DescriptorWrapper(descriptorByTag[tag]));
    }
    this.cleanUpCallbacks.push(this.rnInstance.subscribeToLifecycleEvents('FOREGROUND', () => {
      this.callListeners(this.updatedUnnotifiedTags);
      this.updatedUnnotifiedTags.clear();
    }));
    stopTracing();
  }

  private saveDescriptor(descriptor: Descriptor) {
    const previousRenderKey = this.descriptorByTag.get(descriptor.tag)?.renderKey;
    descriptor.renderKey = previousRenderKey !== undefined ? (previousRenderKey + 1) : 0;
    this.descriptorByTag.set(descriptor.tag, descriptor);
    const createDescriptorWrapper = this.descriptorWrapperFactoryByDescriptorType.get(descriptor.type);
    let descriptorWrapper = new DescriptorWrapper(descriptor);
    if (createDescriptorWrapper) {
      descriptorWrapper = createDescriptorWrapper({ descriptor });
    }
    this.descriptorWrapperByTag.set(descriptor.tag, descriptorWrapper);
    const id = descriptorWrapper.id;
    this.descriptorTagById.set(id, descriptor.tag);
    if (descriptorWrapper.hints.includes('debug')) {
      this.logger.clone(`id:${id}`).debug(JSON.stringify(descriptor, null, 2));
    }
  }

  private deleteDescriptor(tag: Tag) {
    const descriptorWrapper = this.descriptorWrapperByTag.get(tag);
    if (descriptorWrapper) {
      this.descriptorByTag.delete(descriptorWrapper.tag);
      if (descriptorWrapper.id) {
        this.descriptorTagById.delete(descriptorWrapper.id);
      }
      this.descriptorWrapperByTag.delete(tag);
    }
  }

  /**
   * @brief Call it when destroying, and clear the callback function
   * @internal
   */
  public onDestroy() {
    const stopTracing = this.logger.clone('destroy').startTracing();
    this.cleanUpCallbacks.forEach(cb => cb());
    stopTracing();
  }

  /**
   * @brief Get descriptor based on the tag value
   * @param { number } tag - Node tag
   * @return descriptor It includes type, parentTag, props, state, and so
   * on, which are all related to tags
   */
  public getDescriptor<TDescriptor extends Descriptor>(tag: Tag): TDescriptor {
    return this.descriptorByTag.get(tag) as TDescriptor;
  }

  /**
   * @brief Get DescriptorWrapper according to the tag value
   * @param { number } tag - Node tag
   * @return descriptorWrapper
   */
  public findDescriptorWrapperByTag<TDescriptorWrapper extends DescriptorWrapper>(tag: Tag): TDescriptorWrapper | null {
    return this.descriptorWrapperByTag.get(tag) as TDescriptorWrapper | null;
  }
  /**
   * @brief Get Descriptor according to the id value
   * @param { string } id - NativeId
   * @return descriptor || null
   */
  public findDescriptorById<TDescriptor extends Descriptor>(id: NativeId): TDescriptor | null {
    const tag = this.descriptorTagById.get(id);
    if (tag) {
      return this.descriptorByTag.get(tag) as (TDescriptor | null);
    }
    return null;
  }

  /**
   * @brief Tree of descriptors represent data we have received from RN. To get
   * representation of current state of UI use getComponentManagerLineage
   * @param { number } tag - Node tag
   * @return [...ancestors, descriptor]
   */
  public getDescriptorLineage(tag: Tag): Descriptor[] {
    const results: Descriptor[] = [];
    let currentTag: Tag | undefined = tag;
    do {
      let descriptor = this.getDescriptor(currentTag);
      currentTag = descriptor.parentTag;
      results.push(descriptor);
    } while (currentTag !== undefined);
    return results.reverse();
  }

  private splitAnimatedProps(animatedProps: Object): [Object, Object] {
    const props = {};
    const rawProps = {};

    Object.entries(animatedProps).forEach(([key, value]) => {
      if (DescriptorRegistry.ANIMATED_NON_RAW_PROP_KEYS.includes(key)) {
        props[key] = value;
      } else {
        rawProps[key] = value;
      }
    });

    return [props, rawProps];
  }

  /**
   * @brief Called by NativeAnimatedTurboModule. This method needs to be
   * encapsulated
   * @param { number } tag - Node tag
   * @param { TProps } newProps - The new properties of the animation
   */
  public setAnimatedRawProps<TProps extends Object>(tag: Tag, newProps: TProps): void {
    this.logger.clone('setAnimatedRawProps').debug('');
    let descriptor = this.getDescriptor<Descriptor<string, TProps>>(tag);

    if (!descriptor) {
      return;
    }

    // update stored animated props
    const oldProps = this.animatedRawPropsByTag.get(tag);
    const mergedProps = { ...oldProps, ...newProps };
    this.animatedRawPropsByTag.set(tag, mergedProps);

    const [props, rawProps] = this.splitAnimatedProps(mergedProps);

    // set new props for the descriptor
    descriptor.props = { ...descriptor.props, ...props };
    descriptor.rawProps = { ...descriptor.rawProps, ...rawProps };
    const updatedDescriptor = { ...descriptor };
    this.saveDescriptor(updatedDescriptor);

    this.descriptorListenersSetByTag.get(tag)?.forEach(cb => cb(updatedDescriptor, this.findDescriptorWrapperByTag(tag)));
    if (updatedDescriptor.parentTag) {
      const listeners = this.descriptorMutationListenersByTag.get(updatedDescriptor.parentTag);
      const parentDescriptor = this.getDescriptor(updatedDescriptor.parentTag);
      const childIndex = parentDescriptor?.childrenTags.findIndex((childTag) => childTag === tag);
      if (childIndex >= 0) {
        listeners?.forEach(listener => listener({ descriptorMutationType: 'UPDATE_CHILD', childIndex, childTag: tag }));
      }
    }
    this.callSubtreeListeners(new Set([tag]));
  }
  /**
   * @brief Call the native method to set the State value
   * @param { number } tag - Node tag
   * @param { TState } state - The state of the tag has changed
   */
  public setState<TState extends Object>(tag: Tag, state: TState): void {
    const stopTracing = this.logger.clone('setState').startTracing();
    let descriptor = this.getDescriptor<Descriptor<string, TState>>(tag);
    if (!descriptor) {
      return;
    }

    this.setNativeStateFn(descriptor.type, tag, state);
    stopTracing();
  }

  /**
   * @brief The changes of applying mutation
   * @param { Array } mutations - Instructions for node changes
   * @internal
   */
  public applyMutations(mutations: Mutation[]) {
    const updatedDescriptorTags = new Set(mutations.flatMap(mutation => {
      return this.applyMutation(mutation);
    }
    ));
    if (!this.rnInstance.shouldUIBeUpdated()) {
      updatedDescriptorTags.forEach(tag => this.updatedUnnotifiedTags.add(tag));
      return;
    }
    this.callListeners(updatedDescriptorTags);
  }

  private callListeners(tags: Set<Tag>): void {
    tags.forEach(tag => {
      const updatedDescriptor = this.getDescriptor(tag);
      if (!updatedDescriptor) {
        return;
      }
      this.descriptorListenersSetByTag.get(tag)?.forEach(cb => {
        cb(updatedDescriptor, this.findDescriptorWrapperByTag(tag));
      });
    });
    this.callSubtreeListeners(tags);
  }

  private callSubtreeListeners(updatedDescriptorTags: Set<Tag>) {
    const setOfSubtreeListenersToCall = new Set<SubtreeListener>();
    for (const tag of updatedDescriptorTags) {
      let descriptor = this.descriptorByTag.get(tag);
      while (descriptor) {
        const listeners = this.subtreeListenersSetByTag.get(descriptor.tag);
        if (listeners) {
          for (const listener of listeners) {
            setOfSubtreeListenersToCall.add(listener);
          }
        }
        if (descriptor.parentTag) {
          descriptor = this.descriptorByTag.get(descriptor.parentTag);
        } else {
          break;
        }
      }
    }
    setOfSubtreeListenersToCall.forEach(listener => {
      listener();
    });
  }

  /**
   * @brief Subscribe to the changes of mutations
   * @param { number } tag - Node tag
   * @param { DescriptorMutationListener } listener - The function that listens to changes in mutations
   * @internal
   */
  public subscribeToDescriptorMutations(tag: Tag, listener: DescriptorMutationListener) {
    if (!this.descriptorMutationListenersByTag.has(tag)) {
      this.descriptorMutationListenersByTag.set(tag, new Set());
    }
    this.descriptorMutationListenersByTag.get(tag)!.add(listener);
    return () => {
      const listeners = this.descriptorMutationListenersByTag.get(tag);
      listeners?.delete(listener);
      if (listeners?.size === 0) {
        this.descriptorMutationListenersByTag.delete(tag);
      }
    };
  }

  /**
   * @brief Subscribe to the changes of Descriptor
   * @param { number } tag - Node tag
   * @param { DescriptorChangeListener } listener - listener function for changes in Descriptor.
   */
  public subscribeToDescriptorChanges(
    tag: Tag,
    listener: DescriptorChangeListener,
  ) {
    if (!this.descriptorListenersSetByTag.has(tag)) {
      this.descriptorListenersSetByTag.set(tag, new Set());
    }
    this.descriptorListenersSetByTag.get(tag)!.add(listener);
    return () => {
      this.removeDescriptorChangesListener(tag, listener);
    };
  }

  private removeDescriptorChangesListener(
    tag: Tag,
    listener: DescriptorChangeListener,
  ) {
    const callbacksSet = this.descriptorListenersSetByTag.get(tag);
    callbacksSet?.delete(listener);
    if (callbacksSet?.size === 0) {
      this.descriptorListenersSetByTag.delete(tag);
    }
  }

  /**
   * @brief Subscribe to the changes of the Descriptor of the subtree
   * @param { number } tag - Node tag
   * @param { SubtreeListener } listener - The listener function of the subtree
   */
  public subscribeToDescriptorSubtreeChanges(
    rootTag: Tag,
    listener: SubtreeListener,
  ) {
    if (!this.subtreeListenersSetByTag.has(rootTag)) {
      this.subtreeListenersSetByTag.set(rootTag, new Set());
    }
    this.subtreeListenersSetByTag.get(rootTag)!.add(listener);

    return () => {
      const callbacksSet = this.descriptorListenersSetByTag.get(rootTag);
      callbacksSet?.delete(listener);
      if (callbacksSet?.size === 0) {
        this.descriptorListenersSetByTag.delete(rootTag);
      }
    };
  }

  private applyMutation(mutation: Mutation): Tag[] {
    if (mutation.type === MutationType.CREATE) {
      this.saveDescriptor(this.maybeOverwriteProps(mutation.descriptor));
      return [];
    } else if (mutation.type === MutationType.INSERT) {
      const childDescriptor = this.descriptorByTag.get(mutation.childTag);
      if (childDescriptor) {
        childDescriptor.parentTag = mutation.parentTag;
      }
      this.descriptorByTag.get(mutation.parentTag)?.childrenTags.splice(
        mutation.index,
        0,
        mutation.childTag,
      );
      this.descriptorMutationListenersByTag.get(mutation.parentTag)?.forEach((cb) => cb({
        descriptorMutationType: 'INSERT_CHILD',
        childIndex: mutation.index,
        childTag: mutation.childTag
      }));
      return [mutation.childTag, mutation.parentTag];
    } else if (mutation.type === MutationType.UPDATE) {
      const currentDescriptor = this.descriptorByTag.get(mutation.descriptor.tag);
      if (!currentDescriptor) {
        this.logger.warn('UPDATE mutation for unknown tag=' + mutation.descriptor.tag + ' (type=' + mutation.descriptor.type + '), skipping');
        return [];
      }
      const children = currentDescriptor.childrenTags;
      const mutationDescriptor = this.maybeOverwriteProps(mutation.descriptor);
      const animatedProps = this.animatedRawPropsByTag.get(mutation.descriptor.tag);

      const newDescriptor = {
        ...currentDescriptor,
        ...mutation.descriptor,
        // NOTE: animated props override the ones from the mutation
        props: { ...currentDescriptor.props, ...mutationDescriptor.props, ...animatedProps },
        rawProps: { ...currentDescriptor.rawProps, ...mutationDescriptor.rawProps, ...animatedProps },
        state: { ...currentDescriptor.state, ...mutationDescriptor.state },
        childrenTags: children,
      };
      this.saveDescriptor(newDescriptor);

      if (currentDescriptor?.parentTag) {
        const parentDescriptor = this.descriptorByTag.get(currentDescriptor.parentTag ?? 0);
        const childIndex = parentDescriptor?.childrenTags.indexOf(currentDescriptor.tag);
        if (parentDescriptor && childIndex >= 0) {
          this.descriptorMutationListenersByTag.get(currentDescriptor.parentTag)?.forEach((cb) => cb({
            descriptorMutationType: 'UPDATE_CHILD',
            childTag: currentDescriptor.tag,
            childIndex
          }));
        }
      }

      return [mutation.descriptor.tag];
    } else if (mutation.type === MutationType.REMOVE) {
      const parentDescriptor = this.descriptorByTag.get(mutation.parentTag);
      const childDescriptor = this.descriptorByTag.get(mutation.childTag);
      const idx = parentDescriptor?.childrenTags.indexOf(mutation.childTag);
      this.descriptorMutationListenersByTag.get(mutation.parentTag)?.forEach((cb) => cb({
        descriptorMutationType: 'REMOVE_CHILD',
        childTag: mutation.childTag,
        childIndex: idx
      }));
      if (parentDescriptor && idx !== -1) {
        parentDescriptor.childrenTags.splice(idx, 1);
      }
      if (childDescriptor) {
        childDescriptor.parentTag = undefined;
      }
      return [mutation.parentTag];
    } else if (mutation.type === MutationType.DELETE) {
      const currentDescriptor = this.descriptorByTag.get(mutation.tag);
      if (currentDescriptor?.parentTag) {
        const parentDescriptor = this.descriptorByTag.get(currentDescriptor.parentTag ?? 0);
        const childIndex = parentDescriptor?.childrenTags.indexOf(currentDescriptor.tag);
        if (childIndex !== -1) {
          parentDescriptor?.childrenTags.splice(childIndex, 1);
        }
      }
      this.deleteDescriptor(mutation.tag);
      this.animatedRawPropsByTag.delete(mutation.tag);
      return currentDescriptor?.parentTag ? [currentDescriptor.parentTag] : [];
    } else if (mutation.type === MutationType.REMOVE_DELETE_TREE) {
      return [];
    }
    return [];
  }

  /**
   * @deprecated: It was deprecated when preparing 0.77 branch for release.
   */
  private maybeOverwriteProps(descriptor: Descriptor) {
    const props = descriptor.isDynamicBinder ? descriptor.rawProps : descriptor.props;
    return { ...descriptor, props };
  }

  /**
   * @brief Create Descriptor for the root node
   * @param { number } tag - Node tag
   * @internal
   */
  public createRootDescriptor(tag: Tag) {
    const rootDescriptor: RootDescriptor = {
      isDynamicBinder: false,
      type: 'RootView',
      tag,
      childrenTags: [],
      props: { top: 0, left: 0, width: 0, height: 0 },
      state: {},
      rawProps: {},
      layoutMetrics: {
        frame: {
          origin: {
            x: 0,
            y: 0,
          },
          size: {
            width: 0,
            height: 0,
          }
        }
      }
    };
    this.saveDescriptor(rootDescriptor);
  }

  /**
   * @brief Delete Descriptor for the root node
   * @param { number } tag - Node tag
   * @internal
   */
  public deleteRootDescriptor(tag: Tag) {
    const descriptor = this.getDescriptor(tag);
    if (descriptor?.type !== 'RootView') {
      return;
    }
    // delay deleting the root descriptor until the mutation
    // removing all its children has been applied
    if (descriptor.childrenTags.length === 0) {
      this.descriptorByTag.delete(tag);
      return;
    }
    const unsubscribe = this.subscribeToDescriptorChanges(tag, (newDescriptor) => {
      if (newDescriptor.childrenTags.length === 0) {
        unsubscribe();
        this.descriptorByTag.delete(tag);
      }
    });
  }

  /**
   * @deprecated: Use other methods from this class instead.
   * It was deprecated when preparing 0.77 branch for release.
   */
  public getDescriptorByTagMap() {
    return this.descriptorByTag;
  }

  /**
   * @brief Get the stats of Descriptor
   * @return stats include Descriptor, the proportion of quantity, and so on
   * @internal
   */
  public getStats() {
    const stats = new DescriptorRegistryStats();
    for (const descriptor of this.descriptorByTag.values()) {
      if (!stats.countByDescriptorType.has(descriptor.type)) {
        stats.countByDescriptorType.set(descriptor.type, 0);
      }
      const currentCount = stats.countByDescriptorType.get(descriptor.type);
      stats.countByDescriptorType.set(descriptor.type, currentCount + 1);
    }
    return stats;
  }
}

/**
 * @actor RNOH_LIBRARY
 * @brief Count and manage the quantity and proportion of different types of descriptors
 * in the component tree.
 */
export class DescriptorRegistryStats {
  countByDescriptorType = new Map<string, number>();

  /**
   * @brief Returns the total number of all descriptors.
   */
  get totalDescriptorsCount() {
    return Array.from(this.countByDescriptorType.values()).reduce((acc, val) => (acc + val), 0);
  }

  /**
   * @brief Calculates the percentage share of each descriptor type in the total count.
   */
  get shareInPctByDescriptorType() {
    const result = new Map<string, number>();
    this.countByDescriptorType.forEach((count, descriptorType) => {
      result.set(descriptorType, count / (this.totalDescriptorsCount || 1));
    });
    return result;
  }

  /**
   * @brief Generates a string for debugging purposes.
   */
  toDebugString() {
    const lines: string[] = [];
    lines.push(`TOTAL ${this.totalDescriptorsCount}`);
    sortMapByValue(this.shareInPctByDescriptorType, (a, b) => b - a).forEach((shareInPct, descriptorType) => {
      lines.push(`${descriptorType} ${Math.round(shareInPct * 100)}%`);
    });
    return lines.join('\n');
  }
}

function sortMapByValue<K, V>(map: Map<K, V>, compareFn?: (a: V, b: V) => number): Map<K, V> {
  const mapEntries = Array.from(map.entries());
  mapEntries.sort((a, b) => {
    if (compareFn) {
      return compareFn(a[1], b[1]);
    } else {
      if (a[1] > b[1]) {
        return 1;
      } else if (a[1] < b[1]) {
        return -1;
      } else {
        return 0;
      }
    }
  });
  return new Map<K, V>(mapEntries);
}