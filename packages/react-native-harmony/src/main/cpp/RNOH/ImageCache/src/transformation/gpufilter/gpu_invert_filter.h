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

#ifndef IMAGE_KNIFE_C_KEKE_GPU_INVERT_FILTER_H
#define IMAGE_KNIFE_C_KEKE_GPU_INVERT_FILTER_H

#include "gpu_transform.h"
namespace ImageCachePro {

class GPUInvertFilter : public GPUTransform {
 private:
  const char* GetFragmentShaderString() const override {
    const char* invertFShaderStr =
        "#version 300 es\n"
        "precision highp float;\n"
        "in vec2 v_texCoord;\n"

        "uniform lowp sampler2D s_TextureMap;\n"
        "layout(location = 0) out vec4 outColor;\n"

        "void main()\n"
        "{\n"
        "lowp vec4 textureColor = texture(s_TextureMap, v_texCoord);\n"
        "\n"
        "outColor = vec4((1.0 - textureColor.rgb), textureColor.w);\n"
        "}";
    return invertFShaderStr;
  }
};

} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_GPU_INVERT_FILTER_H
