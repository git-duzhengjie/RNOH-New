/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import url from '@ohos.url';
import http from '@ohos.net.http';

import {EventEmitter} from './EventEmitter';

export interface ErrorData {
  whatHappened: string,
  howCanItBeFixed: string[];
  extraData?: any;
  customStack?: RNOHErrorStack | string;
}

export class RNOHError extends Error {
  protected data: undefined | {
    whatHappened: string,
    howCanItBeFixed: string[];
    extraData?: any;
    customStack?: RNOHErrorStack | string;
  };
  protected rnohError: undefined | RNOHError;

  static fromMultipleRNOHErrors(rnohErrors: RNOHError[]) {
    const message = rnohErrors.map((err, idx) => {
      return `${idx + 1}) ${err.getMessage()}`;
    }).join('\n');
    const suggestions = rnohErrors.flatMap(err => err.getSuggestions());
    const details =
      rnohErrors.map(err => err.getDetails()).filter(d =>!!d).map((d, idx) => `${idx + 1}) ${d.trim()}`).join('\n\n');
    return new RNOHError({ whatHappened: message, howCanItBeFixed: suggestions, extraData: details });
  }


  constructor(data: ErrorData | RNOHError) {
    super(data instanceof RNOHError ? data.getMessage() : data.whatHappened);
    if (data instanceof RNOHError) {
      this.rnohError = data;
    } else {
      this.data = data;
    }
  }

  getMessage(): string {
    if (this.data) {
      return this.data.whatHappened;
    } else {
      return this.rnohError.getMessage();
    }
  }

  getSuggestions(): string[] {
    if (this.data) {
      return this.data.howCanItBeFixed;
    } else {
      return this.rnohError.getSuggestions();
    }
  }

  getStack(): string | RNOHErrorStack | undefined {
    if (this.data) {
      let customStack = this.data.customStack;

      if (typeof customStack === 'string') {
        return customStack;
      } else if (customStack instanceof RNOHErrorStack) {
        return customStack;
      } else {
        return this.stack;
      }
    } else {
      return this.rnohError.getStack();
    }
  }

  getExtraData(): any {
    if (this.data) {
      return this.data.extraData;
    } else {
      return this.rnohError.getExtraData();
    }
  }

  /**
   * @deprecated: use getExtraData instead.
   */

  getDetails(): string {
    if (this.data) {
      if (!this.data.extraData) {
        return '';
      }
      if (typeof this.data.extraData === 'string') {
        return this.data.extraData.trim();
      }
      if (this.data.extraData instanceof Error) {
        let lines = [`${this.data.extraData.name}: ${this.data.extraData.message}`];
        for (let stackEntry of (this.data.extraData.stack ?? '').split('\n')) {
          lines.push('');
          lines.push(stackEntry);
        }
        return lines.join('\n');
      }
      try {
        return JSON.stringify(this.data.extraData, null, 2);
      } catch (err) {
        return '';
      }
    } else {
      return this.rnohError.getDetails();
    }
  }
}

export class RNInstanceError extends RNOHError {
  constructor(data: {
    whatHappened: string,
    howCanItBeFixed: string[];
    extraData?: any;
    customStack?: string;
  } | RNOHError, protected rnInstanceData: {
    name: string | undefined,
    id: number,
  }) {
    super(data);
  }

  getExtraData() {
    return {
      rnInstanceName: this.rnInstanceData.name,
      rnInstanceId: this.rnInstanceData.id,
      wrappedData: super.getExtraData(),
    };
  }

  getRNInstanceName() {
    return this.rnInstanceData.name;
  }

  getRNInstanceId() {
    return this.rnInstanceData.id;
  }
}

export class FatalRNOHError extends RNOHError {
}


export class WorkerRNOHError extends RNOHError {
}

export class RNOHErrorStack {
  constructor(private frames: StackFrame[]) {}

  static async fromBundledStackFrames(
    stackFrames: StackFrame[],
    shouldTryToSymbolicate: boolean = false,
  ): Promise<RNOHErrorStack> {
    if (!shouldTryToSymbolicate || stackFrames?.length === 0) {
      return Promise.resolve(new RNOHErrorStack(stackFrames));
    }

    // in our best effort, resolve the dev server url from bundle name
    let bundleUrl: url.URL;
    try {
      bundleUrl = url.URL.parseURL(stackFrames[0].file);
    } catch (ignored) {
      return Promise.resolve(new RNOHErrorStack(stackFrames));
    }

    // try to symbolicate with dev server
    try {
      return http
        .createHttp()
        .request(`${bundleUrl.origin}/symbolicate`, {
          method: http.RequestMethod.POST,
          readTimeout: 3000,
          connectTimeout: 3000,
          extraData: JSON.stringify({stack: stackFrames}),
        })
        .then((resp: http.HttpResponse) => {
          return new RNOHErrorStack(
            resp.responseCode === 200
              ? JSON.parse(resp.result.toString()).stack
              : stackFrames,
          );
        });
    } catch (ignored) {}

    return Promise.resolve(new RNOHErrorStack(stackFrames));
  }

  getFrames() {
    return this.frames;
  }

  toString(): string {
    return this.frames.map(entry => {
      let location = '';
      let locationInFile = '';
      if (entry.lineNumber) {
        if (entry.column !== undefined) {
          locationInFile = `${entry.lineNumber}:${entry.column}`;
        } else {
          locationInFile = `${entry.lineNumber}`;
        }
      }
      if (entry.file) {
        if (locationInFile) {
          location = `${entry.file}:${locationInFile}`;
        } else {
          location = `${entry.file}`;
        }
      } else {
        location = locationInFile;
      }
      if (location) {
        return `${entry.methodName} (${location})`;
      } else {
        return `${entry.methodName}`;
      }
    }).join('\n');
  }
}

export type RNOHErrorEventEmitter = EventEmitter<{
  'NEW_ERROR': [RNOHError];
}>;

export type RNInstanceErrorEventEmitter = EventEmitter<{
  'NEW_ERROR': [RNInstanceError];
}>;

export type StackFrame = {
  file?: string,
  methodName: string,
  lineNumber?: number,
  column?: number,
  collapse?: boolean,
};