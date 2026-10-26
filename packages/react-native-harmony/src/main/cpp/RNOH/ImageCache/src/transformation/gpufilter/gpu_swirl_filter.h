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

#ifndef IMAGE_KNIFE_C_KEKE_GPU_SWIRL_FILTER_H
#define IMAGE_KNIFE_C_KEKE_GPU_SWIRL_FILTER_H

#include "gpu_transform.h"
namespace ImageCachePro {

class GPUSwirlFilter : public GPUTransform {
 public:
  GPUSwirlFilter(float radius, float angle, float centerX, float centerY)
      : radius_(radius), angle_(angle), centerX_(centerX), centerY_(centerY) {}

 private:
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  GLuint texSizeLoc_ = GL_NONE;
  GLuint centerLoc_ = GL_NONE;

  float radius_ = 0;
  float angle_ = 0;
  float centerX_ = 0;
  float centerY_ = 0;

  const char* GetFragmentShaderString() const override {
    const char* swirlFShaderStr =
        "#version 300 es\n"
        "precision highp float;\n"
        "layout(location = 0) out vec4 outColor;\n"
        "in vec2 v_texCoord;\n"

        "uniform lowp sampler2D s_TextureMap;\n"
        "uniform vec2 u_texSize;\n"
        "uniform highp float radius;\n"
        "uniform highp float angle;\n"
        "uniform vec2 center;\n"

        "void main() {\n"
        "vec2 tc = v_texCoord * u_texSize;\n"
        "tc -= center;\n"
        "float dist = length(tc);\n"
        "if (dist < radius) {\n"
        "float percent = (radius - dist) / radius;\n"
        "float theta = percent * percent * angle * 8.0;\n"
        "float s = sin(theta);\n"
        "float c = cos(theta);\n"
        "tc = vec2(dot(tc, vec2(c, -s)), dot(tc, vec2(s, c)));\n"
        "}\n"
        "tc += center;\n"
        "outColor = texture(s_TextureMap, tc / u_texSize);\n"
        "}";
    return swirlFShaderStr;
  }

  void UniformValue() override {
    GLuint program = GetProgramObject();
    GLuint loc = glGetUniformLocation(program, "radius");
    glUniform1f(loc, radius_);

    loc = glGetUniformLocation(program, "angle");
    glUniform1f(loc, angle_);

    centerLoc_ = glGetUniformLocation(program, "center");
    texSizeLoc_ = glGetUniformLocation(program, "u_texSize");
  }

  void UniformValueWithImageSize(const uint32_t& width, const uint32_t& height)
      override {
    if (width == width_ && height == height_) {
      return;
    }
    width_ = width;
    height_ = width;

    glUniform2f(texSizeLoc_, width_, height_);
    glUniform2f(centerLoc_, centerX_ * width_, centerY_ * height_);
  }
};

} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_GPU_SWIRL_FILTER_H
