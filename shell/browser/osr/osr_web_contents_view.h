// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_OSR_OSR_WEB_CONTENTS_VIEW_H_
#define ELECTRON_SHELL_BROWSER_OSR_OSR_WEB_CONTENTS_VIEW_H_

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "shell/browser/native_window_observer.h"

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ptr_exclusion.h"
#include "base/memory/weak_ptr.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "content/browser/renderer_host/render_view_host_delegate_view.h"  // nogncheck
#include "content/browser/web_contents/web_contents_view.h"  // nogncheck
#include "shell/browser/osr/osr_render_widget_host_view.h"
#include "third_party/blink/public/common/page/drag_operation.h"
#include "third_party/blink/public/mojom/drag/drag.mojom-forward.h"
#include "ui/base/ime/text_input_mode.h"
#include "ui/base/ime/text_input_type.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/range/range.h"

#if BUILDFLAG(IS_MAC)
#ifdef __OBJC__
@class OffScreenView;
#else
class OffScreenView;
#endif
#endif

namespace content {
class RenderWidgetHostImpl;
class WebContents;
}  // namespace content

namespace ui {
struct ImeTextSpan;
}

namespace electron {

class NativeWindow;

// Callbacks through which offscreen contents report the text input state of
// the focused element to the embedder. Coordinates are DIPs in view space.
struct OffscreenTextInputCallbacks {
  using StateChanged = base::RepeatingCallback<
      void(ui::TextInputType, ui::TextInputMode, bool can_compose_inline)>;
  // |range| is invalid when the composition has ended.
  using CompositionRangeChanged = base::RepeatingCallback<
      void(const gfx::Range&, const std::vector<gfx::Rect>& character_bounds)>;
  using SelectionBoundsChanged =
      base::RepeatingCallback<void(const gfx::Rect& anchor,
                                   const gfx::Rect& focus)>;

  OffscreenTextInputCallbacks();
  OffscreenTextInputCallbacks(const OffscreenTextInputCallbacks&);
  OffscreenTextInputCallbacks& operator=(const OffscreenTextInputCallbacks&);
  ~OffscreenTextInputCallbacks();

