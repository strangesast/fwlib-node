#include "fwlib.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../extern/fwlib/fwlib32.h"

Fwlib::Fwlib(char* host, unsigned int port)
    : host_(host), port_(port), env_(nullptr), wrapper_(nullptr) {
  host_ = new char[strlen(host) + 1];
  strcpy(host_, host);
}

Fwlib::~Fwlib() { napi_delete_reference(env_, wrapper_); }

void Fwlib::Destructor(napi_env env, void* nativeObject,
                       void* /*finalize_hint*/) {
  reinterpret_cast<Fwlib*>(nativeObject)->~Fwlib();
  cnc_exitprocess();
}

#define DECLARE_NAPI_METHOD(name, func) \
  { name, 0, func, 0, 0, 0, napi_default, 0 }

napi_value Fwlib::Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_property_descriptor properties[] = {
      {"connected", 0, 0, GetConnected, 0, 0, napi_default, 0},
      DECLARE_NAPI_METHOD("connect", Connect),
      DECLARE_NAPI_METHOD("disconnect", Disconnect),
      DECLARE_NAPI_METHOD("rdcncid", Rdcncid),
      DECLARE_NAPI_METHOD("sysinfo", Sysinfo),
      DECLARE_NAPI_METHOD("rdaxisdata", Rdaxisdata),
  };

  napi_value cons;
  status = napi_define_class(env, "Fwlib", NAPI_AUTO_LENGTH, New, nullptr, 6,
                             properties, &cons);
  assert(status == napi_ok);
  napi_ref* constructor = new napi_ref;
  status = napi_create_reference(env, cons, 1, constructor);
  assert(status == napi_ok);
  status = napi_set_instance_data(
      env, constructor,
      [](napi_env env, void* data, void* hint) {
        napi_ref* constructor = static_cast<napi_ref*>(data);
        napi_status status = napi_delete_reference(env, *constructor);
        assert(status == napi_ok);
        delete constructor;
      },
      nullptr);
  assert(status == napi_ok);

  status = napi_set_named_property(env, exports, "Fwlib", cons);
  assert(status == napi_ok);

  short ret;
  ret = cnc_startupprocess(0, "focas.log");
  assert(ret == EW_OK);

  return exports;
}

napi_value Fwlib::Constructor(napi_env env) {
  void* instance_data = nullptr;
  napi_status status = napi_get_instance_data(env, &instance_data);
  assert(status == napi_ok);
  napi_ref* constructor = static_cast<napi_ref*>(instance_data);

  napi_value cons;
  status = napi_get_reference_value(env, *constructor, &cons);
  assert(status == napi_ok);
  return cons;
}

napi_value Fwlib::New(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value target;
  status = napi_get_new_target(env, info, &target);
  assert(status == napi_ok);
  bool is_constructor = target != nullptr;

  if (is_constructor) {
    // Invoked as constructor: `new Fwlib(...)`
    size_t argc = 2;
    napi_value args[2];
    napi_value jsthis;
    status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
    assert(status == napi_ok);

    char host[100] = "";
    unsigned int port = 8193;

    // read arg0
    napi_valuetype valuetype;
    status = napi_typeof(env, args[0], &valuetype);
    assert(status == napi_ok);

    if (valuetype != napi_undefined) {
      status =
          napi_get_value_string_utf8(env, args[0], host, sizeof(host), nullptr);
      assert(status == napi_ok);
    }

    // read arg1
    status = napi_typeof(env, args[1], &valuetype);
    assert(status == napi_ok);

    if (valuetype != napi_undefined) {
      uint32_t result;
      status = napi_get_value_uint32(env, args[1], &result);
      assert(status == napi_ok);
      port = result;
    }

    Fwlib* obj = new Fwlib(host, port);

    obj->env_ = env;
    status =
        napi_wrap(env, jsthis, reinterpret_cast<void*>(obj), Fwlib::Destructor,
                  nullptr,  // finalize_hint
                  &obj->wrapper_);
    assert(status == napi_ok);

    return jsthis;
  } else {
    // Invoked as plain function `Fwlib(...)`, turn into construct call.
    size_t argc_ = 2;
    napi_value args[2];
    status = napi_get_cb_info(env, info, &argc_, args, nullptr, nullptr);
    assert(status == napi_ok);

    const size_t argc = 2;
    napi_value argv[argc] = {args[0], args[1]};

    napi_value instance;
    status = napi_new_instance(env, Constructor(env), argc, argv, &instance);
    assert(status == napi_ok);

    return instance;
  }
}

