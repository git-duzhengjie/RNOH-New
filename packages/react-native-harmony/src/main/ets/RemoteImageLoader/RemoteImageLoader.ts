/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import image from '@ohos.multimedia.image';
import type {RemoteImageMemoryCache} from './RemoteImageCache';
import {RemoteImageLoaderError} from './RemoteImageLoaderError';
import request from '@ohos.request';
import type {RemoteImageDiskCache} from './RemoteImageDiskCache';
import fs from '@ohos.file.fs';

import {
  fetchDataFromUrl,
  FetchOptions,
  FetchResult,
  FetchAbortController,
  downloadFileFromUrl,
} from '../RNOH/HttpRequestHelper';
import http from '@ohos.net.http';
import {RemoteImageSource} from './RemoteImageSource';
import {common} from '@kit.AbilityKit';

/**
 * Per-instance image loader. Only the prefetch/download dedup table is shared
 * cross-instance (all instances write the same tempPath); everything else
 * (decoded memoryCache, fetch dedup, abort tables, disk-cache-update callback)
 * is per-instance and dies with it. The injected diskCache is used solely for
 * canonical path computation — its index/eviction are NOT used: disk cache
 * state lives in C++ ImageSourceResolver. All map mutations run on the
 * single-threaded ArkTS event loop.
 */
export class RemoteImageLoader {
  private activeRequestByUrl: Map<string, Promise<FetchResult>> = new Map();
  // Shared cross-instance: a global dedup prevents colliding downloads onto
  // the same tempPath. Claim discipline: check+register must stay synchronous
  // (no await in between) — the single-threaded event loop is the only lock.
  // Never cleared on instance destroy — in-flight downloads must keep running
  // and self-clean via their own .finally().
  private static activePrefetchByUrl: Map<string, Promise<boolean>> = new Map();
  private abortControllerById: Map<number, FetchAbortController> = new Map();
  private abortRequestById: Map<number, request.DownloadTask> = new Map();

  public constructor(
    private memoryCache: RemoteImageMemoryCache,
    private diskCache: RemoteImageDiskCache,
    private context: common.UIAbilityContext,
    private onDiskCacheUpdate: (e: {
      remoteUri: string;
      fileUri: string;
    }) => void,
    private remoteValidation?: http.RemoteValidation,
  ) {}

  private async fetchImage(
    url: string,
    headers?: Record<string, any>,
  ): Promise<FetchResult> {
    if (this.activeRequestByUrl.has(url)) {
      return this.activeRequestByUrl.get(url);
    }

    const remoteValidation = this.remoteValidation;
    const fetchOptions: FetchOptions = {
      usingCache: true,
      headers: headers,
      remoteValidation: remoteValidation,
    };
    const promise = fetchDataFromUrl(url, fetchOptions);
    this.activeRequestByUrl.set(url, promise);
    promise.finally(() => {
      this.activeRequestByUrl.delete(url);
    });
    return promise;
  }

  public async getPixelMap(uri: string): Promise<image.PixelMap> {
    const imageSource = await this.getImageSource(uri);
    return await imageSource.getPixelMapPromise();
  }

  public async getImageSource(
    uri: string,
    headers?: Record<string, any>,
  ): Promise<RemoteImageSource> {
    if (uri.startsWith('data:')) {
      const imageSource = image.createImageSource(uri);
      return new RemoteImageSource(imageSource, '');
    }
    if (this.memoryCache.has(uri)) {
      return this.memoryCache.get(uri);
    }

    const finalPath = this.diskCache.getLocation(uri);

    // 1. Completed file on disk: build the source from it directly.
    const fromDisk = this.createSourceFromDisk(uri, finalPath);
    if (fromDisk) {
      return fromDisk;
    }

    // 2/3. Wait for an in-flight download, or claim the shared dedup slot
    // and fetch ourselves. The claim is registered BEFORE any I/O and the
    // get/set around it contain no await, so on the single-threaded event
    // loop claiming is atomic: concurrent callers (any component, any
    // instance of this runtime) either wait here or claim first — they can
    // never fetch/save in parallel onto the same tempPath.
    // The loop is a claim-retry: after a waited entry settles (owner always
    // deletes it), re-check before claiming so a slot another waiter just
    // took is never overwritten; the claim branch always returns or throws.
    for (;;) {
      const inFlight = RemoteImageLoader.activePrefetchByUrl.get(uri);
      if (inFlight !== undefined) {
        // ignoring any errors that occur (we'll claim and retry instead)
        await inFlight.catch(() => {});
        // The claimant already decoded the source into memoryCache (even if
        // its persist failed) — reuse it instead of re-downloading or
        // re-decoding from disk.
        if (this.memoryCache.has(uri)) {
          return this.memoryCache.get(uri);
        }
        const waited = this.createSourceFromDisk(uri, finalPath);
        if (waited) {
          return waited;
        }
        continue;
      }

      let releaseClaim: (done: boolean) => void = () => {};
      const claim = new Promise<boolean>(resolve => {
        releaseClaim = resolve;
      });
      RemoteImageLoader.activePrefetchByUrl.set(uri, claim);
      try {
        const {response, persisted} = await this.fetchAndPersist(
          uri,
          finalPath,
          headers,
        );
        // The claim boolean means "file is on disk": prefetch piggybackers
        // return it as the Image.prefetch result.
        releaseClaim(persisted);
        return this.createSourceFromResponse(uri, response);
      } catch (e) {
        releaseClaim(false);
        console.warn(`Failed to fetch image for ${uri}: ${e}`);
        throw new RemoteImageLoaderError(
          e.message ?? 'Failed to fetch the image',
        );
      } finally {
        RemoteImageLoader.activePrefetchByUrl.delete(uri);
      }
    }
  }

