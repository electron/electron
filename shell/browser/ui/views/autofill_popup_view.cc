// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/autofill_popup_view.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/i18n/rtl.h"
#include "base/task/single_thread_task_runner.h"
#include "cc/paint/skia_paint_canvas.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "shell/browser/ui/autofill_popup.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/color/color_provider.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/text_utils.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/border.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/widget/widget.h"

namespace electron {

BEGIN_METADATA(AutofillPopupChildView)
END_METADATA

AutofillPopupView::AutofillPopupView(AutofillPopup* popup,
                                     views::Widget* parent_widget)
    : views::WidgetDelegateView(CreatePassKey()),
      popup_(popup),
      parent_widget_(parent_widget) {
  CreateChildViews();
  SetFocusBehavior(FocusBehavior::ALWAYS);
  set_drag_controller(this);

  auto& view_a11y = GetViewAccessibility();
  view_a11y.SetRole(ax::mojom::Role::kMenu);
  view_a11y.SetName(u"Autofill Menu");
}

AutofillPopupView::~AutofillPopupView() {
  if (popup_) {
    auto* host = popup_->frame_host_->GetRenderViewHost()->GetWidget();
    host->RemoveKeyPressEventCallback(keypress_callback_);
    popup_->view_ = nullptr;
    popup_ = nullptr;
  }

  RemoveObserver();

  if (view_proxy_.get()) {
    view_proxy_->ResetView();
  }

  if (GetWidget()) {
    GetWidget()->Close();
  }
}

void AutofillPopupView::Show() {
  bool visible = parent_widget_->IsVisible();
  visible = visible || view_proxy_;
  if (!popup_ || !visible || parent_widget_->IsClosed())
    return;

  const bool initialize_widget = !GetWidget();
  if (initialize_widget) {
    parent_widget_->AddObserver(this);

    // The widget is destroyed by the corresponding NativeWidget, so we use
    // a weak pointer to hold the reference and don't have to worry about
    // deletion.
    auto* widget = new views::Widget;
    views::Widget::InitParams params{
        views::Widget::InitParams::NATIVE_WIDGET_OWNS_WIDGET,
        views::Widget::InitParams::TYPE_POPUP};
    params.delegate = this;
    params.parent = parent_widget_->GetNativeView();
    params.z_order = ui::ZOrderLevel::kFloatingUIElement;
    widget->Init(std::move(params));

    // No animation for popup appearance (too distracting).
    widget->SetVisibilityAnimationTransition(views::Widget::ANIMATE_HIDE);

    show_time_ = base::Time::Now();
  }

  SetBorder(views::CreateSolidBorder(
      kPopupBorderThickness,
      GetColorProvider()->GetColor(ui::kColorUnfocusedBorder)));

  DoUpdateBoundsAndRedrawPopup();
  GetWidget()->Show();

  if (initialize_widget)
    views::NativeViewFocusManager::GetInstance()->AddFocusChangeListener(this);

  keypress_callback_ = base::BindRepeating(
      &AutofillPopupView::HandleKeyPressEvent, base::Unretained(this));
  auto* host = popup_->frame_host_->GetRenderViewHost()->GetWidget();
  host->AddKeyPressEventCallback(keypress_callback_);

  NotifyAccessibilityEventDeprecated(ax::mojom::Event::kMenuStart, true);
}

void AutofillPopupView::Hide() {
  if (popup_) {
    auto* host = popup_->frame_host_->GetRenderViewHost()->GetWidget();
    host->RemoveKeyPressEventCallback(keypress_callback_);
    popup_ = nullptr;
  }

  RemoveObserver();
  NotifyAccessibilityEventDeprecated(ax::mojom::Event::kMenuEnd, true);

  if (GetWidget()) {
    GetWidget()->Close();
  }
}

void AutofillPopupView::OnSuggestionsChanged() {
  if (!popup_)
    return;

  CreateChildViews();
  if (popup_->line_count() == 0) {
    popup_->Hide();
    return;
  }
  DoUpdateBoundsAndRedrawPopup();
}

void AutofillPopupView::WriteDragDataForView(views::View*,
                                             const gfx::Point&,
                                             ui::OSExchangeData*) {}

int AutofillPopupView::GetDragOperationsForView(views::View*,
                                                const gfx::Point&) {
  return ui::DragDropTypes::DRAG_NONE;
}

bool AutofillPopupView::CanStartDragForView(views::View*,
                                            const gfx::Point&,
                                            const gfx::Point&) {
  return false;
}

void AutofillPopupView::OnSelectedRowChanged(
    std::optional<int> previous_row_selection,
    std::optional<int> current_row_selection) {
  SchedulePaint();

  if (current_row_selection) {
    int selected = current_row_selection.value_or(-1);
    if (selected == -1 || static_cast<size_t>(selected) >= children().size())
      return;
    children().at(selected)->NotifyAccessibilityEventDeprecated(
        ax::mojom::Event::kSelection, true);
  }
}

void AutofillPopupView::DrawAutofillEntry(gfx::Canvas* canvas,
                                          int index,
                                          const gfx::Rect& entry_rect) {
  if (!popup_)
    return;

  canvas->FillRect(entry_rect, GetColorProvider()->GetColor(
                                   popup_->GetBackgroundColorIDForRow(index)));

  const bool is_rtl = base::i18n::IsRTL();
  const int text_align =
      is_rtl ? gfx::Canvas::TEXT_ALIGN_RIGHT : gfx::Canvas::TEXT_ALIGN_LEFT;
  gfx::Rect value_rect = entry_rect;
  value_rect.Inset(gfx::Insets::VH(0, kEndPadding));

  int x_align_left = value_rect.x();
  const int value_width = gfx::GetStringWidth(
      popup_->value_at(index), popup_->GetValueFontListForRow(index));
  int value_x_align_left = x_align_left;
  value_x_align_left =
      is_rtl ? value_rect.right() - value_width : value_rect.x();

  canvas->DrawStringRectWithFlags(
      popup_->value_at(index), popup_->GetValueFontListForRow(index),
      GetColorProvider()->GetColor(ui::kColorResultsTableNormalText),
      gfx::Rect(value_x_align_left, value_rect.y(), value_width,
                value_rect.height()),
      text_align);

  // Draw the label text, if one exists.
  if (auto const& label = popup_->label_at(index); !label.empty()) {
    const int label_width =
        gfx::GetStringWidth(label, popup_->GetLabelFontListForRow(index));
    int label_x_align_left = x_align_left;
    label_x_align_left =
        is_rtl ? value_rect.x() : value_rect.right() - label_width;

    canvas->DrawStringRectWithFlags(
        label, popup_->GetLabelFontListForRow(index),
        GetColorProvider()->GetColor(ui::kColorResultsTableDimmedText),
        gfx::Rect(label_x_align_left, entry_rect.y(), label_width,
                  entry_rect.height()),
        text_align);
  }
}

void AutofillPopupView::CreateChildViews() {
  if (!popup_)
    return;

  RemoveAllChildViews();

  for (int i = 0; i < popup_->line_count(); ++i) {
    auto child_view =
        std::make_unique<AutofillPopupChildView>(popup_->value_at(i));
    child_view->set_drag_controller(this);
    AddChildView(std::move(child_view));
  }
}

void AutofillPopupView::DoUpdateBoundsAndRedrawPopup() {
  if (!popup_)
    return;

  // Clamp popup_bounds_ to ensure it's never zero-width.
  popup_->popup_bounds_.Union(
      gfx::Rect(popup_->popup_bounds_.origin(), gfx::Size(1, 1)));
  GetWidget()->SetBounds(popup_->popup_bounds_);
  if (view_proxy_.get()) {
    view_proxy_->SetBounds(popup_->popup_bounds_in_view());
  }
  SchedulePaint();
}

void AutofillPopupView::OnPaint(gfx::Canvas* canvas) {
  if (!popup_ || static_cast<size_t>(popup_->line_count()) != children().size())
    return;

  gfx::Rect offscreen_bounds;
  SkBitmap offscreen_bitmap;
  std::optional<cc::SkiaPaintCanvas> offscreen_paint_canvas;
  std::optional<gfx::Canvas> offscreen_draw_canvas;
  if (view_proxy_) {
    offscreen_bounds = popup_->popup_bounds_in_view();
    offscreen_bitmap.allocN32Pixels(offscreen_bounds.width(),
                                    offscreen_bounds.height(), true);
    offscreen_paint_canvas.emplace(offscreen_bitmap);
    offscreen_draw_canvas.emplace(&offscreen_paint_canvas.value(), 1.0);
    canvas = &offscreen_draw_canvas.value();
  }

  canvas->DrawColor(
      GetColorProvider()->GetColor(ui::kColorResultsTableNormalBackground));
  OnPaintBorder(canvas);

  for (int i = 0; i < popup_->line_count(); ++i) {
    gfx::Rect line_rect = popup_->GetRowBounds(i);

    DrawAutofillEntry(canvas, i, line_rect);
  }

  if (view_proxy_) {
    view_proxy_->SetBounds(offscreen_bounds);
    view_proxy_->SetBitmap(offscreen_bitmap);
  }
}

void AutofillPopupView::OnMouseCaptureLost() {
  ClearSelection();
}

bool AutofillPopupView::OnMouseDragged(const ui::MouseEvent& event) {
  if (HitTestPoint(event.location())) {
    SetSelection(event.location());

    // We must return true in order to get future OnMouseDragged and
    // OnMouseReleased events.
    return true;
  }

  // If we move off of the popup, we lose the selection.
  ClearSelection();
  return false;
}

void AutofillPopupView::OnMouseExited(const ui::MouseEvent& event) {
  // Pressing return causes the cursor to hide, which will generate an
  // OnMouseExited event. Pressing return should activate the current selection
  // via AcceleratorPressed, so we need to let that run first.
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&AutofillPopupView::ClearSelection,
                                weak_ptr_factory_.GetWeakPtr()));
}