napi_value Fwlib::GetConnected(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value jsthis;
  status = napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  napi_value num;
  status = napi_create_int32(env, obj->connected ? 1 : 0, &num);
  assert(status == napi_ok);

  return num;
}

napi_value Fwlib::Connect(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value jsthis;
  status = napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  short ret;
  ret = cnc_allclibhndl3(obj->host_, obj->port_, 10, &obj->libh);
  if (ret != EW_OK) {
    char code[8] = "";
    const char* msg;
    switch (ret) {
      case EW_SOCKET:
        msg = "Socket communication error";
        break;
      case EW_NODLL:
        msg = "There is no DLL file for each CNC series";
        break;
      case EW_HANDLE:
        msg = "Allocation of handle number is failed.";
        break;
      default:
        msg = "An unknown error occurred.";
    }
    snprintf(code, 7, "%d", ret);
    status = napi_throw_error(env, code, msg);
    assert(status == napi_ok);
    return nullptr;
  }

  obj->connected = true;

  napi_value num;
  status = napi_create_int32(env, ret, &num);
  assert(status == napi_ok);

  return num;
}

napi_value Fwlib::Disconnect(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value jsthis;
  status = napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  short ret;
  ret = cnc_freelibhndl(obj->libh);
  obj->connected = false;

  napi_value num;
  status = napi_create_int32(env, ret, &num);
  assert(status == napi_ok);

  return num;
}

napi_value Fwlib::Sysinfo(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value jsthis;
  status = napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  short ret;
  ODBSYS sysinfo;
  ret = cnc_sysinfo(obj->libh, &sysinfo);

  if (ret != EW_OK) {
    char code[8] = "";
    const char* msg = "An unknown error occurred.";
    snprintf(code, 7, "%d", ret);
    status = napi_throw_error(env, code, msg);
    assert(status == napi_ok);
    return nullptr;
  }

  napi_value result;
  status = napi_create_object(env, &result);
  assert(status == napi_ok);

  napi_value val;
  status = napi_create_uint32(env, sysinfo.addinfo, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "addinfo", val);
  assert(status == napi_ok);

  status = napi_create_string_utf8(env, sysinfo.axes, 2, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "axes", val);
  assert(status == napi_ok);

  status = napi_create_string_utf8(env, sysinfo.cnc_type, 2, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "cnc_type", val);
  assert(status == napi_ok);

  status = napi_create_uint32(env, sysinfo.max_axis, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "max_axis", val);
  assert(status == napi_ok);

  status = napi_create_string_utf8(env, sysinfo.mt_type, 2, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "mt_type", val);
  assert(status == napi_ok);

  status = napi_create_string_utf8(env, sysinfo.series, 4, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "series", val);
  assert(status == napi_ok);

  status = napi_create_string_utf8(env, sysinfo.version, 4, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "version", val);
  assert(status == napi_ok);

  return result;
}

napi_value Fwlib::Rdcncid(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value jsthis;
  status = napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  short ret;
  uint32_t ids[4] = {0};
  ret = cnc_rdcncid(obj->libh, (unsigned long*)ids);

  if (ret != EW_OK) {
    char code[8] = "";
    const char* msg;
    switch (ret) {
      case EW_FUNC:
        msg = "This function is not available.";
        break;
      default:
        msg = "An unknown error occurred.";
    }
    snprintf(code, 7, "%d", ret);
    status = napi_throw_error(env, code, msg);
    assert(status == napi_ok);
    return nullptr;
  }

  napi_value result;
  status = napi_create_array_with_length(env, 4, &result);
  assert(status == napi_ok);
  for (int i = 0; i < 4; i++) {
    napi_value val;
    status = napi_create_uint32(env, ids[i], &val);
    assert(status == napi_ok);
    status = napi_set_element(env, result, i, val);
    assert(status == napi_ok);
  }

  return result;
}

typedef struct odbaxdt_t {
  char name[4];
  int32_t data;
  int16_t dec;
  int16_t unit;
  int16_t flag;
  int16_t reserve;
} ODBAXDT_T;

