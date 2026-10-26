/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ImageLoaderTurboModule.h"
#include "RNOH/ArkTSTurboModule.h"

using namespace rnoh;
using namespace facebook;

rnoh::ImageLoaderTurboModule::ImageLoaderTurboModule(
    const ArkTSTurboModule::Context ctx,
    const std::string name)
    : rnoh::ArkTSTurboModule(ctx, name) {
  methodMap_ = {
      ARK_METHOD_METADATA(abortRequest, 1),
      ARK_METHOD_METADATA(getConstants, 0),
      ARK_ASYNC_METHOD_METADATA(getSize, 1),
      ARK_ASYNC_METHOD_METADATA(getSizeWithHeaders, 2),
      ARK_ASYNC_METHOD_METADATA(prefetchImage, 2),
      ARK_ASYNC_METHOD_METADATA(prefetchImageWithMetadata, 3),
      ARK_ASYNC_METHOD_METADATA(queryCache, 1)};
}
