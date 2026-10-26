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
 *
 * 注意：ArkTS 严格模式禁止可变参数 spread，因此每个方法只接收单一 message 字符串。
 * 如需拼接多段信息，请在调用方使用模板字符串拼接。
 */
export class RNHLogger {
  private static config: RNHLoggerConfig = {
    minLevel: RNHLogLevel.DEBUG,
    enableConsole: true,
  };

  static configure(config: Partial<RNHLoggerConfig>): void {
    if (config.minLevel !== undefined) {
      RNHLogger.config.minLevel = config.minLevel;
    }
    if (config.enableConsole !== undefined) {
      RNHLogger.config.enableConsole = config.enableConsole;
    }
  }

  static debug(tag: string, message: string): void {
    if (RNHLogger.config.minLevel <= RNHLogLevel.DEBUG && RNHLogger.config.enableConsole) {
      console.debug(`[${tag}] ${message}`);
    }
  }

  static info(tag: string, message: string): void {
    if (RNHLogger.config.minLevel <= RNHLogLevel.INFO && RNHLogger.config.enableConsole) {
      console.info(`[${tag}] ${message}`);
    }
  }

  static warn(tag: string, message: string): void {
    if (RNHLogger.config.minLevel <= RNHLogLevel.WARN && RNHLogger.config.enableConsole) {
      console.warn(`[${tag}] ${message}`);
    }
  }

  static error(tag: string, message: string): void {
    if (RNHLogger.config.minLevel <= RNHLogLevel.ERROR && RNHLogger.config.enableConsole) {
      console.error(`[${tag}] ${message}`);
    }
  }
}
