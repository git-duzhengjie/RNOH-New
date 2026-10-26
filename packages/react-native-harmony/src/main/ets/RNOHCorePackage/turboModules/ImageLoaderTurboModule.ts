/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import {AnyThreadTurboModule} from '../../RNOH/TurboModule';
import {
  RemoteImageDiskCache,
  RemoteImageLoader,
  RemoteImageMemoryCache,
} from '../../RemoteImageLoader';
import {AnyThreadTurboModuleContext} from '../../RNOH/RNOHContext';
import http from '@ohos.net.http';
import image from '@ohos.multimedia.image';
import {fileIo} from '@kit.CoreFileKit';
import {media} from '@kit.MediaKit';

export class ImageLoaderTurboModule extends AnyThreadTurboModule {
  static NAME = 'ImageLoader' as const;
  private imageLoader: RemoteImageLoader;
  private videoThumbnailCache: RemoteImageDiskCache;
  private thumbnailDir: string;
  private static readonly VIDEO_FORMATS: Set<string> = new Set([
    '.mp4',
    '.mov',
    '.avi',
    '.mpeg',
    '.mpg',
    '.mkv',
    '.flv',
    '.wmv',
    '.m2ts',
    '.vob',
    '.ts',
    '.webm',
    '.m3u8',
    '.mpd',
    '.3gp',
    '.3g2',
    '.rm',
    '.rmvb',
    '.f4v',
    '.mxf',
  ]);

  constructor(protected ctx: AnyThreadTurboModuleContext) {
    super(ctx);
    const cacheSize = 128;
    this.imageLoader = new RemoteImageLoader(
      new RemoteImageMemoryCache(cacheSize),
      new RemoteImageDiskCache(
        cacheSize,
        `${ctx.uiAbilityContext.cacheDir}/rn_image_cache`,
      ),
      ctx.uiAbilityContext,
      ({remoteUri, fileUri}) => {
        ctx.rnInstance.postMessageToCpp('UPDATE_IMAGE_SOURCE_MAP', {
          remoteUri,
          fileUri,
        });
      },
      // Pass remoteValidation if it exists
      ctx.remoteValidation,
    );
    this.thumbnailDir = `${ctx.uiAbilityContext.cacheDir}/rn_video_thumbnail_cache`;
    this.videoThumbnailCache = new RemoteImageDiskCache(
      cacheSize,
      this.thumbnailDir,
    );
  }

  protected fetchVideoThumbnail(uri: string): string {
    if (this.videoThumbnailCache.has(uri)) {
      return this.getCacheFilePath(uri);
    } else {
      this.getVideoThumbnail(uri);
      return 'pending';
    }
  }

  private getCacheFilePath(uri: string): string {
    return `file://${this.videoThumbnailCache.getLocation(uri)}`;
  }

  protected async getVideoThumbnail(uri) {
    let inputFd: fileIo.File | undefined;
    let outputFd: fileIo.File | undefined;
    try {
      const avMetadataExtractor = await media.createAVMetadataExtractor();
      inputFd = fileIo.openSync(uri, fileIo.OpenMode.READ_ONLY);
      avMetadataExtractor.fdSrc = {fd: inputFd.fd};

      const metadata = await avMetadataExtractor.fetchMetadata();
      avMetadataExtractor.release();

      const avImageGenerator: media.AVImageGenerator =
        await media.createAVImageGenerator();
      avImageGenerator.fdSrc = {fd: inputFd.fd};

      const timeUs: number = 0;
      const queryOption: media.AVImageQueryOptions =
        media.AVImageQueryOptions.AV_IMAGE_QUERY_NEXT_SYNC;
      const backUpNum = 100;
      const param: media.PixelMapParams = {
        width: Number(metadata.videoWidth) || backUpNum,
        height: Number(metadata.videoHeight) || backUpNum,
      };
      const pixelMap = await avImageGenerator.fetchFrameByTime(
        timeUs,
        queryOption,
        param,
      );
      avImageGenerator.release();

      const imagePackerApi: image.ImagePacker = image.createImagePacker();
      const packOpts: image.PackingOption = {
        format: 'image/jpeg',
        quality: 100,
      };
      if (pixelMap !== undefined) {
        await imagePackerApi
          .packing(pixelMap, packOpts)
          .then(async (data: ArrayBuffer) => {
            const filePath = this.getCacheFilePath(uri);
            outputFd = fileIo.openSync(
              filePath,
              fileIo.OpenMode.READ_WRITE | fileIo.OpenMode.CREATE,
            );
            await fileIo.write(outputFd.fd, data);

            this.videoThumbnailCache.set(uri);
            this.ctx.rnInstance.postMessageToCpp('UPDATE_IMAGE_SOURCE_MAP', {
              remoteUri: uri,
              fileUri: filePath,
            });
          });
      } else {
        this.ctx.logger.info('pixelMap is undefined');
      }
    } catch (e) {
      this.ctx.logger.error(JSON.stringify(e));
    } finally {
      // Ensure both file descriptors are closed on every exit path (including
      // exceptions) to avoid leaking system fds on repeated thumbnail requests.
      if (outputFd !== undefined) {
        try {
          fileIo.closeSync(outputFd);
        } catch (e) {
          this.ctx.logger.error(
            'Failed to close output fd: ' + JSON.stringify(e),
          );
        }
      }
      if (inputFd !== undefined) {
        try {
          fileIo.closeSync(inputFd);
        } catch (e) {
          this.ctx.logger.error(
            'Failed to close input fd: ' + JSON.stringify(e),
          );
        }
      }
    }
  }

