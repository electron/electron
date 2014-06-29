// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "native_mate/dictionary.h"
#include "ui/gfx/screen.h"

#include "atom/common/node_includes.h"

#if defined(TOOLKIT_GTK)
#include "base/command_line.h"
#include "base/environment.h"
#include "base/nix/xdg_util.h"
#include "ui/gfx/gtk_util.h"
#endif

namespace mate {

namespace {

gfx::Display AdaptToWindowManager(const gfx::Display& display) {
  gfx::Display changed(display);
#if defined(TOOLKIT_GTK)
  scoped_ptr<base::Environment> env(base::Environment::Create());
  base::nix::DesktopEnvironment de(base::nix::GetDesktopEnvironment(env.get()));
  if (de == base::nix::DESKTOP_ENVIRONMENT_UNITY) {
    // Unity's 24px global menu bar should not be included in the work area.
    gfx::Rect rect(changed.work_area());
    rect.set_height(rect.height() - 24);
    changed.set_work_area(rect);
  }
#endif
  return changed;
}

}  // namespace

template<>
struct Converter<gfx::Point> {
  static v8::Handle<v8::Value> ToV8(v8::Isolate* isolate,
                                    const gfx::Point& val) {
    return mate::ObjectTemplateBuilder(isolate).SetValue("x", val.x())
                                               .SetValue("y", val.y())
                                               .Build()->NewInstance();
  }
};

template<>
struct Converter<gfx::Size> {
  static v8::Handle<v8::Value> ToV8(v8::Isolate* isolate,
                                    const gfx::Size& val) {
    return mate::ObjectTemplateBuilder(isolate).SetValue("width", val.width())
                                               .SetValue("height", val.height())
                                               .Build()->NewInstance();
  }
};

template<>
struct Converter<gfx::Rect> {
  static v8::Handle<v8::Value> ToV8(v8::Isolate* isolate,
                                    const gfx::Rect& val) {
    return mate::ObjectTemplateBuilder(isolate).SetValue("x", val.x())
                                               .SetValue("y", val.y())
                                               .SetValue("width", val.width())
                                               .SetValue("height", val.height())
                                               .Build()->NewInstance();
  }
};

template<>
struct Converter<gfx::Display> {
  static v8::Handle<v8::Value> ToV8(v8::Isolate* isolate,
                                    const gfx::Display& val) {
    gfx::Display display(AdaptToWindowManager(val));
    return mate::ObjectTemplateBuilder(isolate)
        .SetValue("bounds", display.bounds())
        .SetValue("workArea", display.work_area())
        .SetValue("size", display.size())
        .SetValue("workAreaSize", display.work_area_size())
        .SetValue("scaleFactor", display.device_scale_factor())
        .Build()->NewInstance();
  }
};

}  // namespace mate

namespace {

void Initialize(v8::Handle<v8::Object> exports, v8::Handle<v8::Value> unused,
                v8::Handle<v8::Context> context, void* priv) {
#if defined(TOOLKIT_GTK)
  gfx::GdkInitFromCommandLine(*CommandLine::ForCurrentProcess());
#endif

  gfx::Screen* screen = gfx::Screen::GetNativeScreen();
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("getCursorScreenPoint",
                 base::Bind(&gfx::Screen::GetCursorScreenPoint,
                            base::Unretained(screen)));
  dict.SetMethod("getPrimaryDisplay",
                 base::Bind(&gfx::Screen::GetPrimaryDisplay,
                            base::Unretained(screen)));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_screen, Initialize)
