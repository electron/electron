// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/message_box.h"

#include "base/message_loop.h"
#include "base/run_loop.h"
#include "base/utf_string_conversions.h"
#include "browser/native_window.h"
#include "skia/ext/skia_utils_win.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/message_box_view.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/layout/layout_constants.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace atom {

namespace {

class MessageDialog : public base::MessageLoop::Dispatcher,
                      public views::WidgetDelegate,
                      public views::View,
                      public views::ButtonListener {
 public:
  MessageDialog(NativeWindow* parent_window,
                MessageBoxType type,
                const std::vector<std::string>& buttons,
                const std::string& title,
                const std::string& message,
                const std::string& detail);
  virtual ~MessageDialog();

 private:
  // Overridden from MessageLoop::Dispatcher:
  virtual bool Dispatch(const base::NativeEvent& event) OVERRIDE;

  // Overridden from views::Widget:
  virtual void WindowClosing() OVERRIDE;
  virtual views::Widget* GetWidget() OVERRIDE;
  virtual const views::Widget* GetWidget() const OVERRIDE;
  virtual views::View* GetContentsView() OVERRIDE;
  virtual ui::ModalType GetModalType() const OVERRIDE;

  // Overridden from views::View:
  virtual gfx::Size GetPreferredSize() OVERRIDE;
  virtual void Layout() OVERRIDE;

  // Overridden from views::ButtonListener:
  virtual void ButtonPressed(views::Button* sender,
                             const ui::Event& event) OVERRIDE;

  bool should_close_;
  views::Widget* widget_;
  views::MessageBoxView* message_box_view_;
  std::vector<views::LabelButton*> buttons_;

  DISALLOW_COPY_AND_ASSIGN(MessageDialog);
};

////////////////////////////////////////////////////////////////////////////////
// MessageDialog, public:

MessageDialog::MessageDialog(NativeWindow* parent_window,
                             MessageBoxType type,
                             const std::vector<std::string>& buttons,
                             const std::string& title,
                             const std::string& message,
                             const std::string& detail)
    : should_close_(false),
      widget_(NULL),
      message_box_view_(NULL) {
  set_owned_by_client();

  views::MessageBoxView::InitParams params(UTF8ToUTF16(title));
  params.message = UTF8ToUTF16(message);
  message_box_view_ = new views::MessageBoxView(params);
  AddChildView(message_box_view_);

  for (size_t i = 0; i < buttons.size(); ++i) {
    views::LabelButton* button = new views::LabelButton(
        this, UTF8ToUTF16(buttons[i]));
    if (i == 0)
      button->SetIsDefault(true);
    button->set_tag(i);
    button->set_min_size(gfx::Size(60, 20));
    button->SetStyle(views::Button::STYLE_NATIVE_TEXTBUTTON);

    buttons_.push_back(button);
    AddChildView(button);
  }

  views::Widget::InitParams widget_params;
  widget_params.delegate = this;
  if (parent_window)
    widget_params.parent = parent_window->GetNativeWindow();
  widget_ = new views::Widget;
  widget_->set_frame_type(views::Widget::FRAME_TYPE_FORCE_NATIVE);
  widget_->Init(widget_params);

  set_background(views::Background::CreateSolidBackground(
        skia::COLORREFToSkColor(GetSysColor(COLOR_WINDOW))));
  widget_->Show();
}

MessageDialog::~MessageDialog() {
}

////////////////////////////////////////////////////////////////////////////////
// MessageDialog, private:

bool MessageDialog::Dispatch(const base::NativeEvent& event) {
  TranslateMessage(&event);
  DispatchMessage(&event);
  return !should_close_;
}

void MessageDialog::WindowClosing() {
  should_close_ = true;
}

views::Widget* MessageDialog::GetWidget() {
  return widget_;
}

const views::Widget* MessageDialog::GetWidget() const {
  return widget_;
}

views::View* MessageDialog::GetContentsView() {
  return this;
}

ui::ModalType MessageDialog::GetModalType() const {
  return ui::MODAL_TYPE_WINDOW;
}

gfx::Size MessageDialog::GetPreferredSize() {
  gfx::Size size(0, buttons_[0]->GetPreferredSize().height());
  for (size_t i = 0; i < buttons_.size(); ++i)
    size.Enlarge(buttons_[i]->GetPreferredSize().width(), 0);

  // Button spaces.
  size.Enlarge(views::kRelatedButtonHSpacing * (buttons_.size() - 1),
               views::kRelatedControlVerticalSpacing);

  // The message box view.
  gfx::Size contents_size = message_box_view_->GetPreferredSize();
  size.Enlarge(0, contents_size.height());
  if (contents_size.width() > size.width())
    size.set_width(contents_size.width());

  return size;
}

void MessageDialog::Layout() {
  gfx::Rect bounds = GetContentsBounds();

  // Layout the row containing the buttons.
  int x = bounds.width();
  int height = buttons_[0]->GetPreferredSize().height() +
               views::kRelatedControlVerticalSpacing;
  for (size_t i = 0; i < buttons_.size(); ++i) {
    gfx::Size size = buttons_[i]->GetPreferredSize();
    x -= size.width() + views::kRelatedButtonHSpacing;

    buttons_[i]->SetBounds(x, bounds.height() - height,
                           size.width(), size.height());
  }

  // Layout the message box view.
  message_box_view_->SetBounds(bounds.x(), bounds.y(), bounds.width(),
                               bounds.height() - height);
}

void MessageDialog::ButtonPressed(views::Button* sender,
                                  const ui::Event& event) {
}

}  // namespace

int ShowMessageBox(NativeWindow* parent_window,
                   MessageBoxType type,
                   const std::vector<std::string>& buttons,
                   const std::string& title,
                   const std::string& message,
                   const std::string& detail) {
  MessageDialog dialog(parent_window, type, buttons, title, message, detail);
  {
    base::MessageLoop::ScopedNestableTaskAllower allow(
        base::MessageLoopForUI::current());
    base::RunLoop run_loop(&dialog);
    run_loop.Run();
  }
  return 0;
}

}  // namespace atom
