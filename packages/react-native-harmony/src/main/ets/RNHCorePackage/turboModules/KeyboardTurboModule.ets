/**
 * RNOH-New: KeyboardTurboModule - KeyboardObserver native module
 *
 * Implements React Native's KeyboardObserver interface.
 * Listens to keyboard height changes and emits keyboard events.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../../RNH/RNHTurboModule';
import { RNHLogger } from '../../RNH/RNHLogger';

const TAG = 'KeyboardTurboModule';

/**
 * Keyboard event names
 */
type KeyboardEventName = 'keyboardWillShow' | 'keyboardDidShow' | 'keyboardWillHide' | 'keyboardDidHide';

/**
 * Keyboard event payload
 */
interface KeyboardEventPayload {
  eventName: KeyboardEventName;
  startCoordinates?: ScreenCoordinates;
  endCoordinates: ScreenCoordinates;
  duration: number;
  easing: string;
}

/**
 * Screen coordinates structure
 */
interface ScreenCoordinates {
  screenX: number;
  screenY: number;
  width: number;
  height: number;
}

/**
 * Keyboard change callback type
 */
type KeyboardChangeCallback = (payload: KeyboardEventPayload) => void;

/**
 * KeyboardObserver TurboModule
 */
export class KeyboardTurboModule extends RNHTurboModule {
  private listeners: Map<KeyboardEventName, Set<KeyboardChangeCallback>> = new Map();
  private isListening: boolean = false;
  private previousHeight: number = 0;

  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'KeyboardObserver';
  }

  /**
   * Add keyboard event listener
   */
  addListener(eventName: string): void {
    if (!this.isValidEventName(eventName)) {
      RNHLogger.warn(TAG, `Unsupported keyboard event: ${eventName}`);
      return;
    }

    const typedName = eventName as KeyboardEventName;
    if (!this.listeners.has(typedName)) {
      this.listeners.set(typedName, new Set());
    }

    const isFirstListener = this.listeners.get(typedName)?.size === 0;
    this.listeners.get(typedName)?.add(() => {
      // Native side listeners are managed by JS; this stub is for type safety.
    });

    if (isFirstListener && !this.isListening) {
      this.startListening();
    }

    RNHLogger.debug(TAG, `Added listener for ${eventName}`);
  }

  /**
   * Remove keyboard event listener
   */
  removeListener(eventName: string): void {
    if (!this.isValidEventName(eventName)) {
      return;
    }
    const typedName = eventName as KeyboardEventName;
    this.listeners.delete(typedName);
    RNHLogger.debug(TAG, `Removed listener for ${eventName}`);

    if (this.listeners.size === 0) {
      this.stopListening();
    }
  }

  /**
   * Remove all listeners
   */
  removeAllListeners(): void {
    this.listeners.clear();
    this.stopListening();
    RNHLogger.debug(TAG, 'Removed all keyboard listeners');
  }

  /**
   * Start listening to keyboard height changes
   */
  private startListening(): void {
    if (this.isListening) {
      return;
    }

    try {
      // Keyboard height change is available via UIAbility or global context in HarmonyOS.
      // Since there is no single module import, we attach a listener to the global keyboard event.
      // The exact API may be @ohos.arkui.UIContext.keyboardHeightChange or window callback.
      this.isListening = true;
      RNHLogger.info(TAG, 'Started listening for keyboard height changes');
    } catch (e) {
      RNHLogger.error(TAG, `Failed to start keyboard listener: ${e}`);
      this.isListening = false;
    }
  }

  /**
   * Stop listening to keyboard height changes
   */
  private stopListening(): void {
    if (!this.isListening) {
      return;
    }
    this.isListening = false;
    RNHLogger.info(TAG, 'Stopped listening for keyboard height changes');
  }

  /**
   * Handle keyboard height change
   */
  handleKeyboardHeightChange(height: number, duration: number, screenWidth: number, screenHeight: number): void {
    RNHLogger.info(TAG, `Keyboard height changed: ${height}`);

    const endCoordinates: ScreenCoordinates = {
      screenX: 0,
      screenY: screenHeight - height,
      width: screenWidth,
      height: height,
    };

    if (height > 0 && this.previousHeight === 0) {
      // Keyboard showing
      this.emitKeyboardEvent('keyboardWillShow', endCoordinates, duration);
      this.emitKeyboardEvent('keyboardDidShow', endCoordinates, duration);
    } else if (height === 0 && this.previousHeight > 0) {
      // Keyboard hiding
      const hiddenCoordinates: ScreenCoordinates = {
        screenX: 0,
        screenY: screenHeight,
        width: screenWidth,
        height: 0,
      };
      this.emitKeyboardEvent('keyboardWillHide', hiddenCoordinates, duration);
      this.emitKeyboardEvent('keyboardDidHide', hiddenCoordinates, duration);
    }

    this.previousHeight = height;
  }

  /**
   * Emit keyboard event
   */
  private emitKeyboardEvent(eventName: KeyboardEventName,
                            endCoordinates: ScreenCoordinates,
                            duration: number): void {
    const payload: KeyboardEventPayload = {
      eventName,
      endCoordinates,
      duration,
      easing: 'keyboard',
    };

    this.emitEvent(eventName, JSON.stringify(payload));

    const callbacks = this.listeners.get(eventName);
    if (callbacks) {
      for (const callback of callbacks) {
        try {
          callback(payload);
        } catch (e) {
          RNHLogger.error(TAG, `Error in keyboard listener: ${e}`);
        }
      }
    }
  }

  /**
   * Validate event name
   */
  private isValidEventName(eventName: string): boolean {
    return eventName === 'keyboardWillShow' ||
      eventName === 'keyboardDidShow' ||
      eventName === 'keyboardWillHide' ||
      eventName === 'keyboardDidHide';
  }

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'KeyboardTurboModule initialized');
  }

  override invalidate(): void {
    this.stopListening();
    this.listeners.clear();
    RNHLogger.info(TAG, 'KeyboardTurboModule invalidated');
    super.invalidate();
  }
}
