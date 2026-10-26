/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <js_native_api.h>
#include <js_native_api_types.h>
#include <node_api.h>
#include <cstring>

extern "C" {

napi_status napi_get_undefined(napi_env, napi_value* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_get_null(napi_env, napi_value* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_get_boolean(napi_env, bool v, napi_value* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_create_int32(napi_env, int32_t, napi_value* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_create_double(napi_env, double, napi_value* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_create_string_utf8(napi_env, const char*, size_t, napi_value* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_create_object(napi_env, napi_value* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_create_array(napi_env, napi_value* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_create_array_with_length(napi_env, size_t, napi_value* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_set_element(napi_env, napi_value, uint32_t, napi_value) { return napi_ok; }
napi_status napi_get_array_length(napi_env, napi_value, uint32_t* r) { if (r) *r = 0; return napi_ok; }
napi_status napi_get_arraybuffer_info(napi_env, napi_value, void**, size_t*) { return napi_ok; }
napi_status napi_get_property_names(napi_env, napi_value, napi_value* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_get_last_error_info(napi_env, const napi_extended_error_info**) { return napi_ok; }
napi_status napi_throw_error(napi_env, const char*, const char*) { return napi_ok; }
napi_status napi_typeof(napi_env, napi_value, napi_valuetype* r) { if (r) *r = static_cast<napi_valuetype>(0); return napi_ok; }
napi_status napi_is_array(napi_env, napi_value, bool* r) { if (r) *r = false; return napi_ok; }
napi_status napi_is_promise(napi_env, napi_value, bool* r) { if (r) *r = false; return napi_ok; }
napi_status napi_is_arraybuffer(napi_env, napi_value, bool* r) { if (r) *r = false; return napi_ok; }
napi_status napi_strict_equals(napi_env, napi_value, napi_value, bool* r) { if (r) *r = false; return napi_ok; }
napi_status napi_define_properties(napi_env, napi_value, size_t, const napi_property_descriptor*) { return napi_ok; }
napi_status napi_call_function(napi_env, napi_value, napi_value, size_t, const napi_value*, napi_value* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_get_value_string_utf8(napi_env, napi_value, char* buf, size_t bufsize, size_t* result) {
  if (result) *result = 0;
  if (buf && bufsize > 0) buf[0] = '\0';
  return napi_ok;
}
napi_status napi_get_reference_value(napi_env, napi_ref, napi_value* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_create_reference(napi_env, napi_value, uint32_t, napi_ref* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_delete_reference(napi_env, napi_ref) { return napi_ok; }
napi_status napi_is_exception_pending(napi_env, bool* r) { if (r) *r = false; return napi_ok; }
napi_status napi_get_and_clear_last_exception(napi_env, napi_value* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_add_finalizer(napi_env, napi_value, void*, napi_finalize, void*, napi_ref* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_get_cb_info(napi_env, napi_callback_info, size_t* r, napi_value*, napi_value*, void**) { if (r) *r = 0; return napi_ok; }
napi_status napi_create_function(napi_env, const char*, size_t, napi_callback, void*, napi_value* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_has_property(napi_env, napi_value, napi_value, bool* r) { if (r) *r = false; return napi_ok; }
napi_status napi_get_property(napi_env, napi_value, napi_value, napi_value* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_get_value_bool(napi_env, napi_value, bool* r) { if (r) *r = false; return napi_ok; }
napi_status napi_get_value_int32(napi_env, napi_value, int32_t* r) { if (r) *r = 0; return napi_ok; }
napi_status napi_get_value_double(napi_env, napi_value, double* r) { if (r) *r = 0; return napi_ok; }
napi_status napi_get_element(napi_env, napi_value, uint32_t, napi_value* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_set_property(napi_env, napi_value, napi_value, napi_value) { return napi_ok; }
napi_status napi_has_named_property(napi_env, napi_value, const char*, bool* r) { if (r) *r = false; return napi_ok; }
napi_status napi_get_named_property(napi_env, napi_value, const char*, napi_value* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_create_promise(napi_env, napi_deferred* d, napi_value* r) {
  if (d) *d = nullptr;
  if (r) *r = nullptr;
  return napi_ok;
}
napi_status napi_resolve_deferred(napi_env, napi_deferred, napi_value) { return napi_ok; }
napi_status napi_reject_deferred(napi_env, napi_deferred, napi_value) { return napi_ok; }
napi_status napi_is_error(napi_env, napi_value, bool* r) { if (r) *r = false; return napi_ok; }
napi_status napi_create_error(napi_env, napi_value, napi_value, napi_value* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_create_int64(napi_env, int64_t, napi_value* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_create_string_latin1(napi_env, const char*, size_t, napi_value* r) { if (r) *r = nullptr; return napi_ok; }
napi_status napi_get_value_int64(napi_env, napi_value, int64_t* r) { if (r) *r = 0; return napi_ok; }
napi_status napi_get_value_string_latin1(napi_env, napi_value, char*, size_t, size_t* r) { if (r) *r = 0; return napi_ok; }

} // extern "C"
