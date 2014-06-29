// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTK2UI_GTK2_SIGNAL_H_
#define CHROME_BROWSER_UI_LIBGTK2UI_GTK2_SIGNAL_H_

#include "ui/base/glib/glib_signal.h"

typedef struct _GtkWidget GtkWidget;

// These macros handle the common case where the sender object will be a
// GtkWidget*.
#define CHROMEGTK_CALLBACK_0(CLASS, RETURN, METHOD) \
  CHROMEG_CALLBACK_0(CLASS, RETURN, METHOD, GtkWidget*);

#define CHROMEGTK_CALLBACK_1(CLASS, RETURN, METHOD, ARG1) \
  CHROMEG_CALLBACK_1(CLASS, RETURN, METHOD, GtkWidget*, ARG1);

#define CHROMEGTK_CALLBACK_2(CLASS, RETURN, METHOD, ARG1, ARG2) \
  CHROMEG_CALLBACK_2(CLASS, RETURN, METHOD, GtkWidget*, ARG1, ARG2);

#define CHROMEGTK_CALLBACK_3(CLASS, RETURN, METHOD, ARG1, ARG2, ARG3) \
  CHROMEG_CALLBACK_3(CLASS, RETURN, METHOD, GtkWidget*, ARG1, ARG2, ARG3);

#define CHROMEGTK_CALLBACK_4(CLASS, RETURN, METHOD, ARG1, ARG2, ARG3, ARG4) \
  CHROMEG_CALLBACK_4(CLASS, RETURN, METHOD, GtkWidget*, ARG1, ARG2, ARG3,   \
                     ARG4);

#define CHROMEGTK_CALLBACK_5(CLASS, RETURN, METHOD, ARG1, ARG2, ARG3, ARG4, \
                             ARG5)                                          \
  CHROMEG_CALLBACK_5(CLASS, RETURN, METHOD, GtkWidget*, ARG1, ARG2, ARG3,   \
                     ARG4, ARG5);

#define CHROMEGTK_CALLBACK_6(CLASS, RETURN, METHOD, ARG1, ARG2, ARG3, ARG4, \
                             ARG5, ARG6)                                    \
  CHROMEG_CALLBACK_6(CLASS, RETURN, METHOD, GtkWidget*, ARG1, ARG2, ARG3,   \
                     ARG4, ARG5, ARG6);

#define CHROMEGTK_VIRTUAL_CALLBACK_0(CLASS, RETURN, METHOD) \
  CHROMEG_VIRTUAL_CALLBACK_0(CLASS, RETURN, METHOD, GtkWidget*);

#define CHROMEGTK_VIRTUAL_CALLBACK_1(CLASS, RETURN, METHOD, ARG1) \
  CHROMEG_VIRTUAL_CALLBACK_1(CLASS, RETURN, METHOD, GtkWidget*, ARG1);

#define CHROMEGTK_VIRTUAL_CALLBACK_2(CLASS, RETURN, METHOD, ARG1, ARG2) \
  CHROMEG_VIRTUAL_CALLBACK_2(CLASS, RETURN, METHOD, GtkWidget*, ARG1, ARG2);

#define CHROMEGTK_VIRTUAL_CALLBACK_3(CLASS, RETURN, METHOD, ARG1, ARG2, ARG3) \
  CHROMEG_VIRTUAL_CALLBACK_3(CLASS, RETURN, METHOD, GtkWidget*, ARG1, ARG2,   \
                             ARG3);

#define CHROMEGTK_VIRTUAL_CALLBACK_4(CLASS, RETURN, METHOD, ARG1, ARG2, ARG3, \
                                     ARG4) \
  CHROMEG_VIRTUAL_CALLBACK_4(CLASS, RETURN, METHOD, GtkWidget*, ARG1, ARG2,   \
                             ARG3, ARG4);

#define CHROMEGTK_VIRTUAL_CALLBACK_5(CLASS, RETURN, METHOD, ARG1, ARG2, ARG3, \
                                     ARG4, ARG5)                              \
  CHROMEG_VIRTUAL_CALLBACK_5(CLASS, RETURN, METHOD, GtkWidget*, ARG1, ARG2,   \
                             ARG3, ARG4, ARG5);

#define CHROMEGTK_VIRTUAL_CALLBACK_6(CLASS, RETURN, METHOD, ARG1, ARG2, ARG3, \
                                     ARG4, ARG5, ARG6)                        \
  CHROMEG_VIRTUAL_CALLBACK_6(CLASS, RETURN, METHOD, GtkWidget*, ARG1, ARG2,   \
                             ARG3, ARG4, ARG5, ARG6);

#endif  // CHROME_BROWSER_UI_LIBGTK2UI_GTK2_SIGNAL_H_
