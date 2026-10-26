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

#ifndef IMAGE_KNIFE_C_KEKE_GPU_TRANSFORM_H
#define IMAGE_KNIFE_C_KEKE_GPU_TRANSFORM_H

#include <GLES3/gl3.h>

namespace ImageCachePro {
class GPUTransform {
 public:
  bool InitGL();
  void DeleteGL();
  void Rendering(
      uint8_t* buffer,
      uint32_t width,
      uint32_t height,
      uint32_t format = GL_RGBA);
  virtual ~GPUTransform() {
    DeleteGL();
  }

 protected:
  virtual const char* GetFragmentShaderString() const = 0;
  virtual const char* GetVertexShaderString() const;
  /**
   * @brief Set the uniform variable values in the fragment shader. Executed only once during transformation.
   */
  virtual void UniformValue() {}

  /**
   * @brief Set image width and height related values in the fragment shader. Executed for every frame transformation.
   */
  virtual void UniformValueWithImageSize(
      const uint32_t& width,
      const uint32_t& height) {}

  GLuint GetProgramObject() const;

 private:
  GLuint programObj_ = GL_NONE;
  GLuint fboId_ = GL_NONE;
  GLuint vboIds_[3] = {GL_NONE};
  GLuint vaoIds_[1] = {GL_NONE};

  // Image texture
  GLuint imageTextureId_ = GL_NONE;
  // Framebuffer texture, image output
  GLuint fboTextureId_ = GL_NONE;

  void TextureInit();
  void BufferInit();
  void CreateProgram();
  GLuint CreateShader(GLenum shaderType, const char* pSource);
  void SetImageCacheData(
      uint8_t* buffer,
      const uint32_t& width,
      const uint32_t& height,
      uint32_t format);
};

} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_GPU_TRANSFORM_H
