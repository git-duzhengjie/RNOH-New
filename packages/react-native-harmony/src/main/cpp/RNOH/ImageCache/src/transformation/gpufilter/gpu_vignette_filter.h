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

#ifndef IMAGE_KNIFE_C_KEKE_GPU_VIGNETTE_FILTER_H
#define IMAGE_KNIFE_C_KEKE_GPU_VIGNETTE_FILTER_H

#include "gpu_transform.h"
namespace ImageCachePro {

class GPUVignetteFilter : public GPUTransform {
 public:
  GPUVignetteFilter(
      float centerX,
      float centerY,
      float red,
      float green,
      float blue,
      float start,
      float end)
      : centerX_(centerX),
        centerY_(centerY),
        red_(red),
        green_(green),
        blue_(blue),
        start_(start),
        end_(end) {}

 private:
  float centerX_ = 0;
  float centerY_ = 0;
  float red_ = 0;
  float green_ = 0;
  float blue_ = 0;
  float start_ = 0;
  float end_ = 0;

  const char* GetFragmentShaderString() const override {
    const char* vignetteFShaderStr =
        "#version 300 es\n"
        "precision highp float;\n"
        "in vec2 v_texCoord;\n"
        "layout(location = 0) out vec4 outColor;\n"

        "uniform lowp sampler2D s_TextureMap;\n"
        "uniform lowp vec2 vignetteCenter;\n"
        "uniform lowp vec3 vignetteColor;\n"
        "uniform highp float vignetteStart;\n"
        "uniform highp float vignetteEnd;\n"

        "void main()\n"
        "{\n"
        "lowp vec3 rgb = texture(s_TextureMap, v_texCoord).rgb;\n"
        "lowp float d = distance(v_texCoord, vec2(0.5,0.5));\n"
        "rgb *= (1.0 - smoothstep(vignetteStart , vignetteEnd, d));\n"
        "outColor = vec4(vec3(rgb), 1.0);\n"

        "lowp vec3 rgb2 = texture(s_TextureMap, v_texCoord).rgb;\n"
        "lowp float d2 = distance(v_texCoord, vec2(vignetteCenter.x, vignetteCenter.y));\n"
        "lowp float percent = smoothstep(vignetteStart, vignetteEnd, d2);\n"
        "outColor = vec4(mix(rgb2.x,vignetteColor.x,percent), mix(rgb2.y, vignetteColor.y, percent),\
                mix(rgb2.z, vignetteColor.z, percent), 1.0);\n"
        "}";

    return vignetteFShaderStr;
  }

  void UniformValue() override {
    GLuint program = GetProgramObject();
    GLuint loc = glGetUniformLocation(program, "vignetteCenter");
    glUniform2f(loc, centerX_, centerY_);

    loc = glGetUniformLocation(program, "vignetteColor");
    glUniform3f(loc, red_, green_, blue_);

    loc = glGetUniformLocation(program, "vignetteStart");
    glUniform1f(loc, start_);

    loc = glGetUniformLocation(program, "vignetteEnd");
    glUniform1f(loc, end_);
  }
};

} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_GPU_VIGNETTER_FILTER_H
