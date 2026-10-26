/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import { DescriptorWrapperFactoryByDescriptorType } from '../RNOH/ts';
import type { AnyThreadTurboModule, UITurboModule } from '../RNOH/TurboModule';
import type { AnyThreadTurboModuleContext, UITurboModuleContext } from "../RNOH/RNOHContext"
import {
  AccessibilityInfoTurboModule,
  ActionSheetManagerTurboModule,
  AlertManagerTurboModule,
  AppStateTurboModule,
  BlobTurboModule,
  DeviceEventManagerTurboModule,
  DeviceInfoTurboModule,
  DevLoadingViewTurboModule,
  DevMenuTurboModule,
  DevSettingsTurboModule,
  ExceptionsManagerTurboModule,
  FileReaderTurboModule,
  I18nManagerTurboModule,
  ImageLoaderTurboModule,
  KeyboardObserverTurboModule,
  LogBoxTurboModule,
  NativeAnimatedTurboModule,
  NetworkingTurboModule,
  PlatformColorTurboModule,
  PlatformConstantsTurboModule,
  SafeAreaTurboModule,
  ShareTurboModule,
  SourceCodeTurboModule,
  StatusBarTurboModule,
  ToastAndroidTurboModule,
  VibrationTurboModule,
  WebSocketTurboModule,
  LinkingManagerTurboModule
} from './turboModules';
import { ViewDescriptorWrapper } from './components/ts';
import { AppearanceTurboModule } from './turboModules/AppearanceTurboModule';
import { RNOHPackage } from '../RNOH/RNOHPackage';
import {
  buildDrawerLayoutAndroid,
  RNDrawerLayoutAndroid
} from './components/RNDrawerLayoutAndroid/RNDrawerLayoutAndroid';
import { ComponentBuilderContext } from '../RNOH';
import {
  buildRNDrawerLayoutChildWrapperView,
  RNDrawerLayoutChildWrapperView
} from './components/RNDrawerLayoutAndroid/RNDrawerLayoutChildWrapperView';
import { RNDrawerLayoutAndroidDescriptorWrapper } from './components/RNDrawerLayoutAndroid/types';
import { RNViewBase } from '.';

export class RNOHCorePackage extends RNOHPackage {
  override createDescriptorWrapperFactoryByDescriptorType(): DescriptorWrapperFactoryByDescriptorType {
    const result: DescriptorWrapperFactoryByDescriptorType = {}
    result[RNDrawerLayoutAndroid.NAME] = ctx => new RNDrawerLayoutAndroidDescriptorWrapper(ctx.descriptor)
    result[RNViewBase.NAME] = (ctx) => new ViewDescriptorWrapper(ctx.descriptor)
    return result;
  }

  override getUITurboModuleFactoryByNameMap(): Map<string, (ctx: UITurboModuleContext) => UITurboModule | null> {
    return new Map<string, ((ctx: UITurboModuleContext) => UITurboModule)>()
      .set(AccessibilityInfoTurboModule.NAME, (ctx) => new AccessibilityInfoTurboModule(ctx))
      .set(ActionSheetManagerTurboModule.NAME, (ctx) => new ActionSheetManagerTurboModule(ctx))
      .set(AlertManagerTurboModule.NAME, (ctx) => new AlertManagerTurboModule(ctx))
      .set(DeviceEventManagerTurboModule.NAME, (ctx) => new DeviceEventManagerTurboModule(ctx))
      .set(DevSettingsTurboModule.NAME, (ctx) => new DevSettingsTurboModule(ctx))
      .set(DevMenuTurboModule.NAME, (ctx) => new DevMenuTurboModule(ctx))
      .set(ExceptionsManagerTurboModule.NAME, (ctx) => new ExceptionsManagerTurboModule(ctx))
      .set(KeyboardObserverTurboModule.NAME, (ctx) => new KeyboardObserverTurboModule(ctx))
      .set(NativeAnimatedTurboModule.NAME, (ctx) => new NativeAnimatedTurboModule(ctx))
      .set(ShareTurboModule.NAME, (ctx) => new ShareTurboModule(ctx))
      .set(I18nManagerTurboModule.NAME, (ctx) => new I18nManagerTurboModule(ctx))
      .set(LogBoxTurboModule.NAME, (ctx) => new LogBoxTurboModule(ctx))
      .set(DevLoadingViewTurboModule.NAME, (ctx) => new DevLoadingViewTurboModule(ctx))
      .set(VibrationTurboModule.NAME, (ctx) => new VibrationTurboModule(ctx))
      .set(ToastAndroidTurboModule.NAME, (ctx) => new ToastAndroidTurboModule(ctx))
  }

  override async createEagerUITurboModuleByNameMap(ctx: UITurboModuleContext): Promise<Map<string, UITurboModule>> {
    const deviceInfoTurboModulePromise = DeviceInfoTurboModule.create(ctx)
    const statusBarTurboModule = new StatusBarTurboModule(ctx);
    const safeAreaTurboModulePromise = SafeAreaTurboModule.create(
      ctx,
      statusBarTurboModule,
    )
    return new Map()
      .set(DeviceInfoTurboModule.NAME, await deviceInfoTurboModulePromise)
      .set(SafeAreaTurboModule.NAME, await safeAreaTurboModulePromise)
      .set(StatusBarTurboModule.NAME, statusBarTurboModule)
  }

  override getAnyThreadTurboModuleFactoryByNameMap(): Map<string, (ctx: AnyThreadTurboModuleContext) => AnyThreadTurboModule | null> {
    return new Map<string, ((ctx: AnyThreadTurboModuleContext) => AnyThreadTurboModule)>()
      .set(AppearanceTurboModule.NAME, (ctx) => new AppearanceTurboModule(ctx))
      .set(NetworkingTurboModule.NAME, (ctx) => new NetworkingTurboModule(ctx))
      .set(WebSocketTurboModule.NAME, (ctx) => new WebSocketTurboModule(ctx))
      .set(BlobTurboModule.NAME, (ctx) => new BlobTurboModule(ctx))
      .set(FileReaderTurboModule.NAME, (ctx) => new FileReaderTurboModule(ctx))
      .set(LinkingManagerTurboModule.NAME, (ctx) => new LinkingManagerTurboModule(ctx))
      .set(AppStateTurboModule.NAME, (ctx) => new AppStateTurboModule(ctx))
      .set(ImageLoaderTurboModule.NAME, (ctx) => new ImageLoaderTurboModule(ctx))
      .set(PlatformColorTurboModule.NAME, (ctx) => new PlatformColorTurboModule(ctx))
      .set(PlatformConstantsTurboModule.NAME, (ctx) => new PlatformConstantsTurboModule(ctx))
      .set(SourceCodeTurboModule.NAME, (ctx) => new SourceCodeTurboModule(ctx))
  }

  override createWrappedCustomRNComponentBuilderByComponentNameMap(): Map<string, WrappedBuilder<[ComponentBuilderContext]>> {
    return new Map()
      .set(RNDrawerLayoutAndroid.NAME, wrapBuilder(buildDrawerLayoutAndroid))
      .set(RNDrawerLayoutChildWrapperView.NAME, wrapBuilder(buildRNDrawerLayoutChildWrapperView))
  }

  override getDebugName() {
    return 'rnoh';
  }
}
