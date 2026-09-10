// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/osr/osr_web_contents_view.h"

#include "base/check.h"
#include "base/functional/callback_helpers.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"  // nogncheck
#include "content/browser/renderer_host/text_input_manager.h"       // nogncheck
#include "content/browser/web_contents/web_contents_impl.h"         // nogncheck
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "shell/browser/native_window.h"
#include "ui/base/ime/ime_text_span.h"
#include "ui/display/screen.h"
#include "ui/display/screen_info.h"

namespace electron {

OffscreenTextInputCallbacks::OffscreenTextInputCallbacks() = default;
OffscreenTextInputCallbacks::OffscreenTextInputCallbacks(
    const OffscreenTextInputCallbacks&) = default;
OffscreenTextInputCallbacks& OffscreenTextInputCallbacks::operator=(
    const OffscreenTextInputCallbacks&) = default;
OffscreenTextInputCallbacks::~OffscreenTextInputCallbacks() = default;

OffScreenWebContentsView::OffScreenWebContentsView(
    bool transparent,
    bool offscreen_use_shared_texture,
    const std::string& offscreen_shared_texture_pixel_format,
    float offscreen_device_scale_factor)
    : transparent_(transparent),
      offscreen_use_shared_texture_(offscreen_use_shared_texture),
      offscreen_shared_texture_pixel_format_(
          offscreen_shared_texture_pixel_format),
      offscreen_device_scale_factor_(offscreen_device_scale_factor),
      callback_(base::DoNothing()) {
#if BUILDFLAG(IS_MAC)
  PlatformCreate();
#endif
}

OffScreenWebContentsView::~OffScreenWebContentsView() {
  if (native_window_)
    native_window_->RemoveObserver(this);

#if BUILDFLAG(IS_MAC)
  PlatformDestroy();
#endif
}

void OffScreenWebContentsView::SetWebContents(
    content::WebContents* web_contents) {
  web_contents_ = web_contents;

  if (auto* view = GetView())
    view->InstallTransparency();
}

void OffScreenWebContentsView::SetCallback(const OnPaintCallback& callback) {
  callback_ = callback;
  if (auto* view = GetView())
    view->SetPaintCallback(callback);
}

void OffScreenWebContentsView::SetTextInputCallbacks(
    const OffscreenTextInputCallbacks& callbacks) {
  text_input_callbacks_ = callbacks;
}

void OffScreenWebContentsView::Focus() {
  if (auto* view = GetView())
    view->Focus();
}

content::RenderWidgetHostImpl* OffScreenWebContentsView::GetImeTargetWidget()
    const {
  auto* view = GetView();
  if (!view)
    return nullptr;
  auto* manager = view->GetTextInputManager();
  if (manager && manager->GetActiveWidget())
    return manager->GetActiveWidget();
  return view->render_widget_host();
}

void OffScreenWebContentsView::ImeSetComposition(
    const std::u16string& text,
    const std::vector<ui::ImeTextSpan>& spans,
    const gfx::Range& replacement_range,
    int selection_start,
    int selection_end) {
  auto* widget = GetImeTargetWidget();
  // Empty text outside a composition would delete the selection instead.
  if (!widget || (text.empty() && !has_ime_composition_))
    return;
  widget->ImeSetComposition(text, spans, replacement_range, selection_start,
                            selection_end);
  if (text.empty())
    EndImeComposition();
  else
    has_ime_composition_ = true;
}

void OffScreenWebContentsView::ImeCommitText(
    const std::u16string& text,
    const gfx::Range& replacement_range,
    int relative_cursor_position) {
  auto* widget = GetImeTargetWidget();
  if (!widget)
    return;
  widget->ImeCommitText(text, {}, replacement_range, relative_cursor_position);
  // With a replacement range the renderer only replaces that text.
  if (!replacement_range.IsValid())
    EndImeComposition();
}

void OffScreenWebContentsView::ImeFinishComposingText(bool keep_selection) {
  auto* widget = GetImeTargetWidget();
  if (!widget || !has_ime_composition_)
    return;
  widget->ImeFinishComposingText(keep_selection);
  EndImeComposition();
}

void OffScreenWebContentsView::ImeCancelComposition() {
  auto* widget = GetImeTargetWidget();
  // Without a composition this would delete the selection instead.
  if (!widget || !has_ime_composition_)
    return;
  widget->ImeCancelComposition();
  EndImeComposition();
}

void OffScreenWebContentsView::EndImeComposition() {
  if (!has_ime_composition_)
    return;
  has_ime_composition_ = false;
  if (text_input_callbacks_.composition_range_changed) {
    text_input_callbacks_.composition_range_changed.Run(
        gfx::Range::InvalidRange(), {});
  }
}

void OffScreenWebContentsView::OnTextInputStateChanged(
    const TextInputState& state) {
  if (state == last_text_input_state_)
    return;
  const bool focus_changed = state.widget != last_text_input_state_.widget ||
                             state.node_id != last_text_input_state_.node_id;
  last_text_input_state_ = state;
  // The renderer finishes a composition when the focused element changes.
  if (focus_changed) {
    EndImeComposition();
    last_selection_bounds_.reset();
  }
  if (text_input_callbacks_.state_changed) {
    text_input_callbacks_.state_changed.Run(state.type, state.mode,
                                            state.can_compose_inline);
  }
}

void OffScreenWebContentsView::OnImeCompositionCancelled() {
  EndImeComposition();
}

void OffScreenWebContentsView::OnImeCompositionRangeChanged(
    const gfx::Range& range,
    const std::vector<gfx::Rect>& character_bounds) {
  if (!range.IsValid()) {
    EndImeComposition();
    return;
  }
  has_ime_composition_ = true;
  if (text_input_callbacks_.composition_range_changed)
    text_input_callbacks_.composition_range_changed.Run(range,
                                                        character_bounds);
}

void OffScreenWebContentsView::OnSelectionBoundsChanged(
    const gfx::Rect& anchor,
    const gfx::Rect& focus) {
  if (last_text_input_state_.type == ui::TEXT_INPUT_TYPE_NONE)
    return;
  auto bounds = std::make_pair(anchor, focus);
  if (last_selection_bounds_ == bounds)
    return;
  last_selection_bounds_ = bounds;
  if (text_input_callbacks_.selection_bounds_changed)
    text_input_callbacks_.selection_bounds_changed.Run(anchor, focus);
}

void OffScreenWebContentsView::ResetTextInputState() {
  EndImeComposition();
  OnTextInputStateChanged(TextInputState());
}

void OffScreenWebContentsView::SetNativeWindow(NativeWindow* window) {
  if (native_window_)
    native_window_->RemoveObserver(this);

  native_window_ = window;

  if (native_window_)
    native_window_->AddObserver(this);

  OnWindowResize();
}

void OffScreenWebContentsView::OnWindowResize() {
  // In offscreen mode call RenderWidgetHostView's SetSize explicitly
  if (auto* view = GetView())
    view->SetSize(GetSize());
}

void OffScreenWebContentsView::OnWindowClosed() {
  if (native_window_) {
    native_window_->RemoveObserver(this);
    native_window_ = nullptr;
  }
}

gfx::Size OffScreenWebContentsView::GetSize() const {
  return native_window_ ? native_window_->GetSize() : gfx::Size();
}

#if !BUILDFLAG(IS_MAC)
gfx::NativeView OffScreenWebContentsView::GetNativeView() const {
  if (!native_window_)
    return {};
  return native_window_->GetNativeView();
}

gfx::NativeView OffScreenWebContentsView::GetContentNativeView() const {
  if (!native_window_)
    return {};
  return native_window_->GetNativeView();
}

gfx::NativeWindow OffScreenWebContentsView::GetTopLevelNativeWindow() const {
  if (!native_window_)
    return {};
  return native_window_->GetNativeWindow();
}
#endif

gfx::Rect OffScreenWebContentsView::GetContainerBounds() const {
  return GetViewBounds();
}

content::DropData* OffScreenWebContentsView::GetDropData() const {
  return nullptr;
}

gfx::Rect OffScreenWebContentsView::GetViewBounds() const {
  if (auto* view = GetView())
    return view->GetViewBounds();
  return {};
}

content::RenderWidgetHostViewBase*
OffScreenWebContentsView::CreateViewForWidget(
    content::RenderWidgetHost* render_widget_host) {
  if (auto* rwhv = render_widget_host->GetView())
    return static_cast<content::RenderWidgetHostViewBase*>(rwhv);

  auto* view = new OffScreenRenderWidgetHostView(
      transparent_, offscreen_use_shared_texture_,
      offscreen_shared_texture_pixel_format_, offscreen_device_scale_factor_,
      painting_, GetFrameRate(), callback_, render_widget_host, nullptr,
      GetSize());
  view->SetWebContentsView(weak_factory_.GetWeakPtr());
  return view;
}

content::RenderWidgetHostViewBase*
OffScreenWebContentsView::CreateViewForChildWidget(
    content::RenderWidgetHost* render_widget_host) {
  auto* web_contents_impl =
      static_cast<content::WebContentsImpl*>(web_contents_);

  OffScreenRenderWidgetHostView* embedder_host_view = nullptr;
  if (web_contents_impl->GetOuterWebContents()) {
    embedder_host_view = static_cast<OffScreenRenderWidgetHostView*>(
        web_contents_impl->GetOuterWebContents()->GetRenderWidgetHostView());
  } else {
    embedder_host_view = static_cast<OffScreenRenderWidgetHostView*>(
        web_contents_impl->GetRenderWidgetHostView());
  }
  CHECK(embedder_host_view);

  return new OffScreenRenderWidgetHostView(
      transparent_, offscreen_use_shared_texture_,
      offscreen_shared_texture_pixel_format_, offscreen_device_scale_factor_,
      painting_, embedder_host_view->frame_rate(), callback_,
      render_widget_host, embedder_host_view, GetSize());
}

void OffScreenWebContentsView::RenderViewReady() {
  if (auto* view = GetView())
    view->InstallTransparency();
}

#if BUILDFLAG(IS_MAC)
bool OffScreenWebContentsView::CloseTabAfterEventTrackingIfNeeded() {
  return false;
}
#endif  // BUILDFLAG(IS_MAC)

void OffScreenWebContentsView::StartDragging(
    content::RenderFrameHost& source_rfh,
    const content::DropData& drop_data,
    blink::DragOperationsMask allowed_ops,
    const gfx::ImageSkia& image,
    const gfx::Vector2d& cursor_offset,
    const gfx::Rect& drag_obj_rect,
    const blink::mojom::DragEventSourceInfo& event_info) {
  if (web_contents_) {
    auto* source_rwh = static_cast<content::RenderWidgetHostImpl*>(
        source_rfh.GetRenderWidgetHost());
    static_cast<content::WebContentsImpl*>(web_contents_)
        ->SystemDragEnded(source_rwh);
  }
}

void OffScreenWebContentsView::SetPainting(bool painting) {
  painting_ = painting;
  if (auto* view = GetView())
    view->SetPainting(painting);
}

bool OffScreenWebContentsView::IsPainting() const {
  if (auto* view = GetView())
    return view->is_painting();
  return painting_;
}

void OffScreenWebContentsView::SetFrameRate(int frame_rate) {
  frame_rate_ = frame_rate;
  if (auto* view = GetView())
    view->SetFrameRate(frame_rate);
}

int OffScreenWebContentsView::GetFrameRate() const {
  if (auto* view = GetView())
    return view->frame_rate();
  return frame_rate_;
}

OffScreenRenderWidgetHostView* OffScreenWebContentsView::GetView() const {
  if (web_contents_) {
    return static_cast<OffScreenRenderWidgetHostView*>(
        web_contents_->GetRenderViewHost()->GetWidget()->GetView());
  }
  return nullptr;
}

content::BackForwardTransitionAnimationManager*
OffScreenWebContentsView::GetBackForwardTransitionAnimationManager() {
  return nullptr;
}

}  // namespace electron
