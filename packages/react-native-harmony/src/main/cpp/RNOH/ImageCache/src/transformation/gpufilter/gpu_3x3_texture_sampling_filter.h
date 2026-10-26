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

#ifndef IMAGE_KNIFE_C_KEKE_GPU_3X3_TEXTURE_SAMPLING_FILTER_H
#define IMAGE_KNIFE_C_KEKE_GPU_3X3_TEXTURE_SAMPLING_FILTER_H

#include "gpu_transform.h"
namespace ImageCachePro {

class GPU3x3TextureSamplingFilter : public GPUTransform {
 protected:
  virtual void OnDerivedUniformValue() {}

 private:
  uint32_t width_ = 0;
  uint32_t height_ = 0;

  GLuint texelWidthLoc_ = GL_NONE;
  GLuint texelHeightLoc_ = GL_NONE;

  void UniformValue() final {
    OnDerivedUniformValue();

    GLuint program = GetProgramObject();
    texelWidthLoc_ = glGetUniformLocation(program, "texelWidth");
    texelHeightLoc_ = glGetUniformLocation(program, "texelHeight");
  }

  void UniformValueWithImageSize(const uint32_t& width, const uint32_t& height)
      final {
    if (width == width_ && height == height_) {
      return;
    }
    width_ = width;
    height_ = height;

    glUniform1f(texelWidthLoc_, 1.0f / width_);
    glUniform1f(texelHeightLoc_, 1.0f / height_);
  }

  const char* GetVertexShaderString() const final {
    const char* v3x3ShaderStr =
        "#version 300 es\n"
        "precision highp float;\n"
        "layout(location = 0) in vec4 a_position;\n"
        "layout(location = 1) in vec2 a_texCoord;\n"
        "uniform highp float texelWidth;\n"
        "uniform highp float texelHeight;\n"

        "out vec2 v_texCoord;\n"
        "out vec2 leftTextureCoordinate;\n"
        "out vec2 rightTextureCoordinate;\n"
        "out vec2 topTextureCoordinate;\n"
        "out vec2 topLeftTextureCoordinate;\n"
        "out vec2 topRightTextureCoordinate;\n"
        "out vec2 bottomTextureCoordinate;\n"
        "out vec2 bottomLeftTextureCoordinate;\n"
        "out vec2 bottomRightTextureCoordinate;\n"

        "void main()\n"
        "{\n"
        "gl_Position = a_position;\n"
        "vec2 widthStep = vec2(texelWidth, 0.0);\n"
        "vec2 heightStep = vec2(0.0, texelHeight);\n"
        "vec2 widthHeightStep = vec2(texelWidth, texelHeight);\n"
        "vec2 widthNegativeHeightStep = vec2(texelWidth, -texelHeight);\n"
        "v_texCoord = a_texCoord;\n"
        "leftTextureCoordinate = a_texCoord - widthStep;\n"
        "rightTextureCoordinate = a_texCoord + widthStep;\n"

        "topTextureCoordinate = a_texCoord - heightStep;\n"
        "topLeftTextureCoordinate = a_texCoord - widthHeightStep;\n"
        "topRightTextureCoordinate = a_texCoord + widthNegativeHeightStep;\n"

        "bottomTextureCoordinate = a_texCoord +heightStep;\n"
        "bottomLeftTextureCoordinate = a_texCoord - widthNegativeHeightStep;\n"
        "bottomRightTextureCoordinate = a_texCoord + widthHeightStep;\n"
        "}";
    return v3x3ShaderStr;
  }
};

} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_GPU_3X3_TEXTURE_SAMPLING_FILTER_H
