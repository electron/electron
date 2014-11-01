// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_COLOR_CHOOSER_DIALOG_H_
#define CHROME_BROWSER_UI_VIEWS_COLOR_CHOOSER_DIALOG_H_

#include "base/memory/ref_counted.h"
#include "chrome/browser/ui/views/color_chooser_dialog.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/shell_dialogs/base_shell_dialog.h"
#include "ui/shell_dialogs/base_shell_dialog_win.h"

namespace views {
class ColorChooserListener;
}

class ColorChooserDialog
    : public base::RefCountedThreadSafe<ColorChooserDialog>,
      public ui::BaseShellDialog,
      public ui::BaseShellDialogImpl {
 public:
  ColorChooserDialog(views::ColorChooserListener* listener,
                     SkColor initial_color,
                     gfx::NativeWindow owning_window);
  virtual ~ColorChooserDialog();

  // BaseShellDialog:
  virtual bool IsRunning(gfx::NativeWindow owning_window) const OVERRIDE;
  virtual void ListenerDestroyed() OVERRIDE;

 private:
  struct ExecuteOpenParams {
    ExecuteOpenParams(SkColor color, RunState run_state, HWND owner);
    SkColor color;
    RunState run_state;
    HWND owner;
  };

  // Called on the dialog thread to show the actual color chooser.  This is
  // shown modal to |params.owner|.  Once it's closed, calls back to
  // DidCloseDialog() on the UI thread.
  void ExecuteOpen(const ExecuteOpenParams& params);

  // Called on the UI thread when a color chooser is closed.  |chose_color| is
  // true if the user actually chose a color, in which case |color| is the
  // chosen color.  Calls back to the |listener_| (if applicable) to notify it
  // of the results, and copies the modified array of |custom_colors_| back to
  // |g_custom_colors| so future dialogs will see the changes.
  void DidCloseDialog(bool chose_color, SkColor color, RunState run_state);

  // Copies the array of colors in |src| to |dst|.
  void CopyCustomColors(COLORREF*, COLORREF*);

  // The user's custom colors.  Kept process-wide so that they can be persisted
  // from one dialog invocation to the next.
  static COLORREF g_custom_colors[16];

  // A copy of the custom colors for the current dialog to display and modify.
  // This allows us to safely access the colors even if multiple windows are
  // simultaneously showing color choosers (which would cause thread safety
  // problems if we gave them direct handles to |g_custom_colors|).
  COLORREF custom_colors_[16];

  // The listener to notify when the user closes the dialog.  This may be set to
  // NULL before the color chooser is closed, signalling that the listener no
  // longer cares about the outcome.
  views::ColorChooserListener* listener_;

  DISALLOW_COPY_AND_ASSIGN(ColorChooserDialog);
};

#endif  // CHROME_BROWSER_UI_VIEWS_COLOR_CHOOSER_DIALOG_H_