void AutofillPopupView::OnMouseMoved(const ui::MouseEvent& event) {
  // A synthesized mouse move will be sent when the popup is first shown.
  // Don't preview a suggestion if the mouse happens to be hovering there.
#if BUILDFLAG(IS_WIN)
  if (base::Time::Now() - show_time_ <= base::Milliseconds(50))
    return;
#else
  if (event.flags() & ui::EF_IS_SYNTHESIZED)
    return;
#endif

  if (HitTestPoint(event.location()))
    SetSelection(event.location());
  else
    ClearSelection();
}

bool AutofillPopupView::OnMousePressed(const ui::MouseEvent& event) {
  return event.GetClickCount() == 1;
}

void AutofillPopupView::OnMouseReleased(const ui::MouseEvent& event) {
  // We only care about the left click.
  if (event.IsOnlyLeftMouseButton() && HitTestPoint(event.location()))
    AcceptSelection(event.location());
}

void AutofillPopupView::OnGestureEvent(ui::GestureEvent* event) {
  switch (event->type()) {
    case ui::EventType::kGestureTapDown:
    case ui::EventType::kGestureScrollBegin:
    case ui::EventType::kGestureScrollUpdate:
      if (HitTestPoint(event->location()))
        SetSelection(event->location());
      else
        ClearSelection();
      break;
    case ui::EventType::kGestureTap:
    case ui::EventType::kGestureScrollEnd:
      if (HitTestPoint(event->location()))
        AcceptSelection(event->location());
      else
        ClearSelection();
      break;
    case ui::EventType::kGestureTapCancel:
    case ui::EventType::kScrollFlingStart:
      ClearSelection();
      break;
    default:
      return;
  }
  event->SetHandled();
}

