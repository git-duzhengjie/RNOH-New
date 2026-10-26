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

#ifndef IMAGE_KNIFE_C_KEKE_GPU_BLUR_FILTER_H
#define IMAGE_KNIFE_C_KEKE_GPU_BLUR_FILTER_H

#include "gpu_transform.h"
namespace ImageCachePro {

class GPUBlurFilter : public GPUTransform {
 public:
  GPUBlurFilter(int radius, float offsetX, float offsetY)
      : radius_(radius), offsetX_(offsetX), offsetY_(offsetY) {}

 private:
  int radius_ = 0;
  float offsetX_ = 0;
  float offsetY_ = 0;

  const char* GetFragmentShaderString() const override {
    const char* blurShaderStr =
        "#version 300 es\n"
        "precision highp float;\n"
        "uniform lowp sampler2D s_TextureMap;\n"
        "in vec2 v_texCoord;\n"
        "layout(location = 0) out vec4 outColor;\n"

        "uniform highp int blurRadius;\n"
        "uniform highp vec2 blurOffset;\n"

        "float PI = 3.1415926;\n"
        "float getWeight(int i)\n"
        "{\n"
        "float sigma = float(blurRadius) / 3.0 ;\n"
        "return (1.0 / sqrt(2.0 * PI * sigma * sigma)) * exp(-float(i * i) / (2.0 * sigma * sigma));\n"
        "}\n"

        "vec2 clampCoordinate (vec2 coordinate)\n"
        "{\n"
        "return vec2(clamp(coordinate.x, 0.0, 1.0), clamp(coordinate.y, 0.0, 1.0));\n"
        "}\n"

        "void main()\n"
        "{\n"
        "vec4 sourceColor = texture(s_TextureMap, v_texCoord);\n"
        "if (blurRadius <= 1)\n"
        "{\n"
        "outColor = sourceColor;\n"
        "return;\n"
        "}\n"
        "float weight = getWeight(0);\n"
        "vec3 finalColor = sourceColor.rgb * weight;\n"
        "for(int i = 1; i < blurRadius; i++) {\n"
        "weight = getWeight(i);\n"
        "finalColor += texture(s_TextureMap, clampCoordinate(v_texCoord\
                    - blurOffset * float(i))).rgb * weight;\n"
        "finalColor += texture(s_TextureMap, clampCoordinate(v_texCoord\
                    + blurOffset * float(i))).rgb * weight;\n"
        "}\n"
        "outColor = vec4(finalColor, sourceColor.a);\n"
        "}\n";
    return blurShaderStr;
  }

  void UniformValue() override {
    GLuint program = GetProgramObject();
    GLuint loc = glGetUniformLocation(program, "blurRadius");
    glUniform1i(loc, radius_);

    loc = glGetUniformLocation(program, "blurOffset");
    glUniform2f(loc, offsetX_, offsetY_);
  }
};
} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_GPU_BLUR_FILTER_H