  // Builds a source from the completed cache file; undefined if it's absent.
  private createSourceFromDisk(
    uri: string,
    finalPath: string,
  ): RemoteImageSource | undefined {
    if (!fs.accessSync(finalPath)) {
      return undefined;
    }
    const location = `file://${finalPath}`;
    const imageSource = image.createImageSource(location);
    // createImageSource returns undefined (not null) on failure. Here it
    // means "evicted between probe and open" or unreadable file — treat as
    // a cache miss so the claim loop re-downloads instead of failing.
    if (!imageSource) {
      return undefined;
    }
    const remoteImageSource = new RemoteImageSource(imageSource, location);
    this.memoryCache.set(uri, remoteImageSource);
    return remoteImageSource;
  }

  private createSourceFromResponse(
    uri: string,
    response: FetchResult,
  ): RemoteImageSource {
    const imageSource = image.createImageSource(response.result);
    // createImageSource returns undefined (not null) on failure; the bytes
    // are already in hand, so a decode failure is final — throw (baseline
    // contract: JS failure callback relies on the rejection).
    if (!imageSource) {
      throw new RemoteImageLoaderError("Couldn't create ImageSource");
    }
    // NOTE: workaround for `Image` component not supporting redirects or ImageSource sources.
    // If the image is an animated GIF, we cannot convert it to a PixelMap
    // since that would display a single frame only.
    // Instead, when redirected, we attach the `location` url to the imageSource object
    // and use it as the source parameter for `Image`
    const location =
      (response.headers as Record<string, string>).location ?? uri;
    const remoteImageSource = new RemoteImageSource(imageSource, location);
    this.memoryCache.set(uri, remoteImageSource);
    return remoteImageSource;
  }

  // Fetches into memory, then persists to the canonical path (temp + rename).
  // Reports cache-ready only after a successful persist; a persist failure is
  // logged and does not fail the fetch result, but is surfaced as
  // persisted=false so the claim boolean stays honest ("file is on disk").
  private async fetchAndPersist(
    uri: string,
    finalPath: string,
    headers?: Record<string, any>,
  ): Promise<{response: FetchResult; persisted: boolean}> {
    const response = await this.fetchImage(uri, headers);
    let persisted = false;
    try {
      await this.saveFile(uri, response.result);
      this.onDiskCacheUpdate({remoteUri: uri, fileUri: `file://${finalPath}`});
      persisted = true;
    } catch (e) {
      console.warn(`Failed to persist image cache for ${uri}: ${e}`);
    }
    return {response, persisted};
  }

  private static closeQuietly(file: fs.File): void {
    try {
      fs.closeSync(file);
    } catch (closeError) {
      console.warn('Failed to close file descriptor: ' + closeError.message);
    }
  }

  private async saveFile(uri: string, arrayBuffer: ArrayBuffer) {
    const path = this.diskCache.getLocation(uri);
    const tempPath = path + '_tmp';
    let file: fs.File | undefined;
    try {
      try {
        fs.unlinkSync(tempPath);
      } catch (_) {}
      file = await fs.open(
        tempPath,
        // eslint-disable-next-line no-bitwise
        fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE,
      );
      await fs.write(file.fd, arrayBuffer);
      RemoteImageLoader.closeQuietly(file);
      file = undefined;
      await fs.moveFile(tempPath, path);
    } catch (error) {
      if (file !== undefined) {
        RemoteImageLoader.closeQuietly(file);
      }
      try {
        fs.unlinkSync(tempPath);
      } catch (_) {}
      throw error;
    }
    return true;
  }

  // Registers the promise under the global dedup key, reports the cache-ready
  // update on success, and always clears the dedup entry when settled.
  private trackAndNotify(
    uri: string,
    finalPath: string,
    promise: Promise<boolean>,
    failureLogPrefix: string,
  ): void {
    RemoteImageLoader.activePrefetchByUrl.set(uri, promise);
    promise
      .then(() =>
        this.onDiskCacheUpdate({
          remoteUri: uri,
          fileUri: `file://${finalPath}`,
        }),
      )
      .catch((e: Error) => console.warn(`${failureLogPrefix}: ${e}`))
      .finally(() => RemoteImageLoader.activePrefetchByUrl.delete(uri));
  }

