// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/message_box.h"

#if defined(USE_X11)
#include <gtk/gtk.h>
#endif

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
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/wm/core/shadow_types.h"

#if defined(USE_X11)
#include "atom/browser/browser.h"
#include "ui/views/window/native_frame_view.h"
#endif

#if defined(OS_WIN)
#include "ui/base/win/message_box_win.h"
#endif

#define ANSI_FOREGROUND_RED   "\x1b[31m"
#define ANSI_FOREGROUND_BLACK "\x1b[30m"
#define ANSI_TEXT_BOLD        "\x1b[1m"
#define ANSI_BACKGROUND_GRAY  "\x1b[47m"
#define ANSI_RESET            "\x1b[0m"

namespace atom {

namespace {

// The group used by the buttons.  This name is chosen voluntarily big not to
// conflict with other groups that could be in the dialog content.
const int kButtonGroup = 1127;

class MessageDialogClientView;

class MessageDialog : public views::WidgetDelegate,
                      public views::View,
                      public views::ButtonListener {
 public:
  MessageDialog(NativeWindow* parent_window,
                MessageBoxType type,
                const std::vector<std::string>& buttons,
                const std::string& title,
                const std::string& message,
                const std::string& detail,
                const gfx::ImageSkia& icon);
  virtual ~MessageDialog();

  void Show(base::RunLoop* run_loop = NULL);
  void Close();

  int GetResult() const;

  void set_callback(const MessageBoxCallback& callback) {
    delete_on_close_ = true;
    callback_ = callback;
  }

 private:
  // Overridden from views::WidgetDelegate:
  base::string16 GetWindowTitle() const override;
  gfx::ImageSkia GetWindowAppIcon() override;
  gfx::ImageSkia GetWindowIcon() override;
  bool ShouldShowWindowIcon() const override;
  views::Widget* GetWidget() override;
  const views::Widget* GetWidget() const override;
  views::View* GetContentsView() override;
  views::View* GetInitiallyFocusedView() override;
  ui::ModalType GetModalType() const override;
  views::NonClientFrameView* CreateNonClientFrameView(
      views::Widget* widget) override;
  views::ClientView* CreateClientView(views::Widget* widget) override;

  // Overridden from views::View:
  gfx::Size GetPreferredSize() const override;
  void Layout() override;
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;

  // Overridden from views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  gfx::ImageSkia icon_;

  bool delete_on_close_;
  int result_;
  base::string16 title_;

  NativeWindow* parent_;
  scoped_ptr<views::Widget> widget_;
  views::MessageBoxView* message_box_view_;
  std::vector<views::LabelButton*> buttons_;

  base::RunLoop* run_loop_;
  scoped_ptr<NativeWindow::DialogScope> dialog_scope_;
  MessageBoxCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(MessageDialog);
};

class MessageDialogClientView : public views::ClientView {
 public:
  MessageDialogClientView(MessageDialog* dialog, views::Widget* widget)
      : views::ClientView(widget, dialog),
        dialog_(dialog) {
  }

  // views::ClientView:
  bool CanClose() override {
    dialog_->Close();
    return false;
  }

 private:
  MessageDialog* dialog_;

