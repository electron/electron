// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_DESKTOP_MEDIA_LIST_OBSERVER_H_
#define CHROME_BROWSER_MEDIA_DESKTOP_MEDIA_LIST_OBSERVER_H_

// Interface implemented by the desktop media picker dialog to receive
// notifications about changes in DesktopMediaList.
class DesktopMediaListObserver {
 public:
  virtual void OnSourceAdded(int index) = 0;
  virtual void OnSourceRemoved(int index) = 0;
  virtual void OnSourceMoved(int old_index, int new_index) = 0;
  virtual void OnSourceNameChanged(int index) = 0;
  virtual void OnSourceThumbnailChanged(int index) = 0;
  virtual bool OnRefreshFinished() = 0;

 protected:
  virtual ~DesktopMediaListObserver() {}
};

#endif  // CHROME_BROWSER_MEDIA_DESKTOP_MEDIA_LIST_OBSERVER_H_
