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
#ifndef IMAGE_KNIFE_C_KEKE_GPU_TOON_FILTER_H
#define IMAGE_KNIFE_C_KEKE_GPU_TOON_FILTER_H

#include "gpu_3x3_texture_sampling_filter.h"
namespace ImageCachePro {

class GPUToonFilter : public GPU3x3TextureSamplingFilter {
 public:
  GPUToonFilter(float threshold, float levels)
      : threshold_(threshold), quantizationLevels_(levels) {}

 private:
  float threshold_ = 0;
  float quantizationLevels_ = 0;

  void OnDerivedUniformValue() override {
    GLuint program = GetProgramObject();
    GLuint loc = glGetUniformLocation(program, "threshold");
    glUniform1f(loc, threshold_);

    loc = glGetUniformLocation(program, "quantizationLevels");
    glUniform1f(loc, quantizationLevels_);
  }

  const char* GetFragmentShaderString() const override {
    const char* toonFShaderStr =
        "#version 300 es\n"
        "precision highp float;\n"
        "in vec2 v_texCoord;\n"
        "layout(location = 0) out vec4 outColor;\n"
        "in vec2 leftTextureCoordinate;\n"
        "in vec2 rightTextureCoordinate;\n"
        "in vec2 topTextureCoordinate;\n"
        "in vec2 topLeftTextureCoordinate;\n"
        "in vec2 topRightTextureCoordinate;\n"
        "in vec2 bottomTextureCoordinate;\n"
        "in vec2 bottomLeftTextureCoordinate;\n"
        "in vec2 bottomRightTextureCoordinate;\n"

        "uniform lowp sampler2D s_TextureMap;\n"
        "uniform highp float threshold;\n"
        "uniform highp float quantizationLevels;\n"

        "const highp vec3 W = vec3(0.2125, 0.7154, 0.0721);\n"
        "void main()\n"
        "{\n"
        "vec4 textureColor = texture(s_TextureMap, v_texCoord);\n"
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
        "float mag = length(vec2(h, v));\n"

        "vec3 posterizedImageColor = floor((textureColor.rgb * quantizationLevels) + 0.5)\
                / quantizationLevels;\n"
        "float thresholdTest = 1.0 - step(threshold, mag);\n"

        "outColor = vec4(posterizedImageColor * thresholdTest, textureColor.a);\n"
        "}";

    return toonFShaderStr;
  }
};

} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_GPU_TOON_FILTER_H
