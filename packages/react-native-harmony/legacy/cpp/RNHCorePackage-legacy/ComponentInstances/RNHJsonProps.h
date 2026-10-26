/**
 * RNOH-New: 轻量 JSON 解析辅助工具
 *
 * 提供极简的 JSON 属性提取能力，仅支持从字符串中提取
 * 字符串、数字、布尔值等基本类型。适用于 props 更新场景。
 *
 * TODO: 后续替换为 nlohmann/json 或其他完整 JSON 库
 */

#pragma once

#include <string>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <algorithm>

namespace rnoh_new {

/**
 * 轻量 JSON 属性提取器
 *
 * 用法：
 *   RNHJsonProps props(jsonString);
 *   std::string bg = props.getString("backgroundColor", "#FFFFFF");
 *   float opacity = props.getFloat("opacity", 1.0f);
 */
class RNHJsonProps {
public:
    explicit RNHJsonProps(const std::string& json) : json_(json) {}

    /**
     * 提取字符串属性值
     * @param key JSON 键名
     * @param defaultVal 未找到时的默认值
     * @return 字符串值
     */
    std::string getString(const std::string& key, const std::string& defaultVal = "") const {
        std::string searchKey = "\"" + key + "\"";
        size_t pos = json_.find(searchKey);
        if (pos == std::string::npos) return defaultVal;

        // 跳过键名和冒号
        pos = json_.find(':', pos + searchKey.length());
        if (pos == std::string::npos) return defaultVal;
        pos++;

        // 跳过空白
        while (pos < json_.length() && (json_[pos] == ' ' || json_[pos] == '\t')) pos++;

        if (pos >= json_.length()) return defaultVal;

        // 字符串值
        if (json_[pos] == '"') {
            pos++;
            size_t end = json_.find('"', pos);
            if (end == std::string::npos) return defaultVal;
            return json_.substr(pos, end - pos);
        }

        // 非字符串值（数字/布尔），原样返回
        size_t end = pos;
        while (end < json_.length() && json_[end] != ',' && json_[end] != '}' &&
               json_[end] != ' ' && json_[end] != '\t' && json_[end] != '\n') {
            end++;
        }
        return json_.substr(pos, end - pos);
    }

    /**
     * 提取浮点数属性值
     * @param key JSON 键名
     * @param defaultVal 未找到时的默认值
     * @return 浮点数值
     */
    float getFloat(const std::string& key, float defaultVal = 0.0f) const {
        std::string val = getString(key, "");
        if (val.empty()) return defaultVal;
        try {
            return std::stof(val);
        } catch (...) {
            return defaultVal;
        }
    }

    /**
     * 提取整数属性值
     * @param key JSON 键名
     * @param defaultVal 未找到时的默认值
     * @return 整数值
     */
    int getInt(const std::string& key, int defaultVal = 0) const {
        std::string val = getString(key, "");
        if (val.empty()) return defaultVal;
        try {
            return std::stoi(val);
        } catch (...) {
            return defaultVal;
        }
    }

    /**
     * 提取布尔属性值
     * @param key JSON 键名
     * @param defaultVal 未找到时的默认值
     * @return 布尔值
     */
    bool getBool(const std::string& key, bool defaultVal = false) const {
        std::string val = getString(key, "");
        if (val.empty()) return defaultVal;
        return (val == "true" || val == "1");
    }

    /**
     * 检查属性是否存在
     * @param key JSON 键名
     * @return 是否存在
     */
    bool has(const std::string& key) const {
        std::string searchKey = "\"" + key + "\"";
        return json_.find(searchKey) != std::string::npos;
    }

    /**
     * 解析 CSS 颜色字符串为 uint32_t (ARGB)
     * 支持格式: "#RRGGBB", "#AARRGGBB", "#RGB", "red", "blue" 等
     * @param colorStr 颜色字符串
     * @return ARGB 颜色值
     */
    static uint32_t parseColor(const std::string& colorStr) {
        if (colorStr.empty()) return 0xFF000000; // 默认黑色

        // 去除首尾空白
        std::string color = colorStr;
        color.erase(0, color.find_first_not_of(" \t"));
        color.erase(color.find_last_not_of(" \t") + 1);

        if (color.empty()) return 0xFF000000;

        // #RRGGBB 或 #AARRGGBB
        if (color[0] == '#') {
            std::string hex = color.substr(1);
            if (hex.length() == 3) {
                // #RGB -> #RRGGBB
                std::string expanded;
                for (char c : hex) {
                    expanded += c;
                    expanded += c;
                }
                hex = expanded;
            }
            if (hex.length() == 6) {
                hex = "FF" + hex; // 添加不透明 alpha
            }
            if (hex.length() == 8) {
                try {
                    return static_cast<uint32_t>(std::stoul(hex, nullptr, 16));
                } catch (...) {
                    return 0xFF000000;
                }
            }
            return 0xFF000000;
        }

        // 常见颜色名称映射
        if (color == "red")       return 0xFFFF0000;
        if (color == "green")     return 0xFF00FF00;
        if (color == "blue")      return 0xFF0000FF;
        if (color == "white")     return 0xFFFFFFFF;
        if (color == "black")     return 0xFF000000;
        if (color == "yellow")    return 0xFFFFFF00;
        if (color == "cyan")      return 0xFF00FFFF;
        if (color == "magenta")   return 0xFFFF00FF;
        if (color == "gray" || color == "grey") return 0xFF808080;
        if (color == "orange")    return 0xFFFFA500;
        if (color == "purple")    return 0xFF800080;
        if (color == "pink")      return 0xFFFFC0CB;
        if (color == "brown")     return 0xFFA52A2A;
        if (color == "transparent") return 0x00000000;

        // rgba(r, g, b, a) 格式（简单解析）
        if (color.find("rgba(") == 0 || color.find("rgb(") == 0) {
            size_t start = color.find('(') + 1;
            size_t end = color.find(')');
            if (end == std::string::npos) return 0xFF000000;
            std::string inner = color.substr(start, end - start);

            int r = 0, g = 0, b = 0;
            float a = 1.0f;
            int count = 0;
            size_t prevPos = 0;
            for (size_t i = 0; i <= inner.length() && count < 4; i++) {
                if (i == inner.length() || inner[i] == ',') {
                    std::string part = inner.substr(prevPos, i - prevPos);
                    part.erase(0, part.find_first_not_of(" \t"));
                    part.erase(part.find_last_not_of(" \t") + 1);
                    try {
                        if (count == 0) r = std::stoi(part);
                        else if (count == 1) g = std::stoi(part);
                        else if (count == 2) b = std::stoi(part);
                        else if (count == 3) a = std::stof(part);
                    } catch (...) {}
                    count++;
                    prevPos = i + 1;
                }
            }
            uint8_t alpha = static_cast<uint8_t>(a * 255.0f);
            return (static_cast<uint32_t>(alpha) << 24) |
                   (static_cast<uint32_t>(r & 0xFF) << 16) |
                   (static_cast<uint32_t>(g & 0xFF) << 8) |
                   (static_cast<uint32_t>(b & 0xFF));
        }

        return 0xFF000000; // 未知格式返回黑色
    }

private:
    std::string json_;
};

} // namespace rnoh_new