  DISALLOW_COPY_AND_ASSIGN(MessageDialogClientView);
};

////////////////////////////////////////////////////////////////////////////////
// MessageDialog, public:

MessageDialog::MessageDialog(NativeWindow* parent_window,
                             MessageBoxType type,
                             const std::vector<std::string>& buttons,
                             const std::string& title,
                             const std::string& message,
                             const std::string& detail,
                             const gfx::ImageSkia& icon)
    : icon_(icon),
      delete_on_close_(false),
      result_(-1),
      title_(base::UTF8ToUTF16(title)),
      parent_(parent_window),
      message_box_view_(NULL),
      run_loop_(NULL),
      dialog_scope_(new NativeWindow::DialogScope(parent_window)) {
  DCHECK_GT(buttons.size(), 0u);
  set_owned_by_client();

  if (!parent_)
    set_background(views::Background::CreateStandardPanelBackground());

  std::string content = message + "\n" + detail;
  views::MessageBoxView::InitParams box_params(base::UTF8ToUTF16(content));
  message_box_view_ = new views::MessageBoxView(box_params);
  AddChildView(message_box_view_);

  for (size_t i = 0; i < buttons.size(); ++i) {
    views::LabelButton* button = new views::LabelButton(
        this, base::UTF8ToUTF16(buttons[i]));
    button->set_tag(i);
    button->SetMinSize(gfx::Size(60, 30));
    button->SetStyle(views::Button::STYLE_BUTTON);
    button->SetGroup(kButtonGroup);

    buttons_.push_back(button);
    AddChildView(button);
  }

  // First button is always default button.
  buttons_[0]->SetIsDefault(true);
  buttons_[0]->AddAccelerator(ui::Accelerator(ui::VKEY_RETURN, ui::EF_NONE));

  views::Widget::InitParams params;
  params.delegate = this;
  params.type = views::Widget::InitParams::TYPE_WINDOW;
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  if (parent_) {
    params.parent = parent_->GetNativeWindow();
    params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
    // Use bubble style for dialog has a parent.
    params.remove_standard_frame = true;
  }

  widget_.reset(new views::Widget);
  widget_->Init(params);
  widget_->UpdateWindowIcon();

  // Bind to ESC.
  AddAccelerator(ui::Accelerator(ui::VKEY_ESCAPE, ui::EF_NONE));
}

MessageDialog::~MessageDialog() {
}

void MessageDialog::Show(base::RunLoop* run_loop) {
  run_loop_ = run_loop;
  widget_->Show();
}

void MessageDialog::Close() {
  dialog_scope_.reset();

  if (delete_on_close_) {
    callback_.Run(GetResult());
    base::MessageLoop::current()->DeleteSoon(FROM_HERE, this);
  } else if (run_loop_) {
    run_loop_->Quit();
  }
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

gfx::ImageSkia MessageDialog::GetWindowAppIcon() {
  return icon_;
}

gfx::ImageSkia MessageDialog::GetWindowIcon() {
  return icon_;
}

bool MessageDialog::ShouldShowWindowIcon() const {
  return true;
}

views::Widget* MessageDialog::GetWidget() {
  return widget_.get();
}

const views::Widget* MessageDialog::GetWidget() const {
  return widget_.get();
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
  return ui::MODAL_TYPE_SYSTEM;
}

views::NonClientFrameView* MessageDialog::CreateNonClientFrameView(
    views::Widget* widget) {
  if (!parent_) {
#if defined(USE_X11)
    return new views::NativeFrameView(widget);
#else
    return NULL;
#endif
  }

  // Create a bubble style frame like Chrome.
  views::BubbleFrameView* frame =  new views::BubbleFrameView(gfx::Insets());
  const SkColor color = widget->GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_DialogBackground);
  scoped_ptr<views::BubbleBorder> border(new views::BubbleBorder(
      views::BubbleBorder::FLOAT, views::BubbleBorder::SMALL_SHADOW, color));
  frame->SetBubbleBorder(border.Pass());
  wm::SetShadowType(widget->GetNativeWindow(), wm::SHADOW_TYPE_NONE);
  return frame;
}

views::ClientView* MessageDialog::CreateClientView(views::Widget* widget) {
  return new MessageDialogClientView(this, widget);
}

gfx::Size MessageDialog::GetPreferredSize() const {
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
                   const std::string& detail,
                   const gfx::ImageSkia& icon) {
  MessageDialog dialog(
      parent_window, type, buttons, title, message, detail, icon);
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
                    const gfx::ImageSkia& icon,
                    const MessageBoxCallback& callback) {
  // The dialog would be deleted when the dialog is closed.
  MessageDialog* dialog = new MessageDialog(
      parent_window, type, buttons, title, message, detail, icon);
  dialog->set_callback(callback);
  dialog->Show();
}

void ShowErrorBox(const base::string16& title, const base::string16& content) {
#if defined(OS_WIN)
  ui::MessageBox(NULL, content, title, MB_OK | MB_ICONERROR | MB_TASKMODAL);
#elif defined(USE_X11)
  if (Browser::Get()->is_ready()) {
    GtkWidget* dialog = gtk_message_dialog_new(
        NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
        "%s", base::UTF16ToUTF8(title).c_str());
    gtk_message_dialog_format_secondary_text(
        GTK_MESSAGE_DIALOG(dialog),
        "%s", base::UTF16ToUTF8(content).c_str());
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
  } else {
    fprintf(stderr,
            ANSI_TEXT_BOLD ANSI_BACKGROUND_GRAY
            ANSI_FOREGROUND_RED  "%s\n"
            ANSI_FOREGROUND_BLACK "%s"
            ANSI_RESET "\n",
            base::UTF16ToUTF8(title).c_str(),
            base::UTF16ToUTF8(content).c_str());
  }
#endif
}

}  // namespace atom
