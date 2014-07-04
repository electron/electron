// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/message_box.h"

#include "atom/browser/native_window.h"
#include "base/callback.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/message_box_view.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/layout/layout_constants.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace atom {

namespace {

// The group used by the buttons.  This name is chosen voluntarily big not to
// conflict with other groups that could be in the dialog content.
const int kButtonGroup = 1127;

class MessageDialog : public views::WidgetDelegate,
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

  void Show(base::RunLoop* run_loop = NULL);

  int GetResult() const;

  void set_callback(const MessageBoxCallback& callback) {
    delete_on_close_ = true;
    callback_ = callback;
  }

 private:
  // Overridden from views::WidgetDelegate:
  virtual base::string16 GetWindowTitle() const;
  virtual void WindowClosing() OVERRIDE;
  virtual views::Widget* GetWidget() OVERRIDE;
  virtual const views::Widget* GetWidget() const OVERRIDE;
  virtual views::View* GetContentsView() OVERRIDE;
  virtual views::View* GetInitiallyFocusedView() OVERRIDE;
  virtual ui::ModalType GetModalType() const OVERRIDE;

  // Overridden from views::View:
  virtual gfx::Size GetPreferredSize() OVERRIDE;
  virtual void Layout() OVERRIDE;
  virtual bool AcceleratorPressed(const ui::Accelerator& accelerator) OVERRIDE;

  // Overridden from views::ButtonListener:
  virtual void ButtonPressed(views::Button* sender,
                             const ui::Event& event) OVERRIDE;

  bool should_close_;
  bool delete_on_close_;
  int result_;
  base::string16 title_;
  views::Widget* widget_;
  views::MessageBoxView* message_box_view_;
  base::RunLoop* run_loop_;
  scoped_ptr<NativeWindow::DialogScope> dialog_scope_;
  std::vector<views::LabelButton*> buttons_;
  MessageBoxCallback callback_;

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
      delete_on_close_(false),
      result_(-1),
      title_(base::UTF8ToUTF16(title)),
      widget_(NULL),
      message_box_view_(NULL),
      run_loop_(NULL),
      dialog_scope_(new NativeWindow::DialogScope(parent_window)) {
  DCHECK_GT(buttons.size(), 0u);
  set_owned_by_client();
  set_background(views::Background::CreateStandardPanelBackground());

  std::string content = message + "\n" + detail;
  views::MessageBoxView::InitParams params(base::UTF8ToUTF16(content));
  message_box_view_ = new views::MessageBoxView(params);
  AddChildView(message_box_view_);

  for (size_t i = 0; i < buttons.size(); ++i) {
    views::LabelButton* button = new views::LabelButton(
        this, base::UTF8ToUTF16(buttons[i]));
    button->set_tag(i);
    button->set_min_size(gfx::Size(60, 30));
    button->SetStyle(views::Button::STYLE_BUTTON);
    button->SetHorizontalAlignment(gfx::ALIGN_CENTER);
    button->SetGroup(kButtonGroup);

    buttons_.push_back(button);
    AddChildView(button);
  }

  // First button is always default button.
  buttons_[0]->SetIsDefault(true);
  buttons_[0]->AddAccelerator(ui::Accelerator(ui::VKEY_RETURN, ui::EF_NONE));

  views::Widget::InitParams widget_params;
  widget_params.delegate = this;
  widget_params.top_level = true;
  widget_params.remove_standard_frame = true;
  if (parent_window)
    widget_params.parent = parent_window->GetNativeWindow();
  widget_ = new views::Widget;
  widget_->Init(widget_params);

  // Bind to ESC.
  AddAccelerator(ui::Accelerator(ui::VKEY_ESCAPE, ui::EF_NONE));
}

MessageDialog::~MessageDialog() {
}

void MessageDialog::Show(base::RunLoop* run_loop) {
  run_loop_ = run_loop;
  widget_->Show();
}

int MessageDialog::GetResult() const {
  // When the dialog is closed without choosing anything, we think the user
  // chose 'Cancel', otherwise we think the default behavior is chosen.
  if (result_ == -1) {
    for (size_t i = 0; i < buttons_.size(); ++i)
      if (LowerCaseEqualsASCII(buttons_[i]->GetText(), "cancel")) {
        return i;
      }

    return 0;
  } else {
    return result_;
  }
}

////////////////////////////////////////////////////////////////////////////////
// MessageDialog, private:

base::string16 MessageDialog::GetWindowTitle() const {
  return title_;
}

void MessageDialog::WindowClosing() {
  should_close_ = true;
  dialog_scope_.reset();

  if (delete_on_close_) {
    callback_.Run(GetResult());
    base::MessageLoop::current()->DeleteSoon(FROM_HERE, this);
  } else if (run_loop_) {
    run_loop_->Quit();
  }
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

views::View* MessageDialog::GetInitiallyFocusedView() {
  if (buttons_.size() > 0)
    return buttons_[0];
  else
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

  // NB: We iterate through the buttons backwards here because
  // Mac and Windows buttons are laid out in opposite order.
  for (int i = buttons_.size() - 1; i >= 0; --i) {
    gfx::Size size = buttons_[i]->GetPreferredSize();
    x -= size.width() + views::kRelatedButtonHSpacing;

    buttons_[i]->SetBounds(x, bounds.height() - height,
                           size.width(), size.height());
  }

  // Layout the message box view.
  message_box_view_->SetBounds(bounds.x(), bounds.y(), bounds.width(),
                               bounds.height() - height);
}

bool MessageDialog::AcceleratorPressed(const ui::Accelerator& accelerator) {
  DCHECK_EQ(accelerator.key_code(), ui::VKEY_ESCAPE);
  widget_->Close();
  return true;
}

void MessageDialog::ButtonPressed(views::Button* sender,
                                  const ui::Event& event) {
  result_ = sender->tag();
  widget_->Close();
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
    base::RunLoop run_loop;
    dialog.Show(&run_loop);
    run_loop.Run();
  }

  return dialog.GetResult();
}

void ShowMessageBox(NativeWindow* parent_window,
                    MessageBoxType type,
                    const std::vector<std::string>& buttons,
                    const std::string& title,
                    const std::string& message,
                    const std::string& detail,
                    const MessageBoxCallback& callback) {
  // The dialog would be deleted when the dialog is closed.
  MessageDialog* dialog = new MessageDialog(
      parent_window, type, buttons, title, message, detail);
  dialog->set_callback(callback);
  dialog->Show();
}

}  // namespace atom
