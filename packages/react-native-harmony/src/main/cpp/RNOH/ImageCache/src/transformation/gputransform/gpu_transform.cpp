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

#include "gpu_transform.h"
#include "ImageCacheLog.h"

namespace ImageCachePro {

// Vertex coordinates
const static GLfloat vVertices[] = {
    -1.0f,
    1.0f,
    0.0f, // top left
    1.0f,
    1.0f,
    0.0f, // top right
    -1.0f,
    -1.0f,
    0.0f, // bottom left
    1.0f,
    -1.0f,
    0.0f, // bottom right
};

// FBO texture coordinates
const static GLfloat vFboTexCoors[] = {
    0.0f,
    0.0f, // bottom left
    1.0f,
    0.0f, // bottom right
    0.0f,
    1.0f, // top left
    1.0f,
    1.0f, // top right
};

// Normal texture coordinates
const static GLfloat vTexCoors[] = {
    0.0f,
    1.0f, // bottom left
    1.0f,
    1.0f, // bottom right
    0.0f,
    0.0f, // top left
    1.0f,
    0.0f, // top right
};

const static GLushort indices[] = {0, 1, 2, 1, 3, 2};

const static int DEFAULT_ZERO = 0;
const static int DEFAULT_ONE = 1;
const static int DEFAULT_TWO = 2;
const static int DEFAULT_THREE = 3;
const static int DEFAULT_FOUR = 4;

const static int VERTEX_POS_LOC = 0;
const static int TEXTURE_POS_LOC = 1;

const char* GPUTransform::GetVertexShaderString() const {
  const char* vShaderStr =
      "#version 300 es\n"
      "layout(location = 0) in vec4 a_position;\n"
      "layout(location = 1) in vec2 a_texCoord;\n"
      "out vec2 v_texCoord;\n"
      "void main()\n"
      "{\n"
      "    gl_Position = a_position;\n"
      "    v_texCoord = a_texCoord;\n"
      "}\n";
  return vShaderStr;
}

void GPUTransform::TextureInit() {
  glGenTextures(1, &imageTextureId_);
  glBindTexture(GL_TEXTURE_2D, imageTextureId_);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glGenTextures(1, &fboTextureId_);
  glBindTexture(GL_TEXTURE_2D, fboTextureId_);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void GPUTransform::BufferInit() {
  glGenBuffers(DEFAULT_THREE, vboIds_);
  glBindBuffer(GL_ARRAY_BUFFER, vboIds_[DEFAULT_ZERO]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), vVertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, vboIds_[DEFAULT_ONE]);
  glBufferData(
      GL_ARRAY_BUFFER, sizeof(vFboTexCoors), vTexCoors, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIds_[DEFAULT_TWO]);
  glBufferData(
      GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  glGenVertexArrays(DEFAULT_ONE, vaoIds_);
  glBindVertexArray(vaoIds_[DEFAULT_ZERO]);

  glBindBuffer(GL_ARRAY_BUFFER, vboIds_[DEFAULT_ZERO]);
  glEnableVertexAttribArray(VERTEX_POS_LOC);
  glVertexAttribPointer(
      VERTEX_POS_LOC,
      DEFAULT_THREE,
      GL_FLOAT,
      GL_FALSE,
      DEFAULT_THREE * sizeof(GLfloat),
      (const void*)DEFAULT_ZERO);

  glBindBuffer(GL_ARRAY_BUFFER, vboIds_[DEFAULT_ONE]);
  glEnableVertexAttribArray(TEXTURE_POS_LOC);
  glVertexAttribPointer(
      TEXTURE_POS_LOC,
      DEFAULT_TWO,
      GL_FLOAT,
      GL_FALSE,
      DEFAULT_TWO * sizeof(GLfloat),
      (const void*)DEFAULT_ZERO);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIds_[DEFAULT_TWO]);
  // Create FBO
  glGenFramebuffers(1, &fboId_);
}

void GPUTransform::CreateProgram() {
  GLuint vertexShader = CreateShader(GL_VERTEX_SHADER, GetVertexShaderString());
  GLuint fragmentShader =
      CreateShader(GL_FRAGMENT_SHADER, GetFragmentShaderString());

  IMAGE_CACHE_CHECK(vertexShader == GL_NONE, "Create Vertex Shader Failed");
  IMAGE_CACHE_CHECK(fragmentShader == GL_NONE, "Create Fragment Shader Failed");

  programObj_ = glCreateProgram();
  IMAGE_CACHE_CHECK(!programObj_, "Create GL Program Failed");
  // Link GL Program
  glAttachShader(programObj_, vertexShader);
  glAttachShader(programObj_, fragmentShader);
  glLinkProgram(programObj_);

  // Detach and delete shaders after linking
  glDetachShader(programObj_, vertexShader);
  glDeleteShader(vertexShader);
  glDetachShader(programObj_, fragmentShader);
  glDeleteShader(fragmentShader);

  GLint linkStatus = GL_FALSE;
  glGetProgramiv(programObj_, GL_LINK_STATUS, &linkStatus);
  if (linkStatus != GL_TRUE) {
    GLint infoLen = 0;
    glGetProgramiv(programObj_, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen > 0) {
      char* buf = (char*)malloc(infoLen);
      glGetProgramInfoLog(programObj_, infoLen, NULL, buf);
      LOG(ERROR) << "GL Link Program Failed:" << buf;
      free(buf);
    }
    glDeleteProgram(programObj_);
    programObj_ = GL_NONE;
  } else {
    glUseProgram(programObj_);
  }
}

GLuint GPUTransform::CreateShader(GLenum shaderType, const char* pSource) {
  GLuint shader = GL_NONE;
  shader = glCreateShader(shaderType);
  if (shader) {
    glShaderSource(shader, 1, &pSource, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_NONE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
      GLint infoLen = 0;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
      if (infoLen > 0) {
        char* buf = (char*)malloc(infoLen);
        glGetShaderInfoLog(shader, infoLen, NULL, buf);
        LOG(ERROR) << "GL Compile Shader Failed:" << buf;
        free(buf);
      }
      glDeleteShader(shader);
      shader = GL_NONE;
    }
  }
  return shader;
}

bool GPUTransform::InitGL() {
  TextureInit();
  BufferInit();
  CreateProgram();
  IMAGE_CACHE_CHECK_RETURN(imageTextureId_ == GL_NONE, false);
  IMAGE_CACHE_CHECK_RETURN(fboId_ == GL_NONE, false);
  IMAGE_CACHE_CHECK_RETURN(fboTextureId_ == GL_NONE, false);
  IMAGE_CACHE_CHECK_RETURN(vaoIds_[0] == GL_NONE, false);
  IMAGE_CACHE_CHECK_RETURN(programObj_ == GL_NONE, false);

  // Texture sampler is fixed to bind Texture0
  GLint SamplerLoc = glGetUniformLocation(programObj_, "s_TextureMap");
  glUniform1i(SamplerLoc, 0);

  UniformValue();
  return true;
}

void GPUTransform::Rendering(
    uint8_t* buffer,
    uint32_t width,
    uint32_t height,
    uint32_t format) {
  IMAGE_CACHE_CHECK(buffer == nullptr);
  IMAGE_CACHE_CHECK(width == 0 || height == 0);
  UniformValueWithImageSize(width, height);
  SetImageCacheData(buffer, width, height, format);

  // Rendering
  const int drawSize = 6;
  glViewport(0, 0, width, height);
  glBindVertexArray(vaoIds_[0]);
  glDrawElements(GL_TRIANGLES, drawSize, GL_UNSIGNED_SHORT, (const void*)0);
  glBindVertexArray(GL_NONE); // Unbind vertex array to prevent upside-down image reading

  // write back data
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  // Consider using PBO to reduce CPU blocking and improve read efficiency,
  // but at the cost of increased VRAM consumption.
  glReadPixels(0, 0, width, height, format, GL_UNSIGNED_BYTE, buffer);
}

void GPUTransform::SetImageCacheData(
    uint8_t* buffer,
    const uint32_t& width,
    const uint32_t& height,
    uint32_t format) {
  glBindFramebuffer(GL_FRAMEBUFFER, fboId_);
  glBindTexture(GL_TEXTURE_2D, fboTextureId_);
  glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTextureId_, 0);
  glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGBA8,
      width,
      height,
      0,
      format,
      GL_UNSIGNED_BYTE,
      nullptr);

  glBindTexture(GL_TEXTURE_2D, imageTextureId_);
  glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGBA8,
      width,
      height,
      0,
      format,
      GL_UNSIGNED_BYTE,
      buffer);
  glActiveTexture(GL_TEXTURE0);
}

