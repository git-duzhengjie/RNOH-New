/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import { RNViewBase } from '..';
import { ComponentBuilderContext, RNComponentContext } from '../../../RNOH';
import { RNDrawerLayoutChildWrapperViewDescriptor } from './types';


@Builder
export function buildRNDrawerLayoutChildWrapperView(ctx: ComponentBuilderContext) {
  RNDrawerLayoutChildWrapperView({
    ctx: ctx.rnComponentContext,
    tag: ctx.tag,
  });
}

@Component
export struct RNDrawerLayoutChildWrapperView {
  public static readonly NAME = 'DrawerLayoutChildWrapperView';
  public ctx!: RNComponentContext;
  public tag: number = 0;
  @State descriptor: RNDrawerLayoutChildWrapperViewDescriptor = {} as RNDrawerLayoutChildWrapperViewDescriptor;
  private cleanUpCallbacks: (() => void)[] = [];

  aboutToAppear() {
    this.descriptor = this.ctx.descriptorRegistry.getDescriptor<RNDrawerLayoutChildWrapperViewDescriptor>(this.tag);
    this.cleanUpCallbacks.push(this.ctx.descriptorRegistry.subscribeToDescriptorChanges(this.tag,
      (newDescriptor) => {
        this.descriptor = (newDescriptor as RNDrawerLayoutChildWrapperViewDescriptor);
      }
    ));
  }

  aboutToDisappear() {
    this.cleanUpCallbacks.forEach(cb => cb());
  }

  build() {
    Column() {
      RNViewBase({ tag: this.tag, ctx: this.ctx }) {
        ContentSlot(this.ctx.getContentForTag(this.tag));
      }
    }
    .width('100%')
    .height('100%')
    .onTouch((event: TouchEvent) => {
      if (this.descriptor.props.postTouches) {
        this.ctx.rnInstance.postTouchEventMessageToCpp(this.tag, event);
      }
    });
  }
}
