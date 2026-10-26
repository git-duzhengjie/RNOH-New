/**
 * RNOH-New: AccessibilityInfoTurboModule - Provides accessibility info to JS
 *
 * Implements the RN AccessibilityInfo native module interface.
 * Uses HarmonyOS accessibility APIs.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../../RNH/RNHTurboModule';
import { RNHLogger } from '../../RNH/RNHLogger';

const TAG = 'AccessibilityInfoTurboModule';

/**
 * Event listener callback type
 */
type AccessibilityListener = (enabled: boolean) => void;

/**
 * AccessibilityInfo TurboModule
 */
export class AccessibilityInfoTurboModule extends RNHTurboModule {
  private listeners: Map<string, Set<AccessibilityListener>> = new Map();

  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'AccessibilityInfo';
  }

  /**
   * Check if reduce motion is enabled
   */
  isReduceMotionEnabled(): Promise<boolean> {
    // HarmonyOS does not have a direct API for reduce motion
    // Return false as default; can be extended later
    return Promise.resolve(false);
  }

  /**
   * Check if bold text is enabled
   */
  isBoldTextEnabled(): Promise<boolean> {
    // HarmonyOS does not have a direct API for bold text
    // Return false as default; can be extended later
    return Promise.resolve(false);
  }

  /**
   * Check if grayscale is enabled
   */
  isGrayscaleEnabled(): Promise<boolean> {
    // HarmonyOS does not have a direct API for grayscale
    return Promise.resolve(false);
  }

  /**
   * Check if invert colors is enabled
   */
  isInvertColorsEnabled(): Promise<boolean> {
    // HarmonyOS does not have a direct API for invert colors
    return Promise.resolve(false);
  }

  /**
   * Check if touch exploration is enabled (TalkBack equivalent)
   */
  isTouchExplorationEnabled(): Promise<boolean> {
    // HarmonyOS has screen reader, but API may vary
    // Return false as default; can be extended later
    return Promise.resolve(false);
  }

  /**
   * Check if screen reader is enabled
   */
  isScreenReaderEnabled(): Promise<boolean> {
    // HarmonyOS has screen reader, but API may vary
    // Return false as default; can be extended later
    return Promise.resolve(false);
  }

  /**
   * Add event listener
   */
  addEventListener(eventName: string, listener: AccessibilityListener): void {
    if (!this.listeners.has(eventName)) {
      this.listeners.set(eventName, new Set());
    }
    this.listeners.get(eventName)?.add(listener);
    RNHLogger.debug(TAG, `Added listener for event: ${eventName}`);
  }

  /**
   * Remove event listener
   */
  removeEventListener(eventName: string, listener: AccessibilityListener): void {
    this.listeners.get(eventName)?.delete(listener);
    RNHLogger.debug(TAG, `Removed listener for event: ${eventName}`);
  }

  /**
   * Announce for accessibility
   */
  announceForAccessibility(message: string): void {
    RNHLogger.info(TAG, `Accessibility announcement: ${message}`);
    // Can be extended to use HarmonyOS accessibility announcement API
  }

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'AccessibilityInfoTurboModule initialized');
  }

  override invalidate(): void {
    this.listeners.clear();
    RNHLogger.info(TAG, 'AccessibilityInfoTurboModule invalidated');
    super.invalidate();
  }
}
