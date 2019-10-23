// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_NATIVE_THEME_H_
#define SHELL_BROWSER_API_ATOM_API_NATIVE_THEME_H_

#include "shell/common/gin_helper/event_emitter.h"
#include "ui/native_theme/native_theme.h"
#include "ui/native_theme/native_theme_observer.h"

namespace electron {

namespace api {

class NativeTheme : public gin_helper::EventEmitter<NativeTheme>,
                    public ui::NativeThemeObserver {
 public:
  static v8::Local<v8::Value> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  NativeTheme(v8::Isolate* isolate, ui::NativeTheme* theme);
  ~NativeTheme() override;

  void SetThemeSource(ui::NativeTheme::ThemeSource override);
#if defined(OS_MACOSX)
  void UpdateMacOSAppearanceForOverrideValue(
      ui::NativeTheme::ThemeSource override);
#endif
  ui::NativeTheme::ThemeSource GetThemeSource() const;
  bool ShouldUseDarkColors();
  bool ShouldUseHighContrastColors();
  bool ShouldUseInvertedColorScheme();

  // ui::NativeThemeObserver:
  void OnNativeThemeUpdated(ui::NativeTheme* theme) override;
  void OnNativeThemeUpdatedOnUI();

 private:
  ui::NativeTheme* theme_;

  DISALLOW_COPY_AND_ASSIGN(NativeTheme);
};

}  // namespace api

}  // namespace electron

namespace gin {

template <>
struct Converter<ui::NativeTheme::ThemeSource> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const ui::NativeTheme::ThemeSource& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     ui::NativeTheme::ThemeSource* out);
};

}  // namespace gin

#endif  // SHELL_BROWSER_API_ATOM_API_NATIVE_THEME_H_
