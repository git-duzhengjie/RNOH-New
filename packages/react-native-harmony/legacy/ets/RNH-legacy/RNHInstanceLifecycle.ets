/**
 * RNOH-New: RNHInstanceLifecycle - Strict lifecycle state machine
 *
 * Enforces valid state transitions for the RNH instance.
 * States: Uninitialized → Initialized → Started → Stopped → Destroyed
 */

import { RNHLogger } from './RNHLogger';

const TAG = 'RNHInstanceLifecycle';

/**
 * Instance lifecycle states
 */
export enum RNHInstanceState {
  Uninitialized = 'Uninitialized',
  Initialized = 'Initialized',
  Started = 'Started',
  Stopped = 'Stopped',
  Destroyed = 'Destroyed',
}

/**
 * Valid state transitions
 */
const VALID_TRANSITIONS: Map<RNHInstanceState, Set<RNHInstanceState>> = new Map([
  [RNHInstanceState.Uninitialized, new Set([RNHInstanceState.Initialized, RNHInstanceState.Destroyed])],
  [RNHInstanceState.Initialized, new Set([RNHInstanceState.Started, RNHInstanceState.Destroyed])],
  [RNHInstanceState.Started, new Set([RNHInstanceState.Stopped, RNHInstanceState.Destroyed])],
  [RNHInstanceState.Stopped, new Set([RNHInstanceState.Started, RNHInstanceState.Destroyed])],
  [RNHInstanceState.Destroyed, new Set()],
]);

/**
 * Lifecycle state machine for RNHInstance
 */
export class RNHInstanceLifecycle {
  private state: RNHInstanceState = RNHInstanceState.Uninitialized;
  private stateChangeCount: number = 0;
  private stateEnterTime: number = Date.now();
  private instanceId: number;

  constructor(instanceId: number = -1) {
    this.instanceId = instanceId;
  }

  /**
   * Get the current state
   */
  getState(): RNHInstanceState {
    return this.state;
  }

  /**
   * Assert that the current state is one of the expected states.
   * Throws an error if the assertion fails.
   */
  assertState(expectedStates: RNHInstanceState[]): void {
    const isExpected = expectedStates.indexOf(this.state) >= 0;
    if (!isExpected) {
      const expectedStr = expectedStates.join(', ');
      const msg = `Instance ${this.instanceId}: Expected state [${expectedStr}] but current state is ${this.state}`;
      RNHLogger.error(TAG, msg);
      throw new Error(msg);
    }
  }

  /**
   * Check if the current state is one of the expected states.
   * Returns boolean instead of throwing.
   */
  isInState(expectedStates: RNHInstanceState[]): boolean {
    return expectedStates.indexOf(this.state) >= 0;
  }

  /**
   * Transition to a new state.
   * Validates the transition is allowed, throws if not.
   */
  transitionTo(newState: RNHInstanceState): void {
    const validNext = VALID_TRANSITIONS.get(this.state);
    if (!validNext || !validNext.has(newState)) {
      const msg = `Instance ${this.instanceId}: Invalid state transition ${this.state} → ${newState}`;
      RNHLogger.error(TAG, msg);
      throw new Error(msg);
    }

    const prevState = this.state;
    const duration = Date.now() - this.stateEnterTime;

    this.state = newState;
    this.stateChangeCount++;
    this.stateEnterTime = Date.now();

    RNHLogger.info(
      TAG,
      `Instance ${this.instanceId}: ${prevState} → ${newState} (spent ${duration}ms in ${prevState})`
    );
  }

  /**
   * Try to transition to a new state.
   * Returns true if successful, false if the transition is invalid.
   */
  tryTransitionTo(newState: RNHInstanceState): boolean {
    try {
      this.transitionTo(newState);
      return true;
    } catch (e) {
      RNHLogger.warn(TAG, `Transition failed: ${e}`);
      return false;
    }
  }

  /**
   * Get the number of state changes
   */
  getStateChangeCount(): number {
    return this.stateChangeCount;
  }

  /**
   * Get time spent in current state (ms)
   */
  getTimeInCurrentState(): number {
    return Date.now() - this.stateEnterTime;
  }

  /**
   * Set the instance ID for logging
   */
  setInstanceId(id: number): void {
    this.instanceId = id;
  }

  /**
   * Reset the state machine (for testing)
   */
  reset(): void {
    this.state = RNHInstanceState.Uninitialized;
    this.stateChangeCount = 0;
    this.stateEnterTime = Date.now();
    RNHLogger.info(TAG, `Instance ${this.instanceId}: lifecycle reset`);
  }

  // Convenience state check methods

  isUninitialized(): boolean {
    return this.state === RNHInstanceState.Uninitialized;
  }

  isInitialized(): boolean {
    return this.state === RNHInstanceState.Initialized;
  }

  isStarted(): boolean {
    return this.state === RNHInstanceState.Started;
  }

  isStopped(): boolean {
    return this.state === RNHInstanceState.Stopped;
  }

  isDestroyed(): boolean {
    return this.state === RNHInstanceState.Destroyed;
  }

  /**
   * Check if the instance is usable (not destroyed, not uninitialized)
   */
  isUsable(): boolean {
    return this.state === RNHInstanceState.Initialized
      || this.state === RNHInstanceState.Started
      || this.state === RNHInstanceState.Stopped;
  }
}
