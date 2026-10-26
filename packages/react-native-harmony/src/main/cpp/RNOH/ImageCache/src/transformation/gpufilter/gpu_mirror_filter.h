/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an 'AS IS' BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef IMAGE_KNIFE_C_KEKE_GPU_MIRROR_FILTER_H
#define IMAGE_KNIFE_C_KEKE_GPU_MIRROR_FILTER_H

#include "gpu_transform.h"
namespace ImageCachePro {
class GPUMirrorFilter : public GPUTransform {
 private:
  const char* GetFragmentShaderString() const override {
    const char* mirrorFShaderStr =
        "#version 300 es\n"
        "precision highp float;\n"
        "in vec2 v_texCoord;\n"
        "out vec4 FragColor;\n"

        "uniform lowp sampler2D s_TextureMap;\n"
        "void main()\n"
        "{\n"
        "vec2 mirrorCoords = v_texCoord;\n"
        "mirrorCoords.x = 1.0 - v_texCoord.x;\n"
        "FragColor = texture(s_TextureMap, mirrorCoords);\n"
        "}";

    return mirrorFShaderStr;
  }
};
} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_GPU_MIRROR_FILTER_H
