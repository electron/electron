// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_NATIVE_THEME_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_NATIVE_THEME_H_

#include "base/memory/raw_ptr.h"
#include "gin/per_isolate_data.h"
#include "gin/weak_cell.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "ui/native_theme/native_theme.h"
#include "ui/native_theme/native_theme_observer.h"

#if BUILDFLAG(IS_WIN)
#include "base/win/registry.h"
#endif

namespace electron::api {

class NativeTheme final : public gin::Wrappable<NativeTheme>,
                          public gin_helper::EventEmitterMixin<NativeTheme>,
                          public gin::PerIsolateData::DisposeObserver,
                          private ui::NativeThemeObserver {
 public:
  static NativeTheme* Create(v8::Isolate* isolate);

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  static const char* GetClassName() { return "NativeTheme"; }
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  void Trace(cppgc::Visitor* visitor) const override;

  // gin::PerIsolateData::DisposeObserver
  void OnBeforeDispose(v8::Isolate* isolate) override {}
  void OnBeforeMicrotasksRunnerDispose(v8::Isolate* isolate) override;
  void OnDisposed() override {}

  // disable copy
  NativeTheme(const NativeTheme&) = delete;
  NativeTheme& operator=(const NativeTheme&) = delete;

  // Make public for cppgc::MakeGarbageCollected.
  NativeTheme(v8::Isolate* isolate,
              ui::NativeTheme* ui_theme,
              ui::NativeTheme* web_theme);
  ~NativeTheme() override;

 private:
  void SetThemeSource(ui::NativeTheme::ThemeSource override);
#if BUILDFLAG(IS_MAC)
  void UpdateMacOSAppearanceForOverrideValue(
      ui::NativeTheme::ThemeSource override);
#endif
  ui::NativeTheme::ThemeSource GetThemeSource() const;
  bool ShouldUseDarkColors();
  bool ShouldUseHighContrastColors();
  bool ShouldUseDarkColorsForSystemIntegratedUI();
  bool ShouldUseInvertedColorScheme();
  bool InForcedColorsMode();
  bool GetPrefersReducedTransparency();
#if BUILDFLAG(IS_MAC)
  bool ShouldDifferentiateWithoutColor();
#endif

  // ui::NativeThemeObserver:
  void OnNativeThemeUpdated(ui::NativeTheme* theme) override;
  void OnNativeThemeUpdatedOnUI();

#if BUILDFLAG(IS_WIN)
  base::win::RegKey hkcu_themes_regkey_;
#endif
  std::optional<bool> should_use_dark_colors_for_system_integrated_ui_ =
      std::nullopt;
  raw_ptr<ui::NativeTheme> ui_theme_;
  raw_ptr<ui::NativeTheme> web_theme_;
  gin::WeakCellFactory<NativeTheme> weak_factory_{this};
};

}  // namespace electron::api

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

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_NATIVE_THEME_H_