GLuint GPUTransform::GetProgramObject() const {
  return programObj_;
}

void GPUTransform::DeleteGL() {
  if (programObj_ != GL_NONE) {
    glDeleteProgram(programObj_);
    programObj_ = GL_NONE;
  }

  if (imageTextureId_ != GL_NONE) {
    glDeleteTextures(DEFAULT_ONE, &imageTextureId_);
    imageTextureId_ = GL_NONE;
  }

  if (fboTextureId_ != GL_NONE) {
    glDeleteTextures(DEFAULT_ONE, &fboTextureId_);
    fboTextureId_ = GL_NONE;
  }

  if (vboIds_[DEFAULT_ZERO] != GL_NONE) {
    glDeleteBuffers(DEFAULT_THREE, vboIds_);
    vboIds_[DEFAULT_ZERO] = GL_NONE;
    vboIds_[DEFAULT_ONE] = GL_NONE;
    vboIds_[DEFAULT_TWO] = GL_NONE;
  }

  if (vaoIds_[DEFAULT_ZERO] != GL_NONE) {
    glDeleteVertexArrays(DEFAULT_ONE, vaoIds_);
    vaoIds_[DEFAULT_ZERO] = GL_NONE;
  }

  if (fboId_ != GL_NONE) {
    glDeleteFramebuffers(1, &fboId_);
    fboId_ = GL_NONE;
  }
}

} // namespace ImageCachePro