  StateChanged state_changed;
  CompositionRangeChanged composition_range_changed;
  SelectionBoundsChanged selection_bounds_changed;
};

class OffScreenWebContentsView : public content::WebContentsView,
                                 public content::RenderViewHostDelegateView,
                                 private NativeWindowObserver {
 public:
  OffScreenWebContentsView(
      bool transparent,
      bool offscreen_use_shared_texture,
      const std::string& offscreen_shared_texture_pixel_format,
      float offscreen_device_scale_factor);
  ~OffScreenWebContentsView() override;

  // Identifies the focused editable element and its input type.
  struct TextInputState {
    viz::FrameSinkId widget;
    int node_id = 0;
    ui::TextInputType type = ui::TEXT_INPUT_TYPE_NONE;
    ui::TextInputMode mode = ui::TEXT_INPUT_MODE_DEFAULT;
    bool can_compose_inline = false;

    bool operator==(const TextInputState&) const = default;
  };

  void SetWebContents(content::WebContents*);
  void SetNativeWindow(NativeWindow* window);
  void SetCallback(const OnPaintCallback& callback);
  void SetTextInputCallbacks(const OffscreenTextInputCallbacks& callbacks);

  // IME input from the embedder, routed to the widget with text input focus.
  void ImeSetComposition(const std::u16string& text,
                         const std::vector<ui::ImeTextSpan>& spans,
                         const gfx::Range& replacement_range,
                         int selection_start,
                         int selection_end);
  void ImeCommitText(const std::u16string& text,
                     const gfx::Range& replacement_range,
                     int relative_cursor_position);
  void ImeFinishComposingText(bool keep_selection);
  void ImeCancelComposition();

  // Reports from the root OffScreenRenderWidgetHostView.
  void OnTextInputStateChanged(const TextInputState& state);
  void OnImeCompositionCancelled();
  void OnImeCompositionRangeChanged(
      const gfx::Range& range,
      const std::vector<gfx::Rect>& character_bounds);
  void OnSelectionBoundsChanged(const gfx::Rect& anchor,
                                const gfx::Rect& focus);
  // Reports that no element is focused, e.g. after the renderer died.
  void ResetTextInputState();

  // NativeWindowObserver:
  void OnWindowResize() override;
  void OnWindowClosed() override;

  gfx::Size GetSize() const override;

  // content::WebContentsView:
  gfx::NativeView GetNativeView() const override;
  gfx::NativeView GetContentNativeView() const override;
  gfx::NativeWindow GetTopLevelNativeWindow() const override;
  gfx::Rect GetContainerBounds() const override;
  void Focus() override;
  void Resize(const gfx::Rect& new_bounds) override {}
  void SetInitialFocus() override {}
  void StoreFocus() override {}
  void RestoreFocus() override {}
  void FocusThroughTabTraversal(bool reverse) override {}
  content::DropData* GetDropData() const override;
  gfx::Rect GetViewBounds() const override;
  void CreateView(gfx::NativeView context) override {}
  content::RenderWidgetHostViewBase* CreateViewForWidget(
      content::RenderWidgetHost* render_widget_host) override;
  content::RenderWidgetHostViewBase* CreateViewForChildWidget(
      content::RenderWidgetHost* render_widget_host) override;
  void SetPageTitle(const std::u16string& title) override {}
  void RenderViewReady() override;
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override {}
  void SetOverscrollControllerEnabled(bool enabled) override {}
  void OnCapturerCountChanged() override {}
  void FullscreenStateChanged(bool is_fullscreen) override {}
  void UpdateWindowControlsOverlay(const gfx::Rect& bounding_rect) override {}
  content::BackForwardTransitionAnimationManager*
  GetBackForwardTransitionAnimationManager() override;
  void DestroyBackForwardTransitionAnimationManager() override {}

#if BUILDFLAG(IS_MAC)
  bool CloseTabAfterEventTrackingIfNeeded() override;
#endif

  // content::RenderViewHostDelegateView
  void StartDragging(
      content::RenderFrameHost& source_rfh,
      const content::DropData& drop_data,
      blink::DragOperationsMask allowed_ops,
      const gfx::ImageSkia& image,
      const gfx::Vector2d& cursor_offset,
      const gfx::Rect& drag_obj_rect,
      const blink::mojom::DragEventSourceInfo& event_info) override;
  void UpdateDragOperation(ui::mojom::DragOperation operation,
                           bool document_is_handling_drag) override {}
  void SetPainting(bool painting);
  bool IsPainting() const;
  void SetFrameRate(int frame_rate);
  int GetFrameRate() const;

 private:
#if BUILDFLAG(IS_MAC)
  void PlatformCreate();
  void PlatformDestroy();
#endif

  OffScreenRenderWidgetHostView* GetView() const;
  // The widget IME input should go to; null if there is none.
  content::RenderWidgetHostImpl* GetImeTargetWidget() const;
  // Reports the end of the current composition, if any.
  void EndImeComposition();

  raw_ptr<NativeWindow> native_window_ = nullptr;

  const bool transparent_;
  const bool offscreen_use_shared_texture_;
  const std::string offscreen_shared_texture_pixel_format_;
  const float offscreen_device_scale_factor_;
  bool painting_ = true;
  int frame_rate_ = 60;
  OnPaintCallback callback_;
  OffscreenTextInputCallbacks text_input_callbacks_;

  // Whether the page has an IME composition as far as the embedder knows.
  bool has_ime_composition_ = false;
  TextInputState last_text_input_state_;
  std::optional<std::pair<gfx::Rect, gfx::Rect>> last_selection_bounds_;

  // Weak refs.
  raw_ptr<content::WebContents> web_contents_ = nullptr;

#if BUILDFLAG(IS_MAC)
  RAW_PTR_EXCLUSION OffScreenView* offScreenView_ = nullptr;
#endif

  base::WeakPtrFactory<OffScreenWebContentsView> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_OSR_OSR_WEB_CONTENTS_VIEW_H_
