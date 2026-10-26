/**
 * RNOH-New: AnimatedTurboModule - Native Animated native module
 *
 * Implements React Native's NativeAnimatedModule interface using @ohos.animator.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../../RNH/RNHTurboModule';
import { RNHLogger } from '../../RNH/RNHLogger';
import animator, { AnimatorOptions, AnimatorResult } from '@ohos.animator';

const TAG = 'AnimatedTurboModule';

/**
 * Animated node config from JS
 */
interface AnimatedNodeConfig {
  type: string;
  value?: number;
  offset?: number;
  input?: number[];
  output?: number[];
  extrapolateLeft?: string;
  extrapolateRight?: string;
  toValue?: number;
}

/**
 * Animation config
 */
interface AnimationConfig {
  type: string;
  duration?: number;
  delay?: number;
  easing?: string;
  iterations?: number;
  toValue?: number;
  platformConfig?: Record<string, Object>;
}

/**
 * Animated node value listener
 */
type AnimatedNodeValueListener = (value: number) => void;

/**
 * Animated node abstraction
 */
class AnimatedNode {
  tag: number;
  config: AnimatedNodeConfig;
  value: number = 0;
  offset: number = 0;
  children: Set<number> = new Set();
  valueListeners: Set<AnimatedNodeValueListener> = new Set();

  constructor(tag: number, config: AnimatedNodeConfig) {
    this.tag = tag;
    this.config = config;
    this.value = config.value ?? 0;
    this.offset = config.offset ?? 0;
  }

  getValue(): number {
    return this.value + this.offset;
  }

  setValue(value: number): void {
    this.value = value;
    this.notifyListeners();
  }

  setOffset(offset: number): void {
    this.offset = offset;
  }

  flattenOffset(): void {
    this.value += this.offset;
    this.offset = 0;
  }

  extractOffset(): void {
    this.offset += this.value;
    this.value = 0;
  }

  addListener(listener: AnimatedNodeValueListener): void {
    this.valueListeners.add(listener);
  }

  removeListener(listener: AnimatedNodeValueListener): void {
    this.valueListeners.delete(listener);
  }

  notifyListeners(): void {
    const currentValue = this.getValue();
    for (const listener of this.valueListeners) {
      try {
        listener(currentValue);
      } catch (e) {
        RNHLogger.error(TAG, `Error notifying animated node listener: ${e}`);
      }
    }
  }
}

/**
 * NativeAnimatedModule TurboModule
 */
