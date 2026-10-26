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

#ifndef IMAGE_KNIFE_C_KEKE_GPU_PIXELATION_FILTER_H
#define IMAGE_KNIFE_C_KEKE_GPU_PIXELATION_FILTER_H

#include "gpu_transform.h"
namespace ImageCachePro {

class GPUPixelationFilter : public GPUTransform {
 public:
  GPUPixelationFilter(float pixel) : pixel_(pixel) {}

 private:
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  float pixel_ = 0;
  GLuint widthFactorLoc_ = GL_NONE;
  GLuint heightFactorLoc_ = GL_NONE;

  const char* GetFragmentShaderString() const override {
    const char* pixelFShaderStr =
        "#version 300 es\n"
        "precision highp float;\n"
        "in vec2 v_texCoord;\n"

        "uniform float imageWidthFactor;\n"
        "uniform float imageHeightFactor;\n"
        "uniform lowp sampler2D s_TextureMap;\n"
        "uniform float pixel;\n"
        "layout(location = 0) out vec4 outColor;\n"

        "void main()\n"
        "{\n"
        "vec2 uv = v_texCoord.xy;\n"
        "float dx = pixel * imageWidthFactor;\n"
        "float dy = pixel * imageHeightFactor;\n"
        "vec2 coord = vec2(dx * floor(uv.x / dx), dy * floor(uv.y / dy));\n"
        "vec3 tc = texture(s_TextureMap, coord).xyz;\n"
        "outColor = vec4(tc, 1.0);\n"
        "}";
    return pixelFShaderStr;
  }

  void UniformValue() override {
    GLuint program = GetProgramObject();
    GLuint loc = glGetUniformLocation(program, "pixel");
    glUniform1f(loc, pixel_);

    widthFactorLoc_ = glGetUniformLocation(program, "imageWidthFactor");
    heightFactorLoc_ = glGetUniformLocation(program, "imageHeightFactor");
  }

  void UniformValueWithImageSize(const uint32_t& width, const uint32_t& height)
      override {
    if (width == width_ && height == height_) {
      return;
    }
    width_ = width;
    height_ = height;

    glUniform1f(widthFactorLoc_, 1.0f / width_);
    glUniform1f(heightFactorLoc_, 1.0f / height_);
  }
};

} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_GPU_PIXELATION_FILTER_H