bool AutofillPopupView::AcceleratorPressed(const ui::Accelerator& accelerator) {
  if (accelerator.modifiers() != ui::EF_NONE)
    return false;

  if (accelerator.key_code() == ui::VKEY_ESCAPE) {
    if (popup_)
      popup_->Hide();
    return true;
  }

  if (accelerator.key_code() == ui::VKEY_RETURN)
    return AcceptSelectedLine();

  return false;
}

bool AutofillPopupView::HandleKeyPressEvent(
    const input::NativeWebKeyboardEvent& event) {
  if (!popup_)
    return false;
  switch (event.windows_key_code) {
    case ui::VKEY_UP:
      SelectPreviousLine();
      return true;
    case ui::VKEY_DOWN:
      SelectNextLine();
      return true;
    case ui::VKEY_PRIOR:  // Page up.
      SetSelectedLine(0);
      return true;
    case ui::VKEY_NEXT:  // Page down.
      SetSelectedLine(popup_->line_count() - 1);
      return true;
    case ui::VKEY_ESCAPE:
      popup_->Hide();
      return true;
    case ui::VKEY_TAB:
      // A tab press should cause the selected line to be accepted, but still
      // return false so the tab key press propagates and changes the cursor
      // location.
      AcceptSelectedLine();
      return false;
    case ui::VKEY_RETURN:
      return AcceptSelectedLine();
    default:
      return false;
  }
}

