#include <node_api.h>

#import <Cocoa/Cocoa.h>

#include <vector>

namespace {

napi_value GetMenuItemTitles(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  size_t length;
  bool refresh;
  if (napi_get_cb_info(env, info, &argc, args, nullptr, nullptr) != napi_ok ||
      argc != 2 ||
      napi_get_value_string_utf8(env, args[0], nullptr, 0, &length) !=
          napi_ok ||
      napi_get_value_bool(env, args[1], &refresh) != napi_ok) {
    napi_throw_type_error(env, nullptr,
                          "Expected a menu title and refresh flag");
    return nullptr;
  }
  std::vector<char> title(length + 1);
  napi_get_value_string_utf8(env, args[0], title.data(), title.size(), &length);
  NSMenu* menu = [[NSApp mainMenu]
                     itemWithTitle:[NSString stringWithUTF8String:title.data()]]
                     .submenu;
  napi_value result;
  if (!menu) {
    napi_get_null(env, &result);
    return result;
  }

  // Simulate AppKit's refresh notification before displaying the menu.
  // -update alone does not send this notification.
  if (refresh) {
    id<NSMenuDelegate> delegate = menu.delegate;
    if (![delegate respondsToSelector:@selector(menuNeedsUpdate:)]) {
      napi_throw_error(env, nullptr, "Menu has no refresh delegate");
      return nullptr;
    }
    [delegate menuNeedsUpdate:menu];
  }

  napi_create_array_with_length(env, menu.numberOfItems, &result);
  for (NSInteger i = 0; i < menu.numberOfItems; ++i) {
    napi_value item_title;
    napi_create_string_utf8(env, [menu itemAtIndex:i].title.UTF8String,
                            NAPI_AUTO_LENGTH, &item_title);
    napi_set_element(env, result, i, item_title);
  }
  return result;
}

napi_value Initialize(napi_env env, napi_value exports) {
  napi_value function;

  napi_status status =
      napi_create_function(env, "getMenuItemTitles", NAPI_AUTO_LENGTH,
                           GetMenuItemTitles, nullptr, &function);
  if (status != napi_ok) {
    return nullptr;
  }
  status = napi_set_named_property(env, exports, "getMenuItemTitles", function);
  if (status != napi_ok) {
    return nullptr;
  }
  return exports;
}

}  // namespace

NAPI_MODULE(NODE_GYP_MODULE_NAME, Initialize)
