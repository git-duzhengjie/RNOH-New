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

#ifndef IMAGE_KNIFE_C_KEKE_GPU_SEPIA_FILTER_H
#define IMAGE_KNIFE_C_KEKE_GPU_SEPIA_FILTER_H

#include "gpu_transform.h"
namespace ImageCachePro {

class GPUSepiaFilter : public GPUTransform {
 public:
  explicit GPUSepiaFilter(float intensity) : intensity_(intensity) {}

 private:
  float intensity_;

  const char* GetFragmentShaderString() const override {
    const char* colorMatrixFShaderStr =
        "#version 300 es\n"
        "precision highp float;\n"
        "in vec2 v_texCoord;\n"
        "uniform lowp sampler2D s_TextureMap;\n"
        "layout(location = 0) out vec4 outColor;\n"

        "uniform lowp mat4 colorMatrix;\n"
        "uniform lowp float intensity;\n"

        "void main()\n"
        "{\n"
        "lowp vec4 textureColor = texture(s_TextureMap, v_texCoord);\n"
        "lowp vec4 outputColor = textureColor * colorMatrix;\n"
        "outColor = (intensity * outputColor) + ((1.0 - intensity) * textureColor);\n"
        "}";
    return colorMatrixFShaderStr;
  }

  void UniformValue() override {
    const float colorMatrix[20] = {
        0.3588,
        0.7044,
        0.1368,
        0.0,
        0.2990,
        0.5870,
        0.1140,
        0.0,
        0.2392,
        0.4696,
        0.0912,
        0.0,
        0.0,
        0.0,
        0.0,
        1.0};

    GLuint program = GetProgramObject();
    GLuint loc = glGetUniformLocation(program, "intensity");
    glUniform1f(loc, intensity_);

    loc = glGetUniformLocation(program, "colorMatrix");
    glUniformMatrix4fv(loc, 1, false, colorMatrix);
  }
};
} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_GPU_SEPIA_FILTER_H
