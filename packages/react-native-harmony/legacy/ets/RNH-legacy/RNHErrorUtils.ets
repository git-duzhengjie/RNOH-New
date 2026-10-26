/**
 * RNOH-New: RNHErrorUtils - Shared error handling utilities
 *
 * Provides ArkTS-compatible helpers for converting unknown errors to Error
 * instances, satisfying arkts-limited-throw and arkts-no-any-unknown rules.
 */

const TAG = 'RNHErrorUtils';

/**
 * Convert an unknown error value into a concrete Error instance.
 * This is required because ArkTS strict mode forbids throwing arbitrary types.
 */
export function toError(e: Object | null | undefined): Error {
  if (e === null || e === undefined) {
    return new Error('Unknown error');
  }
  if (e instanceof Error) {
    return e;
  }
  if (typeof e === 'string') {
    return new Error(e);
  }
  try {
    return new Error(String(e));
  } catch {
    return new Error('Unknown error');
  }
}

/**
 * Get a safe error message string from any value.
 */
export function getErrorMessage(e: Object | null | undefined): string {
  if (e === null || e === undefined) {
    return 'Unknown error';
  }
  if (e instanceof Error) {
    return e.message;
  }
  if (typeof e === 'string') {
    return e;
  }
  try {
    return String(e);
  } catch {
    return 'Unknown error';
  }
}

/**
 * Get a safe error stack string from any value.
 */
export function getErrorStack(e: Object | null | undefined): string {
  if (e instanceof Error && e.stack) {
    return e.stack;
  }
  return '';
}
