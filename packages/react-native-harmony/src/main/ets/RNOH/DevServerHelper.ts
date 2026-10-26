/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import url from '@ohos.url';
import http from '@ohos.net.http';
import {getInspectorPackagerConnection} from './InspectorPackagerConnection';
import {RNOHLogger} from './RNOHLogger';
import {InspectorPackagerConnection} from './types';
import {NapiBridge} from './NapiBridge';
import {bundleManager} from '@kit.AbilityKit';
import {deviceInfo} from '@kit.BasicServicesKit';
import {hash} from '@kit.CoreFileKit';
import {buffer} from '@kit.ArkTS';
import {getRemoteValidation} from './HttpRequestHelper';
function getServerHost(bundleUrlString: string): string {
  try {
    const bundleUrl = url.URL.parseURL(bundleUrlString);
    const host = bundleUrl.hostname ?? 'localhost';
    const port = bundleUrl.port ?? '8081';
    return `${host}:${port}`;
  } catch (e) {
    const errMsg = e instanceof Error ? e.message : String(e);
    console.warn(
      `[RNOH] getServerHost: failed to parse bundle URL "${bundleUrlString}", falling back to localhost:8081. Error: ${errMsg}`,
    );
    return 'localhost:8081';
  }
}

/**
 * @internal
 * SHA-256 hashing implementation.
 * Based on the implementation from ReactAndroid: src/main/java/com/facebook/react/devsupport/DevServerHelper.java.
 * Only the first 20 bytes of the hash are used to identify the device,
 * which is why only the first 40 hexadecimal characters are utilized.
 */
function getSHA256(data: string): string {
  const hs = hash.createHash('sha256');
  const dataBuffer = new Uint8Array(buffer.from(data, 'utf-8').buffer);
  hs.update(dataBuffer.buffer);
  return hs.digest().substring(0, 40);
}

/**
 * @internal
 * Returns an opaque ID which is stable for the current combination of device and app it is stable across installs,
 * and identifies a device.
 */
function getInspectorDeviceId(
  appName: string,
  isFuseboxEnabled: boolean,
): string {
  // ODID is an alphanumeric string used to identify a device.
  // It is not guaranteed to be unique but does not require special permissions.
  const openDeviceIdentifier = deviceInfo.ODID;
  return getSHA256(
    `huawei-${appName}-${openDeviceIdentifier}-${isFuseboxEnabled ? 'fusebox' : 'legacy'}`,
  );
}

function getInspectorUrl(bundleUrl: string, isFuseboxEnabled: boolean): string {
  const bundleInfo = bundleManager.getBundleInfoForSelfSync(
    bundleManager.BundleFlag.GET_BUNDLE_INFO_DEFAULT,
  );
  const appName = bundleInfo.name;
  const deviceName = deviceInfo.marketName;
  const deviceId = getInspectorDeviceId(appName, isFuseboxEnabled);
  const params = new url.URLParams(
    `name=${deviceName}&app=${appName}&device=${deviceId}`,
  );
  return `http://${getServerHost(bundleUrl)}/inspector/device?${params}`;
}

/**
 * @internal
 * This is a port of the Android impl, at
 * ReactAndroid/src/main/java/com/facebook/react/devsupport/DevServerHelper.java
 */
export class DevServerHelper {
  static DEBUGGER_MSG_DISABLE = '{ "id":1,"method":"Debugger.disable" }';
  static connectionByInspectorUrl = new Map<
    string,
    InspectorPackagerConnection
  >();

  public static connectToDevServer(
    bundleUrl: string,
    logger: RNOHLogger,
    napiBridge: NapiBridge,
    appName: string | undefined = undefined,
  ) {
    appName = appName ?? 'undefinedAppName@' + new Date().toISOString();
    const _logger = logger.clone(
      `DevServerHelper::connectToDevServer (appName=${appName})`,
    );
    const isFuseboxEnabled = napiBridge.getInspectorFlagIsFuseboxEnabled();
    const inspectorUrl = getInspectorUrl(bundleUrl, isFuseboxEnabled);
    let connection = DevServerHelper.connectionByInspectorUrl.get(inspectorUrl);

    if (!connection) {
      connection = getInspectorPackagerConnection(
        napiBridge,
        inspectorUrl,
        appName,
        logger,
      );
      DevServerHelper.connectionByInspectorUrl.set(inspectorUrl, connection);
      connection.connect();
      _logger.debug('CONNECTED');
    }

    return () => {
      connection.closeQuietly();
      DevServerHelper.connectionByInspectorUrl.delete(inspectorUrl);
      _logger.debug('CLOSE');
    };
  }

  public static openDebugger(
    bundleUrl: string,
    napiBridge: NapiBridge,
    onError: () => void,
    remoteValidation: http.RemoteValidation | undefined = undefined,
  ) {
    const bundleInfo = bundleManager.getBundleInfoForSelfSync(
      bundleManager.BundleFlag.GET_BUNDLE_INFO_DEFAULT,
    );
    const appName = bundleInfo.name;
    const isFuseboxEnabled = napiBridge.getInspectorFlagIsFuseboxEnabled();
    const deviceId = getInspectorDeviceId(appName, isFuseboxEnabled);
    const openDebuggerUrl = `http://${getServerHost(bundleUrl)}/open-debugger?device=${deviceId}`;

    const httpRequest = http.createHttp();

    httpRequest.request(
      openDebuggerUrl,
      {
        method: http.RequestMethod.POST,
        remoteValidation: getRemoteValidation(remoteValidation),
      },
      err => {
        if (err) {
          onError();
        }
        httpRequest.destroy();
      },
    );
  }

  public static disableDebugger() {
    DevServerHelper.connectionByInspectorUrl.forEach(connection => {
      connection.sendEventToAllConnections(
        DevServerHelper.DEBUGGER_MSG_DISABLE,
      );
    });
  }
}
