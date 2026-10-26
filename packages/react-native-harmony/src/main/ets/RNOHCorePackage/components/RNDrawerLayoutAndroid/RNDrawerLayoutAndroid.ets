/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE-MIT file in the root directory of this source tree.
 */

import { RNViewBase } from '..';
import { ComponentBuilderContext, RNComponentContext } from '../../../RNOH';
import { RNDrawerLayoutAndroidDescriptorWrapper, DrawerPosition } from './types';


@Builder
export function buildDrawerLayoutAndroid(ctx: ComponentBuilderContext) {
  RNDrawerLayoutAndroid({
    ctx: ctx.rnComponentContext,
    tag: ctx.tag,
  });
}

function getSideBarPosition(position: DrawerPosition | undefined): SideBarPosition {
  if (position === 'right') {
    return SideBarPosition.End;
  } else {
    return SideBarPosition.Start;
  }
}

/**
 * Native component of DrawerLayoutAndroid, Due to platform limitations, not all RN props are supported.
 *
 * Currently working:
 * - drawerPosition
 * - drawerWidth
 * - drawerBackgroundColor
 * - renderNavigationView
 * - openDrawer/closeDrawer methods
 * - onDrawerOpen/onDrawerClose callbacks
 * - children
 * - styles
 *
 *  NOTE: unlike the original DrawerLayoutAndroid, the Side Bar Container can't currently be opened using swipe gestures, only programmatically using openDrawer() method.
 */

@Component
export struct RNDrawerLayoutAndroid {
  public static readonly NAME = 'AndroidDrawerLayout';
  @State isSideBarVisible: boolean = false;
  public ctx!: RNComponentContext;
  public tag: number = 0;
  @State descriptorWrapper: RNDrawerLayoutAndroidDescriptorWrapper = {} as RNDrawerLayoutAndroidDescriptorWrapper;
  private cleanUpCallbacks: (() => void)[] = [];

  aboutToAppear() {
    this.onDescriptorWrapperChange(this.ctx.descriptorRegistry.findDescriptorWrapperByTag<RNDrawerLayoutAndroidDescriptorWrapper>(this.tag)!);
    this.cleanUpCallbacks.push(this.ctx.descriptorRegistry.subscribeToDescriptorChanges(this.tag,
      (_descriptor, newDescriptorWrapper) => {
        if (newDescriptorWrapper instanceof RNDrawerLayoutAndroidDescriptorWrapper) {
          this.onDescriptorWrapperChange(newDescriptorWrapper);
        }
      }
    ));
    this.cleanUpCallbacks.push(this.ctx.componentCommandReceiver.registerCommandCallback(
      this.tag,
      (command) => {
        if (command === 'closeDrawer') {
          this.isSideBarVisible = false;
        } else if (command === 'openDrawer') {
          this.isSideBarVisible = true;
        }
      }));

  }

  private onDescriptorWrapperChange(descriptorWrapper: RNDrawerLayoutAndroidDescriptorWrapper) {
    this.descriptorWrapper = descriptorWrapper;
  }

  aboutToDisappear() {
    this.cleanUpCallbacks.forEach(cb => cb());
  }

  /**
   * SideBarContainer expects exactly two ArkTS children. Because of that we can't use ContentSlot for the C-API
   * children directly here. We also have to use wrappedRNComponentBuilder instead of wrappedRNChildrenBuilder,
   * because of the same requirement.
   */

  build() {
    SideBarContainer(SideBarContainerType.Overlay) {
      if (this.descriptorWrapper.childrenTags.length === 2) {
        this.ctx.wrappedRNComponentBuilder.builder(this.ctx, this.descriptorWrapper.childrenTags[0]);
      }
      RNViewBase({ ctx: this.ctx, tag: this.tag }) {
        if (this.descriptorWrapper.childrenTags.length === 2) {
          this.ctx.wrappedRNComponentBuilder.builder(this.ctx, this.descriptorWrapper.childrenTags[1]);
        }
      }
    }
    .sideBarWidth(this.descriptorWrapper?.drawerWidth)
    .minSideBarWidth(this.descriptorWrapper?.drawerWidth)
    .maxSideBarWidth(this.descriptorWrapper?.drawerWidth)
    .minContentWidth(0)
    .showControlButton(false)
    .sideBarPosition(getSideBarPosition(this.descriptorWrapper?.drawerPosition))
    .onChange((isOpen: boolean) => {
      if (isOpen) {
        this.ctx.rnInstance.emitComponentEvent(this.tag, 'drawerOpen', {});
      } else {
        this.ctx.rnInstance.emitComponentEvent(this.tag, 'drawerClose', {});
      }
    })
    .showSideBar($$this.isSideBarVisible)
  }
}

