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

#ifndef IMAGE_KNIFE_C_KEKE_GPU_GRAY_SACLE_FILTER_H
#define IMAGE_KNIFE_C_KEKE_GPU_GRAY_SACLE_FILTER_H

#include "gpu_transform.h"
namespace ImageCachePro {

class GPUGrayScaleFilter : public GPUTransform {
 private:
  const char* GetFragmentShaderString() const override {
    const char* grayScaleShaderStr =
        "#version 300 es\n"
        "precision highp float;\n"
        "in vec2 v_texCoord;\n"
        "uniform lowp sampler2D s_TextureMap;\n"
        "layout(location = 0) out vec4 outColor;\n"
        "const highp vec3 W = vec3(0.2125, 0.7154, 0.0721);\n"
        "void main()\n"
        "{\n"
        "lowp vec4 textureColor = texture(s_TextureMap, v_texCoord);\n"
        "float luminance = dot(textureColor.rgb, W);\n"
        "outColor = vec4(vec3(luminance), textureColor.a);\n"
        "}";
    return grayScaleShaderStr;
  }
};

} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_GPU_GRAY_SACLE_FILTER_H
