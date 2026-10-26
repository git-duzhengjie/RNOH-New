/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import {
  RNOHContext,
  RNComponentContext,
  DescriptorWrapper,
  CustomComponentBuilder,
  ComponentBuilderContext,
  DescriptorEssence,
} from '../../RNOH';
import { Descriptor } from '../../RNOH/DescriptorBase';

/**
 * @internal
 * Used by RNOH to build custom native components implemented in ArkTS
 */
@Builder
export function buildRNComponent(ctx: RNComponentContext, descriptorWrapper: DescriptorWrapper) {
  if (ctx.rnInstance.getComponentNameFromDescriptorType(descriptorWrapper.type_) !== '') {
    ctx.wrappedCustomRNComponentBuilder.builder(new ComponentBuilderContext(
      ctx,
      ctx,
      descriptorWrapper.tag,
      ctx.rnInstance.getComponentNameFromDescriptorType(descriptorWrapper.type_),
      ctx.descriptorRegistry.getDescriptor<Descriptor>(descriptorWrapper.tag) as DescriptorEssence,
    ));
  }
}

/**
 * @deprecated (latestRNOHVersion: 0.72.40).
 *
 * NOTE: It was actually deprecated in 0.72.22, but this component was heavily utilized.
 * Removing it may still break some libraries. Instead of removing it completely, it is changed to be NOOP, and removal is postponed.
 */
@Component
export struct RNComponentFactory {
  public ctx!: RNOHContext;
  public tag: number = 0;
  @BuilderParam public buildCustomComponent!: CustomComponentBuilder;

  build() {
  }
}