void AutofillPopupView::OnNativeFocusChanged(gfx::NativeView focused_now) {
  if (GetWidget() && GetWidget()->GetNativeView() != focused_now && popup_)
    popup_->Hide();
}

void AutofillPopupView::OnWidgetBoundsChanged(views::Widget* widget,
                                              const gfx::Rect& new_bounds) {
  if (widget != parent_widget_)
    return;
  if (popup_)
    popup_->Hide();
}

void AutofillPopupView::AcceptSuggestion(int index) {
  if (!popup_)
    return;

  popup_->AcceptSuggestion(index);
  popup_->Hide();
}

bool AutofillPopupView::AcceptSelectedLine() {
  if (!selected_line_ || selected_line_.value() >= popup_->line_count())
    return false;

  AcceptSuggestion(selected_line_.value());
  return true;
}

void AutofillPopupView::AcceptSelection(const gfx::Point& point) {
  if (!popup_)
    return;

  SetSelectedLine(popup_->LineFromY(point.y()));
  AcceptSelectedLine();
}

void AutofillPopupView::SetSelectedLine(std::optional<int> selected_line) {
  if (!popup_)
    return;
  if (selected_line_ == selected_line)
    return;
  if (selected_line && selected_line.value() >= popup_->line_count())
    return;

  auto previous_selected_line(selected_line_);
  selected_line_ = selected_line;
  OnSelectedRowChanged(previous_selected_line, selected_line_);
}

void AutofillPopupView::SetSelection(const gfx::Point& point) {
  if (!popup_)
    return;

  SetSelectedLine(popup_->LineFromY(point.y()));
}

void AutofillPopupView::SelectNextLine() {
  if (!popup_)
    return;

  int new_selected_line = selected_line_ ? *selected_line_ + 1 : 0;
  if (new_selected_line >= popup_->line_count())
    new_selected_line = 0;

  SetSelectedLine(new_selected_line);
}

void AutofillPopupView::SelectPreviousLine() {
  if (!popup_)
    return;

  int new_selected_line = selected_line_.value_or(0) - 1;
  if (new_selected_line < 0)
    new_selected_line = popup_->line_count() - 1;

  SetSelectedLine(new_selected_line);
}

void AutofillPopupView::ClearSelection() {
  SetSelectedLine(std::nullopt);
}

void AutofillPopupView::RemoveObserver() {
  parent_widget_->RemoveObserver(this);
  views::NativeViewFocusManager::GetInstance()->RemoveFocusChangeListener(this);
}

}  // namespace electron
