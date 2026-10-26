/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "gpu_kuwahara_filter.h"

namespace ImageCachePro {

static const char* KUWAHARA_SHADER =
    "#version 300 es\n"
    "precision highp float;\n"
    "layout(location = 0) out vec4 outColor;\n"
    "in vec2 v_texCoord;\n"
    "uniform lowp sampler2D s_TextureMap;\n"
    "uniform highp float radius;\n"
    "uniform highp vec2 src_size;\n"

    "void main() {\n"
    "vec2 uv = v_texCoord;\n"
    "float n = float((radius + 1.0) * (radius + 1.0));\n"
    "int i ; int j ;\n"
    "vec3 m0 = vec3(0.0); vec3 m1 = vec3(0.0); vec3 m2 = vec3(0.0); vec3 m3 = vec3(0.0);\n"
    "vec3 s0 = vec3(0.0); vec3 s1 = vec3(0.0); vec3 s2 = vec3(0.0); vec3 s3 = vec3(0.0);\n"
    "vec3 c;\n"

    "for (j = -int(radius); j <=0; ++j) {\n"
    "for (i = -int(radius); i <=0; ++i) {\n"
    "c = texture(s_TextureMap,uv + vec2(i,j) * src_size).rgb;\n"
    "m0 += c;\n"
    "s0 += c * c;\n"
    "}\n"
    "}\n"
    "for (j = -int(radius); j <=0; ++j) {\n"
    "for (i =0; i <=int(radius); ++i) {\n"
    "c = texture(s_TextureMap,uv + vec2(i,j) * src_size).rgb;\n"
    "m1 += c;\n"
    "s1 += c * c;\n"
    "}\n"
    "}\n"
    "for (j = 0; j <=int(radius); ++j) {\n"
    "for (i = 0; i <= int(radius); ++i) {\n"
    "c = texture(s_TextureMap,uv + vec2(i,j) * src_size).rgb;\n"
    "m2 += c;\n"
    "s2 += c * c;\n"
    "}\n"
    "}\n"
    "for (j = 0; j <=int(radius); ++j) {\n"
    "for (i = -int(radius); i <= 0; ++i) {\n"
    "c = texture(s_TextureMap,uv + vec2(i,j) * src_size).rgb;\n"
    "m3 += c;\n"
    "s3 += c * c;\n"
    "}\n"
    "}\n"

    "float min_sigma2 = 1e+2;\n"
    "m0 /= n;\n"
    "s0 = abs(s0 /n - m0 * m0);\n"
    "float sigma2 = s0.r + s0.g + s0.b;\n"
    "if (sigma2 < min_sigma2) {\n"
    "min_sigma2 = sigma2;\n"
    "outColor = vec4(m0,1.0);\n"
    "}\n"

    "m1 /= n;\n"
    "s1 = abs(s1 / n -m1 * m1);\n"
    "sigma2 = s1.r + s1.g + s1.b;\n"
    "if (sigma2 < min_sigma2) {\n"
    "min_sigma2 = sigma2;\n"
    "outColor = vec4(m1,1.0);\n"
    "}\n"

    "m2 /= n;\n"
    "s2 = abs(s2 / n -m2 * m2);\n"
    "sigma2 = s2.r + s2.g + s2.b;\n"
    "if (sigma2 < min_sigma2) {\n"
    "min_sigma2 = sigma2;\n"
    "outColor = vec4(m2,1.0);\n"
    "}\n"

    "m3 /= n;\n"
    "s3 = abs(s3 / n -m3 * m3);\n"
    "sigma2 = s3.r + s3.g + s3.b;\n"
    "if (sigma2 < min_sigma2) {\n"
    "min_sigma2 = sigma2;\n"
    "outColor = vec4(m3,1.0);\n"
    "}\n"
    "}\n";

const char* GPUKuwaharaFilter::GetFragmentShaderString() const {
  return KUWAHARA_SHADER;
}

} // namespace ImageCachePro