napi_value Fwlib::Rdaxisdata(napi_env env, napi_callback_info info) {
  napi_status status;

  size_t argc = 2;
  napi_value args[2];
  napi_value jsthis;
  status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  short types_length = 0;
  unsigned int type = 0;
  short types[MAX_AXIS] = {0};

  // read arg0
  napi_valuetype valuetype;
  status = napi_typeof(env, args[0], &valuetype);
  assert(status == napi_ok);

  if (valuetype != napi_undefined) {
    uint32_t arg0;
    status = napi_get_value_uint32(env, args[0], &arg0);
    assert(status == napi_ok);
    type = arg0;
  }

  // read arg1
  status = napi_typeof(env, args[1], &valuetype);
  assert(status == napi_ok);

  if (valuetype != napi_undefined) {
    bool is_array;
    status = napi_is_array(env, args[1], &is_array);
    assert(status == napi_ok);
    if (is_array) {
      uint32_t tmp0;
      status = napi_get_array_length(env, args[1], &tmp0);
      assert(status == napi_ok);
      types_length = tmp0;
      for (int i = 0; i < types_length; i++) {
        napi_value array_elem;
        status = napi_get_element(env, args[1], i, &array_elem);
        assert(status == napi_ok);

        napi_value array_num;
        status = napi_coerce_to_number(env, array_elem, &array_num);
        assert(status == napi_ok);

        int32_t array_val;
        status = napi_get_value_int32(env, array_num, &array_val);
        assert(status == napi_ok);
        types[i] = (short)array_val;
      }
    }
  }
  short ret;
  short axis_num = MAX_AXIS;
  ODBAXDT_T *axis_data = new ODBAXDT_T[MAX_AXIS * types_length];
  ODBAXDT* tmp1 = (ODBAXDT*)axis_data;
  ret = cnc_rdaxisdata(obj->libh, type, types, types_length, &axis_num, tmp1);

  if (ret != EW_OK) {
    char code[8] = "";
    const char* msg;
    switch (ret) {
      case EW_LENGTH:
        msg = "Data block error: Number of axis(*len) is less or equal 0.";
        break;
      case EW_NUMBER:
        msg = "Data number error: Data class(cls) is wrong.";
        break;
      case EW_ATTRIB:
        msg =
            "Data attribute error: Kind of data(type) is wrong, or The number "
            "of kind(num) exceeds 4.";
        break;
      case EW_NOOPT:
        msg = "No option: Required option to read data is not specified.";
        break;
      default:
        msg = "An unknown error occurred.";
    }
    snprintf(code, 7, "%d", ret);
    status = napi_throw_error(env, code, msg);
    assert(status == napi_ok);
    return nullptr;
  }

  napi_value result;
  size_t result_arr_length = types_length * axis_num;
  status = napi_create_array_with_length(env, result_arr_length, &result);
  assert(status == napi_ok);

  for (int i = 0; i < axis_num; i++) {
    for (int j = 0; j < types_length; j++) {
      int ji = j * MAX_AXIS + i;
      napi_value elem;
      status = napi_create_object(env, &elem);
      assert(status == napi_ok);

      napi_value val;
      status = napi_create_int32(env, axis_data[ji].data, &val);
      assert(status == napi_ok);
      status = napi_set_named_property(env, elem, "data", val);
      assert(status == napi_ok);

      status = napi_create_int32(env, axis_data[ji].dec, &val);
      assert(status == napi_ok);
      status = napi_set_named_property(env, elem, "dec", val);
      assert(status == napi_ok);

      status = napi_create_int32(env, axis_data[ji].flag, &val);
      assert(status == napi_ok);
      status = napi_set_named_property(env, elem, "flag", val);
      assert(status == napi_ok);

      status = napi_create_string_utf8(env, axis_data[ji].name, strlen(axis_data[ji].name), &val);
      assert(status == napi_ok);
      status = napi_set_named_property(env, elem, "name", val);
      assert(status == napi_ok);

      status = napi_create_int32(env, axis_data[ji].reserve, &val);
      assert(status == napi_ok);
      status = napi_set_named_property(env, elem, "reserve", val);
      assert(status == napi_ok);

      status = napi_create_int32(env, axis_data[ji].unit, &val);
      assert(status == napi_ok);
      status = napi_set_named_property(env, elem, "unit", val);
      assert(status == napi_ok);

      status = napi_set_element(env, result, j * axis_num + i, elem);
      assert(status == napi_ok);
    }
  }
  delete[] axis_data;

  return result;
}

