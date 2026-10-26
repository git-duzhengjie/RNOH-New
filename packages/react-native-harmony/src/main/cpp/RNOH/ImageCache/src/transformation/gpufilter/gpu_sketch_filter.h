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

#ifndef IMAGE_KNIFE_C_KEKE_GPU_SKETCH_FILTER_H
#define IMAGE_KNIFE_C_KEKE_GPU_SKETCH_FILTER_H

#include "gpu_3x3_texture_sampling_filter.h"
namespace ImageCachePro {

class GPUSketchFilter : public GPU3x3TextureSamplingFilter {
 private:
  const char* GetFragmentShaderString() const override {
    const char* sketchShaderStr =
        "#version 300 es\n"
        "precision highp float;\n"
        "layout(location = 0) out vec4 outColor;\n"
        "uniform lowp sampler2D s_TextureMap;\n"

        "in vec2 textureCoordinate;\n"
        "in vec2 leftTextureCoordinate;\n"
        "in vec2 rightTextureCoordinate;\n"
        "in vec2 topTextureCoordinate;\n"
        "in vec2 topLeftTextureCoordinate;\n"
        "in vec2 topRightTextureCoordinate;\n"
        "in vec2 bottomTextureCoordinate;\n"
        "in vec2 bottomLeftTextureCoordinate;\n"
        "in vec2 bottomRightTextureCoordinate;\n"

        "void main()\n"
        "{\n"
        "float bottomLeftIntensity = texture(s_TextureMap, bottomLeftTextureCoordinate).r;\n"
        "float topRightIntensity = texture(s_TextureMap, topRightTextureCoordinate).r;\n"
        "float topLeftIntensity = texture(s_TextureMap, topLeftTextureCoordinate).r;\n"
        "float bottomRightIntensity = texture(s_TextureMap, bottomRightTextureCoordinate).r;\n"
        "float leftIntensity = texture(s_TextureMap, leftTextureCoordinate).r;\n"
        "float rightIntensity = texture(s_TextureMap, rightTextureCoordinate).r;\n"
        "float bottomIntensity = texture(s_TextureMap, bottomTextureCoordinate).r;\n"
        "float topIntensity = texture(s_TextureMap, topTextureCoordinate).r;\n"
        "float h = -topLeftIntensity - 2.0 * topIntensity - topRightIntensity + bottomLeftIntensity\
                 + 2.0 * bottomIntensity + bottomRightIntensity;\n"
        "float v = -bottomLeftIntensity - 2.0 * leftIntensity - topLeftIntensity + bottomRightIntensity\
                 + 2.0 * rightIntensity + topRightIntensity;\n"
        "float mag = 1.0-length(vec2(h, v));\n"
        "outColor = vec4(vec3(mag), 1.0);\n"
        "}";
    return sketchShaderStr;
  }
};

} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_GPU_SKETCH_FILTER_H