  private isVideoUri(uri: string): boolean {
    if (!uri) {
      return false;
    }

    const paramIndex = uri.indexOf('?');
    const path = paramIndex === -1 ? uri : uri.substring(0, paramIndex);

    const lastDotIndex = path.lastIndexOf('.');
    if (lastDotIndex === -1) {
      return false;
    }

    const uriSuffix = path.slice(lastDotIndex);
    const uriSuffixLower = uriSuffix.toLowerCase();

    return ImageLoaderTurboModule.VIDEO_FORMATS.has(uriSuffixLower);
  }

  /**
   * called from cpp
   */
  protected getPrefetchResult(uri: string): string | undefined {
    if (this.isVideoUri(uri)) {
      return this.fetchVideoThumbnail(uri);
    }
    return this.imageLoader.getPrefetchResult(uri);
  }

  public abortRequest(requestId: number): void {
    this.imageLoader.abortRequest(requestId);
  }

  public getConstants() {
    return {};
  }

  public async getSize(uri: string): Promise<{
    width: number;
    height: number;
  }> {
    if (uri && uri.length > 0) {
      const imageSource = await this.imageLoader.getImageSource(uri);
      const imageInfo = await imageSource.getImageSource().getImageInfo();
      return {width: imageInfo.size.width, height: imageInfo.size.height};
    } else {
      throw 'Cannot get the size of an image for an empty URI';
    }
  }

  public async getSizeWithHeaders(
    uri: string,
    headers: Object,
  ): Promise<
    {
      width: number;
      height: number;
    } & Record<string, any>
  > {
    let srcHeaders = headers as Record<string, any>;
    let destHeaders: Record<string, any> = {};

    if (srcHeaders.crossOrigin === 'use-credentials') {
      destHeaders['Access-Control-Allow-Credentials'] = 'true';
    }

    if (srcHeaders.referrerPolicy !== undefined) {
      destHeaders['Referrer-Policy'] = srcHeaders.referrerPolicy;
    }

    const imageSource = await this.imageLoader.getImageSource(uri, destHeaders);
    const imageInfo = await imageSource.getImageSource().getImageInfo();
    return Promise.resolve({
      width: imageInfo.size.width,
      height: imageInfo.size.height,
    });
  }

  public async prefetchImage(
    uri: string,
    requestId: number = 0,
  ): Promise<boolean> {
    return this.imageLoader.prefetch(uri, requestId).catch(err => {
      if (err?.code === 'E_PREFETCH_ABORT') {
        return false;
      }
      return Promise.reject(err);
    });
  }

  public prefetchImageWithMetadata(
    uri: string,
    queryRootName: string,
    rootTag: number,
  ): Promise<boolean> {
    this.ctx.logger.warn(
      'ImageLoader::prefetchImageWithMetadata is not supported',
    );
    return Promise.resolve(false);
  }

  public queryCache(uris: Array<string>): Promise<Object> {
    const cachedUriEntries = uris
      .map(uri => [uri, this.imageLoader.queryCache(uri)])
      .filter(([_uri, value]) => value !== undefined);
    const cachedUriMap = Object.fromEntries(cachedUriEntries);
    return Promise.resolve(cachedUriMap);
  }
}
