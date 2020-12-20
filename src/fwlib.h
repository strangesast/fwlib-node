#ifndef NODE_FWLIB_H
#define NODE_FWLIB_H

#include <node/node_api.h>

class Fwlib {
 public:
  static napi_value Init(napi_env env, napi_value exports);
  static void Destructor(napi_env env, void* nativeObject, void* finalize_hint);

 private:
  explicit Fwlib(char* host_, unsigned int port = 8193);
  ~Fwlib();

  static napi_value New(napi_env env, napi_callback_info info);
  static napi_value Connect(napi_env env, napi_callback_info info);
  static napi_value Disconnect(napi_env env, napi_callback_info info);
  static napi_value GetConnected(napi_env env, napi_callback_info info);
  static napi_value Rdcncid(napi_env env, napi_callback_info info);
  static napi_value Sysinfo(napi_env env, napi_callback_info info);
  static napi_value Rdaxisname(napi_env env, napi_callback_info info);
  static napi_value Rdaxisdata(napi_env env, napi_callback_info info);
  static napi_value Exeprgname(napi_env env, napi_callback_info info);
  static napi_value Exeprgname2(napi_env env, napi_callback_info info);
  static napi_value Statinfo(napi_env env, napi_callback_info info);
  static inline napi_value Constructor(napi_env env);

  bool connected = false;
  char* host_;
  unsigned int port_;
  napi_env env_;
  napi_ref wrapper_;
  unsigned short libh;
};

#endif  // NODE_FWLIB_H
