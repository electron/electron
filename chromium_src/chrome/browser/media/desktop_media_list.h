// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_DESKTOP_MEDIA_LIST_H_
#define CHROME_BROWSER_MEDIA_DESKTOP_MEDIA_LIST_H_

#include "base/basictypes.h"
#include "base/time/time.h"
#include "content/public/browser/desktop_media_id.h"
#include "ui/gfx/image/image_skia.h"

class DesktopMediaListObserver;

// DesktopMediaList provides the list of desktop media source (screens, windows,
// tabs), and their thumbnails, to the desktop media picker dialog. It
// transparently updates the list in the background, and notifies the desktop
// media picker when something changes.
class DesktopMediaList {
 public:
  // Struct used to represent each entry in the list.
  struct Source {
    // Id of the source.
    content::DesktopMediaID id;

    // Name of the source that should be shown to the user.
    base::string16 name;

    // The thumbnail for the source.
    gfx::ImageSkia thumbnail;
  };

  virtual ~DesktopMediaList() {}

  // Sets time interval between updates. By default list of sources and their
  // thumbnail are updated once per second. If called after StartUpdating() then
  // it will take effect only after the next update.
  virtual void SetUpdatePeriod(base::TimeDelta period) = 0;

  // Sets size to which the thumbnails should be scaled. If called after
  // StartUpdating() then some thumbnails may be still scaled to the old size
  // until they are updated.
  virtual void SetThumbnailSize(const gfx::Size& thumbnail_size) = 0;

  // Sets ID of the hosting desktop picker dialog. The window with this ID will
  // be filtered out from the list of sources.
  virtual void SetViewDialogWindowId(content::DesktopMediaID::Id dialog_id) = 0;

  // Starts updating the model. The model is initially empty, so OnSourceAdded()
  // notifications will be generated for each existing source as it is
  // enumerated. After the initial enumeration the model will be refreshed based
  // on the update period, and notifications generated only for changes in the
  // model.
  virtual void StartUpdating(DesktopMediaListObserver* observer) = 0;

  virtual int GetSourceCount() const = 0;
  virtual const Source& GetSource(int index) const = 0;
};

#endif  // CHROME_BROWSER_MEDIA_DESKTOP_MEDIA_LIST_H_
