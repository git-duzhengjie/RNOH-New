/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "JsiConversions.h"
#include "RNOH/ArkJS.h"

namespace rnoh {

napi_value jsiToNapi(napi_env env, jsi::Runtime& rt, const jsi::Value& value) {
  ArkJS arkJS(env);

  if (value.isUndefined()) {
    return arkJS.getUndefined();
  } else if (value.isNull()) {
    return arkJS.getNull();
  } else if (value.isBool()) {
    return arkJS.createBoolean(value.getBool());
  } else if (value.isNumber()) {
    return arkJS.createDouble(value.getNumber());
  } else if (value.isString()) {
    return arkJS.createString(value.getString(rt).utf8(rt));
  } else if (value.isObject()) {
    auto object = value.getObject(rt);
    if (object.isArray(rt)) {
      auto array = object.getArray(rt);
      auto arrayValues = std::vector<napi_value>();
      for (size_t i = 0; i < array.size(rt); i++) {
        auto item = array.getValueAtIndex(rt, i);
        auto napiItem = jsiToNapi(env, rt, item);
        arrayValues.push_back(napiItem);
      }
      auto napiArray = arkJS.createArray(arrayValues);
      return napiArray;
    } else if (object.isFunction(rt)) {
      // TODO: Implement
      return arkJS.getUndefined();
    } else {
      auto propertyNames = object.getPropertyNames(rt);
      auto napiObject = arkJS.createObjectBuilder();
      for (size_t i = 0; i < propertyNames.size(rt); i++) {
        auto propertyName =
            propertyNames.getValueAtIndex(rt, i).asString(rt).utf8(rt);
        auto propertyValue = object.getProperty(rt, propertyName.c_str());
        auto napiPropertyValue = jsiToNapi(env, rt, propertyValue);
        napiObject.addProperty(propertyName.c_str(), napiPropertyValue);
      }
      return napiObject.build();
    }
  }
  return napi_value();
}

jsi::Value napiToJsi(napi_env env, jsi::Runtime& rt, napi_value value) {
  ArkJS arkJS(env);

  switch (arkJS.getType(value)) {
    case napi_undefined:
      return jsi::Value::undefined();
    case napi_null:
      return jsi::Value::null();
    case napi_boolean:
      return jsi::Value(arkJS.getBoolean(value));
    case napi_number:
      return jsi::Value(arkJS.getDouble(value));
    case napi_string:
      return jsi::Value(
          rt, jsi::String::createFromUtf8(rt, arkJS.getString(value)));
    case napi_object: {
      jsi::Object result(rt);
      auto properties = arkJS.getObjectProperties(value);
      for (const auto& [key, value] : properties) {
        auto stringKey = arkJS.getString(key);
        auto jsiValue = napiToJsi(env, rt, value);
        result.setProperty(rt, stringKey.c_str(), jsiValue);
      }
      return jsi::Value(std::move(result));
    }
    default:
      return jsi::Value::undefined();
  }
}

} // namespace rnoh