export class AnimatedTurboModule extends RNHTurboModule {
  private nodes: Map<number, AnimatedNode> = new Map();
  private animations: Map<number, AnimatorResult> = new Map();

  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'NativeAnimatedModule';
  }

  /**
   * Create an animated node
   */
  createAnimatedNode(tag: number, config: AnimatedNodeConfig): void {
    RNHLogger.debug(TAG, `createAnimatedNode: ${tag}, type: ${config.type}`);
    this.nodes.set(tag, new AnimatedNode(tag, config));
  }

  /**
   * Connect two animated nodes (parent -> child)
   */
  connectAnimatedNodes(parentTag: number, childTag: number): void {
    RNHLogger.debug(TAG, `connectAnimatedNodes: ${parentTag} -> ${childTag}`);
    const parent = this.nodes.get(parentTag);
    if (parent) {
      parent.children.add(childTag);
    }
  }

  /**
   * Disconnect two animated nodes
   */
  disconnectAnimatedNodes(parentTag: number, childTag: number): void {
    RNHLogger.debug(TAG, `disconnectAnimatedNodes: ${parentTag} -> ${childTag}`);
    const parent = this.nodes.get(parentTag);
    if (parent) {
      parent.children.delete(childTag);
    }
  }

  /**
   * Start animating a node
   */
  startAnimatingNode(animationId: number, nodeTag: number,
                     config: AnimationConfig, endCallbackId: number): void {
    RNHLogger.info(TAG, `startAnimatingNode: animationId=${animationId}, nodeTag=${nodeTag}`);
    const node = this.nodes.get(nodeTag);
    if (!node) {
      RNHLogger.warn(TAG, `Node ${nodeTag} not found for animation`);
      return;
    }

    const duration = config.duration ?? 300;
    const delay = config.delay ?? 0;
    const iterations = config.iterations ?? 1;
    const startValue = node.getValue();
    const toValue = this.resolveToValue(node, config);

    const options: AnimatorOptions = {
      duration: duration,
      delay: delay,
      iterations: iterations,
      easing: this.mapEasing(config.easing),
      begin: startValue,
      end: toValue,
      fill: 'forwards',
      direction: 'normal',
    };

    const animation = animator.create(options);

    animation.onFrame = (value: number) => {
      node.setValue(value);
      this.propagateValueToChildren(nodeTag, value);
    };

    animation.onFinish = () => {
      RNHLogger.debug(TAG, `Animation ${animationId} finished`);
      this.emitEvent('onAnimatedValueUpdate', JSON.stringify({ tag: nodeTag, value: node.getValue() }));
      this.emitEvent('onAnimatedAnimationEnd', JSON.stringify({ animationId, endCallbackId, finished: true }));
      this.animations.delete(animationId);
    };

    this.animations.set(animationId, animation);
    animation.play();
  }

  /**
   * Stop an animation
   */
  stopAnimation(animationId: number): void {
    RNHLogger.info(TAG, `stopAnimation: ${animationId}`);
    const animation = this.animations.get(animationId);
    if (animation) {
      animation.pause();
      this.animations.delete(animationId);
    }
  }

  /**
   * Set animated node value directly
   */
  setAnimatedNodeValue(tag: number, value: number): void {
    RNHLogger.debug(TAG, `setAnimatedNodeValue: ${tag} = ${value}`);
    const node = this.nodes.get(tag);
    if (node) {
      node.setValue(value);
      this.propagateValueToChildren(tag, value);
      this.emitEvent('onAnimatedValueUpdate', JSON.stringify({ tag, value: node.getValue() }));
    }
  }

  /**
   * Set animated node offset
   */
  setAnimatedNodeOffset(tag: number, offset: number): void {
    RNHLogger.debug(TAG, `setAnimatedNodeOffset: ${tag} = ${offset}`);
    const node = this.nodes.get(tag);
    if (node) {
      node.setOffset(offset);
    }
  }

  /**
   * Flatten offset into value
   */
  flattenAnimatedNodeOffset(tag: number): void {
    RNHLogger.debug(TAG, `flattenAnimatedNodeOffset: ${tag}`);
    const node = this.nodes.get(tag);
    if (node) {
      node.flattenOffset();
    }
  }

  /**
   * Extract offset from value
   */
  extractAnimatedNodeOffset(tag: number): void {
    RNHLogger.debug(TAG, `extractAnimatedNodeOffset: ${tag}`);
    const node = this.nodes.get(tag);
    if (node) {
      node.extractOffset();
    }
  }

  /**
   * Start listening to node value updates
   */
  startListeningToAnimatedNodeValue(tag: number): void {
    RNHLogger.debug(TAG, `startListeningToAnimatedNodeValue: ${tag}`);
    const node = this.nodes.get(tag);
    if (node) {
      node.addListener((value: number) => {
        this.emitEvent('onAnimatedValueUpdate', JSON.stringify({ tag, value }));
      });
    }
  }

  /**
   * Stop listening to node value updates
   */
  stopListeningToAnimatedNodeValue(tag: number): void {
    RNHLogger.debug(TAG, `stopListeningToAnimatedNodeValue: ${tag}`);
    const node = this.nodes.get(tag);
    if (node) {
      // Since we don't keep listener references, we cannot remove specific ones.
      // In a real implementation, keep a map of bridge listeners.
      node.valueListeners.clear();
    }
  }

  /**
   * Drop an animated node
   */
  dropAnimatedNode(tag: number): void {
    RNHLogger.debug(TAG, `dropAnimatedNode: ${tag}`);
    this.nodes.delete(tag);
  }

  /**
   * Resolve target value for animation
   */
  private resolveToValue(node: AnimatedNode, config: AnimationConfig): number {
    // RN animations typically pass toValue inside a 'frames' or 'toValue' field
    if (typeof config.toValue === 'number') {
      return config.toValue;
    }
    // Fallback: animate relative to current value
    return node.getValue() + 1;
  }

  /**
   * Map RN easing name to HarmonyOS animator easing
   */
  private mapEasing(easing?: string): string {
    switch (easing) {
      case 'linear':
        return 'linear';
      case 'ease':
      case 'easeInOut':
        return 'ease-in-out';
      case 'easeIn':
        return 'ease-in';
      case 'easeOut':
        return 'ease-out';
      default:
        return 'linear';
    }
  }

  /**
   * Propagate value changes to child nodes
   */
  private propagateValueToChildren(parentTag: number, value: number): void {
    const parent = this.nodes.get(parentTag);
    if (!parent) {
      return;
    }
    for (const childTag of parent.children) {
      const child = this.nodes.get(childTag);
      if (child) {
        child.setValue(value);
        this.propagateValueToChildren(childTag, value);
      }
    }
  }

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'AnimatedTurboModule initialized');
  }

  override invalidate(): void {
    RNHLogger.info(TAG, 'AnimatedTurboModule invalidated');
    for (const animation of this.animations.values()) {
      animation.pause();
    }
    this.animations.clear();
    this.nodes.clear();
    super.invalidate();
  }
}