  public abortRequest(requestId: number): void {
    // Download-service path (remoteValidation unset): cancel via DownloadTask.delete().
    this.abortRequestById.get(requestId)?.delete();
    // Stream path (remoteValidation set): cancel via FetchAbortController.
    this.abortControllerById.get(requestId)?.abort();
  }

  public async prefetch(uri: string, requestId: number): Promise<boolean> {
    const finalPath = this.diskCache.getLocation(uri);
    if (fs.accessSync(finalPath)) {
      return true;
    }

    if (RemoteImageLoader.activePrefetchByUrl.has(uri)) {
      return await RemoteImageLoader.activePrefetchByUrl.get(uri);
    }

    // evict the image from cache if present
    if (this.memoryCache.has(uri)) {
      this.memoryCache.remove(uri);
    }

    const promise = this.downloadFromNetwork(uri, requestId);
    this.trackAndNotify(uri, finalPath, promise, `Prefetch failed for ${uri}`);

    return await promise;
  }

  private async performDownload(
    config: request.DownloadConfig,
    requestId: number,
  ): Promise<boolean> {
    return await new Promise(async (resolve, reject) => {
      try {
        const downloadTask = await request.downloadFile(this.context, config);
        this.abortRequestById.set(requestId, downloadTask);
        downloadTask.on('complete', () => {
          this.abortRequestById.delete(requestId);
          resolve(true);
        });
        downloadTask.on('remove', () => {
          this.abortRequestById.delete(requestId);
          reject({
            code: 'E_PREFETCH_ABORT',
            message: `The download task has been removed.`,
          });
        });
        downloadTask.on('fail', (err: number) => {
          this.abortRequestById.delete(requestId);
          reject(`Failed to download the task. Code: ${err}`);
        });
      } catch (e) {
        reject(e);
      }
    });
  }

  private async downloadFile(uri: string, requestId: number): Promise<boolean> {
    const path = this.diskCache.getLocation(uri);
    // No requestId suffix: global dedup prevents concurrent same-URL
    // downloads, and a shared temp name self-cleans stale leftovers.
    const tempPath = path + '_tmp';
    try {
      // Remove stale temp file from a prior failed/incomplete download.
      try {
        await fs.unlink(tempPath);
      } catch (_) {
        // Temp file doesn't exist — first run or already cleaned.
      }
      await this.performDownload({url: uri, filePath: tempPath}, requestId);
      // Move the file to the final location and remove the temporary file
      await fs.moveFile(tempPath, path);
      return true;
    } catch (e) {
      // Best-effort cleanup of temp file on failure/abort.
      try {
        await fs.unlink(tempPath);
      } catch (_) {
        // Ignore cleanup errors.
      }
      return Promise.reject(e);
    }
  }

  /**
   * Downloads the image to disk, choosing the data path by whether custom
   * certificate validation is required:
   *
   *   - remoteValidation SET   -> downloadFileFromUrl (HTTP Kit requestInStream).
   *     Only @ohos.net.http exposes remoteValidation, which the download service
   *     does not, so cert validation forces the streamed path.
   *   - remoteValidation UNSET -> downloadFile (this class) via @ohos.request.downloadFile,
   *     the system download service (separate process, native stack, concurrency-friendly).
   *
   * Both paths leave the file at the canonical cache path (getLocation(uri)) on
   * success. The caller (prefetch) then fires onDiskCacheUpdate, which posts
   * UPDATE_IMAGE_SOURCE_MAP to this instance's C++; the cache there is
   * process-global, so all instances see the entry on their next resolve.
   */
  private async downloadFromNetwork(
    uri: string,
    requestId: number,
  ): Promise<boolean> {
    if (this.remoteValidation !== undefined) {
      const abortController = new FetchAbortController();
      this.abortControllerById.set(requestId, abortController);
      const path = this.diskCache.getLocation(uri);
      try {
        await downloadFileFromUrl(
          uri,
          path,
          {usingCache: false, remoteValidation: this.remoteValidation},
          undefined,
          abortController,
        );
        return true;
      } finally {
        this.abortControllerById.delete(requestId);
      }
    } else {
      // Download-service path (@ohos.request.downloadFile).
      return await this.downloadFile(uri, requestId);
    }
  }

  public queryCache(uri: string): 'memory' | 'disk' | undefined {
    const finalPath = this.diskCache.getLocation(uri);
    if (fs.accessSync(finalPath)) {
      return 'disk';
    }
    if (this.memoryCache.has(uri)) {
      return 'memory';
    }
    return undefined;
  }

  // Retained for spec/codegen compatibility; C++ no longer callSyncs this
  // (resolveImageSource queries its own shared tables now). Kept functional
  // via fs probe so it remains correct if ever invoked.
  public getPrefetchResult(uri: string) {
    if (RemoteImageLoader.activePrefetchByUrl.has(uri)) {
      return 'pending';
    }
    const finalPath = this.diskCache.getLocation(uri);
    if (fs.accessSync(finalPath)) {
      return `file://${finalPath}`;
    }
    return undefined;
  }
}
