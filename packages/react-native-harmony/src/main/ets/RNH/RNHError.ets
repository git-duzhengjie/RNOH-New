/**
 * RNOH-New: RNHError - Error types
 */

/**
 * Base error class for RNOH-New
 */
export class RNHError extends Error {
  public readonly code: number;
  public readonly domain: string;

  constructor(code: number, domain: string, message: string) {
    super(message);
    this.name = 'RNHError';
    this.code = code;
    this.domain = domain;
  }

  toString(): string {
    return `[${this.domain}:${this.code}] ${this.message}`;
  }
}

/**
 * Common error codes
 */
export const RNHErrorCode = {
  // Instance errors (1000-1099)
  INSTANCE_CREATE_FAILED: 1000,
  INSTANCE_DESTROY_FAILED: 1001,
  INSTANCE_NOT_FOUND: 1002,

  // Bundle errors (1100-1199)
  BUNDLE_LOAD_FAILED: 1100,
  BUNDLE_NOT_FOUND: 1101,
  BUNDLE_PARSE_ERROR: 1102,

  // Surface errors (1200-1299)
  SURFACE_START_FAILED: 1200,
  SURFACE_STOP_FAILED: 1201,
  SURFACE_NOT_FOUND: 1202,

  // TurboModule errors (1300-1399)
  TURBO_MODULE_NOT_FOUND: 1300,
  TURBO_MODULE_CALL_FAILED: 1301,

  // Component errors (1400-1499)
  COMPONENT_NOT_FOUND: 1400,
  COMPONENT_CREATE_FAILED: 1401,

  // Bridge errors (1500-1599)
  BRIDGE_INIT_FAILED: 1500,
  BRIDGE_CALL_FAILED: 1501,
} as const;

/**
 * Error domains
 */
export const RNHErrorDomain = {
  INSTANCE: 'RNHInstance',
  BUNDLE: 'RNHBundle',
  SURFACE: 'RNHSurface',
  TURBO_MODULE: 'RNHTurboModule',
  COMPONENT: 'RNHComponent',
  BRIDGE: 'RNHBridge',
} as const;
