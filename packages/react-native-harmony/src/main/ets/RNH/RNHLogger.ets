/**
 * RNOH-New: RNHLogger - Logging utility
 */

/**
 * Log levels
 */
export enum RNHLogLevel {
  DEBUG = 0,
  INFO = 1,
  WARN = 2,
  ERROR = 3,
}

/**
 * Logger configuration
 */
interface RNHLoggerConfig {
  minLevel: RNHLogLevel;
  enableConsole: boolean;
}

/**
 * Centralized logger for RNOH-New
 */
export class RNHLogger {
  private static config: RNHLoggerConfig = {
    minLevel: RNHLogLevel.DEBUG,
    enableConsole: true,
  };

  static configure(config: Partial<RNHLoggerConfig>): void {
    RNHLogger.config = { ...RNHLogger.config, ...config };
  }

  static debug(tag: string, message: string): void {
    if (RNHLogger.config.minLevel <= RNHLogLevel.DEBUG) {
      console.debug(`[${tag}] ${message}`);
    }
  }

  static info(tag: string, message: string): void {
    if (RNHLogger.config.minLevel <= RNHLogLevel.INFO) {
      console.info(`[${tag}] ${message}`);
    }
  }

  static warn(tag: string, message: string): void {
    if (RNHLogger.config.minLevel <= RNHLogLevel.WARN) {
      console.warn(`[${tag}] ${message}`);
    }
  }

  static error(tag: string, message: string): void {
    if (RNHLogger.config.minLevel <= RNHLogLevel.ERROR) {
      console.error(`[${tag}] ${message}`);
    }
  }
}
