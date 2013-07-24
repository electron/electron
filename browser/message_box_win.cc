// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/message_box.h"

#include "base/message_loop.h"
#include "base/run_loop.h"
#include "base/utf_string_conversions.h"
#include "browser/native_window.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/message_box_view.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/window/client_view.h"

namespace atom {

namespace {

class MessageDialog : public base::MessageLoop::Dispatcher,
                      public views::WidgetDelegate,
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
  virtual views::ClientView* CreateClientView(views::Widget* widget) OVERRIDE;
  virtual ui::ModalType GetModalType() const OVERRIDE;

  // Overridden from views::ButtonListener:
  virtual void ButtonPressed(views::Button* sender,
                             const ui::Event& event) OVERRIDE;

  bool should_close_;
  views::Widget* widget_;
  views::MessageBoxView* message_box_view_;

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
    : should_close_(false) {
  views::MessageBoxView::InitParams params(UTF8ToUTF16(title));
  params.message = UTF8ToUTF16(message);
  message_box_view_ = new views::MessageBoxView(params);

  views::Widget::InitParams widget_params;
  widget_params.delegate = this;
  if (parent_window)
    widget_params.parent = parent_window->GetNativeWindow();
  widget_ = new views::Widget;
  widget_->Init(widget_params);

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

views::ClientView* MessageDialog::CreateClientView(views::Widget* widget) {
  return new views::ClientView(widget, message_box_view_);
}

ui::ModalType MessageDialog::GetModalType() const {
  return ui::MODAL_TYPE_WINDOW;
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
