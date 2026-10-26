/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import { Descriptor } from '../../../RNOH/ts';
import type { ViewBaseProps, ViewRawProps } from '../RNViewBase/types';
import { ViewDescriptorWrapperBase } from '../ts';

export interface RNDrawerLayoutAndroidProps extends ViewBaseProps {}

export type DrawerPosition = 'left' | 'right';

export interface RNDrawerLayoutAndroidRawProps extends ViewRawProps {
  drawerWidth: number,
  drawerPosition: DrawerPosition;
}

export interface RNDrawerLayoutAndroidState {}

export type RNDrawerLayoutAndroidDescriptor = Descriptor<'AndroidDrawerLayout', RNDrawerLayoutAndroidProps, RNDrawerLayoutAndroidState, RNDrawerLayoutAndroidRawProps>;

export class RNDrawerLayoutAndroidDescriptorWrapper extends ViewDescriptorWrapperBase<string,
RNDrawerLayoutAndroidProps,
RNDrawerLayoutAndroidState,
RNDrawerLayoutAndroidRawProps> {
  public get drawerWidth(): number | undefined {
    return this.rawProps.drawerWidth;
  }

  public get drawerPosition(): DrawerPosition {
    return this.rawProps.drawerPosition;
  }
}


interface RNDrawerLayoutChildWrapperViewProps extends ViewBaseProps {
  postTouches: boolean;
}

export type RNDrawerLayoutChildWrapperViewDescriptor = Descriptor<'DrawerLayoutChildWrapperView', RNDrawerLayoutChildWrapperViewProps>;