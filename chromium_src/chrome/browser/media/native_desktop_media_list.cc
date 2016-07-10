// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/native_desktop_media_list.h"

#include <map>
#include <set>
#include <sstream>

#include "base/hash.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/sequenced_worker_pool.h"
#include "chrome/browser/media/desktop_media_list_observer.h"
#include "content/public/browser/browser_thread.h"
#include "media/base/video_util.h"
#include "third_party/libyuv/include/libyuv/scale_argb.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_frame.h"
#include "third_party/webrtc/modules/desktop_capture/screen_capturer.h"
#include "third_party/webrtc/modules/desktop_capture/window_capturer.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/skia_util.h"

using content::BrowserThread;
using content::DesktopMediaID;

namespace {

// Update the list every second.
const int kDefaultUpdatePeriod = 1000;

// Returns a hash of a DesktopFrame content to detect when image for a desktop
// media source has changed.
uint32_t GetFrameHash(webrtc::DesktopFrame* frame) {
  int data_size = frame->stride() * frame->size().height();
  return base::SuperFastHash(reinterpret_cast<char*>(frame->data()), data_size);
}

gfx::ImageSkia ScaleDesktopFrame(std::unique_ptr<webrtc::DesktopFrame> frame,
                                 gfx::Size size) {
  gfx::Rect scaled_rect = media::ComputeLetterboxRegion(
      gfx::Rect(0, 0, size.width(), size.height()),
      gfx::Size(frame->size().width(), frame->size().height()));

  SkBitmap result;
  result.allocN32Pixels(scaled_rect.width(), scaled_rect.height(), true);
  result.lockPixels();

  uint8* pixels_data = reinterpret_cast<uint8*>(result.getPixels());
  libyuv::ARGBScale(frame->data(), frame->stride(),
                    frame->size().width(), frame->size().height(),
                    pixels_data, result.rowBytes(),
                    scaled_rect.width(), scaled_rect.height(),
                    libyuv::kFilterBilinear);

  // Set alpha channel values to 255 for all pixels.
  // TODO(sergeyu): Fix screen/window capturers to capture alpha channel and
  // remove this code. Currently screen/window capturers (at least some
  // implementations) only capture R, G and B channels and set Alpha to 0.
  // crbug.com/264424
  for (int y = 0; y < result.height(); ++y) {
    for (int x = 0; x < result.width(); ++x) {
      pixels_data[result.rowBytes() * y + x * result.bytesPerPixel() + 3] =
          0xff;
    }
  }

  result.unlockPixels();

  return gfx::ImageSkia::CreateFrom1xBitmap(result);
}

}  // namespace

NativeDesktopMediaList::SourceDescription::SourceDescription(
    DesktopMediaID id,
    const base::string16& name)
    : id(id),
      name(name) {
}

class NativeDesktopMediaList::Worker
    : public webrtc::DesktopCapturer::Callback {
 public:
  Worker(base::WeakPtr<NativeDesktopMediaList> media_list,
         std::unique_ptr<webrtc::ScreenCapturer> screen_capturer,
         std::unique_ptr<webrtc::WindowCapturer> window_capturer);
  ~Worker() override;

  void Refresh(const gfx::Size& thumbnail_size,
               content::DesktopMediaID::Id view_dialog_id);

 private:
  typedef std::map<DesktopMediaID, uint32> ImageHashesMap;

  // webrtc::DesktopCapturer::Callback interface.
  webrtc::SharedMemory* CreateSharedMemory(size_t size) override;
  void OnCaptureCompleted(webrtc::DesktopFrame* frame) override;

  base::WeakPtr<NativeDesktopMediaList> media_list_;

  std::unique_ptr<webrtc::ScreenCapturer> screen_capturer_;
  std::unique_ptr<webrtc::WindowCapturer> window_capturer_;

  std::unique_ptr<webrtc::DesktopFrame> current_frame_;

  ImageHashesMap image_hashes_;

  DISALLOW_COPY_AND_ASSIGN(Worker);
};

NativeDesktopMediaList::Worker::Worker(
    base::WeakPtr<NativeDesktopMediaList> media_list,
    std::unique_ptr<webrtc::ScreenCapturer> screen_capturer,
    std::unique_ptr<webrtc::WindowCapturer> window_capturer)
    : media_list_(media_list),
      screen_capturer_(std::move(screen_capturer)),
      window_capturer_(std::move(window_capturer)) {
  if (screen_capturer_)
    screen_capturer_->Start(this);
  if (window_capturer_)
    window_capturer_->Start(this);
}

NativeDesktopMediaList::Worker::~Worker() {}

void NativeDesktopMediaList::Worker::Refresh(
    const gfx::Size& thumbnail_size,
    content::DesktopMediaID::Id view_dialog_id) {
  std::vector<SourceDescription> sources;

  if (screen_capturer_) {
    webrtc::ScreenCapturer::ScreenList screens;
    if (screen_capturer_->GetScreenList(&screens)) {
      bool mutiple_screens = screens.size() > 1;
      base::string16 title;
      for (size_t i = 0; i < screens.size(); ++i) {
        if (mutiple_screens) {
          title = base::UTF8ToUTF16("Screen " + base::IntToString(i+1));
        } else {
          title = base::UTF8ToUTF16("Entire screen");
        }
        sources.push_back(SourceDescription(DesktopMediaID(
            DesktopMediaID::TYPE_SCREEN, screens[i].id), title));
      }
    }
  }

  if (window_capturer_) {
    webrtc::WindowCapturer::WindowList windows;
    if (window_capturer_->GetWindowList(&windows)) {
      for (auto& window : windows) {
        // Skip the picker dialog window.
        if (window.id != view_dialog_id) {
          sources.push_back(SourceDescription(
              DesktopMediaID(DesktopMediaID::TYPE_WINDOW, window.id),
              base::UTF8ToUTF16(window.title)));
        }
      }
    }
  }
  // Update list of windows before updating thumbnails.
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&NativeDesktopMediaList::OnSourcesList,
                 media_list_, sources));

  ImageHashesMap new_image_hashes;

  // Get a thumbnail for each source.
  for (size_t i = 0; i < sources.size(); ++i) {
    SourceDescription& source = sources[i];
    switch (source.id.type) {
      case DesktopMediaID::TYPE_SCREEN:
        if (!screen_capturer_->SelectScreen(source.id.id))
          continue;
        screen_capturer_->Capture(webrtc::DesktopRegion());
        break;

      case DesktopMediaID::TYPE_WINDOW:
        if (!window_capturer_->SelectWindow(source.id.id))
          continue;
        window_capturer_->Capture(webrtc::DesktopRegion());
        break;

      default:
        NOTREACHED();
    }

    // Expect that DesktopCapturer to always captures frames synchronously.
    // |current_frame_| may be NULL if capture failed (e.g. because window has
    // been closed).
    if (current_frame_) {
      uint32_t frame_hash = GetFrameHash(current_frame_.get());
      new_image_hashes[source.id] = frame_hash;

      // Scale the image only if it has changed.
      auto it = image_hashes_.find(source.id);
      if (it == image_hashes_.end() || it->second != frame_hash) {
        gfx::ImageSkia thumbnail =
            ScaleDesktopFrame(std::move(current_frame_), thumbnail_size);
        BrowserThread::PostTask(
            BrowserThread::UI, FROM_HERE,
            base::Bind(&NativeDesktopMediaList::OnSourceThumbnail,
                        media_list_, i, thumbnail));
      }
    }
  }

  image_hashes_.swap(new_image_hashes);

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&NativeDesktopMediaList::OnRefreshFinished, media_list_));
}

webrtc::SharedMemory* NativeDesktopMediaList::Worker::CreateSharedMemory(
    size_t size) {
  return nullptr;
}

void NativeDesktopMediaList::Worker::OnCaptureCompleted(
    webrtc::DesktopFrame* frame) {
  current_frame_.reset(frame);
}

NativeDesktopMediaList::NativeDesktopMediaList(
    std::unique_ptr<webrtc::ScreenCapturer> screen_capturer,
    std::unique_ptr<webrtc::WindowCapturer> window_capturer)
    : screen_capturer_(std::move(screen_capturer)),
      window_capturer_(std::move(window_capturer)),
      update_period_(base::TimeDelta::FromMilliseconds(kDefaultUpdatePeriod)),
      thumbnail_size_(100, 100),
      view_dialog_id_(-1),
      observer_(nullptr),
      weak_factory_(this) {
  base::SequencedWorkerPool* worker_pool = BrowserThread::GetBlockingPool();
  capture_task_runner_ = worker_pool->GetSequencedTaskRunner(
      worker_pool->GetSequenceToken());
}

NativeDesktopMediaList::~NativeDesktopMediaList() {
  capture_task_runner_->DeleteSoon(FROM_HERE, worker_.release());
}

void NativeDesktopMediaList::SetUpdatePeriod(base::TimeDelta period) {
  DCHECK(!observer_);
  update_period_ = period;
}

void NativeDesktopMediaList::SetThumbnailSize(
    const gfx::Size& thumbnail_size) {
  thumbnail_size_ = thumbnail_size;
}

void NativeDesktopMediaList::SetViewDialogWindowId(
    content::DesktopMediaID::Id dialog_id) {
  view_dialog_id_ = dialog_id;
}

void NativeDesktopMediaList::StartUpdating(DesktopMediaListObserver* observer) {
  DCHECK(!observer_);
  DCHECK(screen_capturer_ || window_capturer_);

  observer_ = observer;

  worker_.reset(new Worker(weak_factory_.GetWeakPtr(),
                           std::move(screen_capturer_),
                           std::move(window_capturer_)));
  Refresh();
}

int NativeDesktopMediaList::GetSourceCount() const {
  return sources_.size();
}

const DesktopMediaList::Source& NativeDesktopMediaList::GetSource(
    int index) const {
  return sources_[index];
}

std::vector<DesktopMediaList::Source> NativeDesktopMediaList::GetSources() const {
  return sources_;
}

void NativeDesktopMediaList::Refresh() {
  capture_task_runner_->PostTask(
      FROM_HERE, base::Bind(&Worker::Refresh, base::Unretained(worker_.get()),
                            thumbnail_size_, view_dialog_id_));
}

void NativeDesktopMediaList::OnSourcesList(
    const std::vector<SourceDescription>& new_sources) {
  typedef std::set<content::DesktopMediaID> SourceSet;
  SourceSet new_source_set;
  for (const auto& new_source : new_sources) {
    new_source_set.insert(new_source.id);
  }
  // Iterate through the old sources to find the removed sources.
  for (size_t i = 0; i < sources_.size(); ++i) {
    if (new_source_set.find(sources_[i].id) == new_source_set.end()) {
      observer_->OnSourceRemoved(i);
      sources_.erase(sources_.begin() + i);
      --i;
    }
  }
  // Iterate through the new sources to find the added sources.
  if (new_sources.size() > sources_.size()) {
    SourceSet old_source_set;
    for (auto& source : sources_) {
      old_source_set.insert(source.id);
    }

    for (size_t i = 0; i < new_sources.size(); ++i) {
      if (old_source_set.find(new_sources[i].id) == old_source_set.end()) {
        sources_.insert(sources_.begin() + i, Source());
        sources_[i].id = new_sources[i].id;
        sources_[i].name = new_sources[i].name;
        observer_->OnSourceAdded(i);
      }
    }
  }
  DCHECK_EQ(new_sources.size(), sources_.size());

  // Find the moved/changed sources.
  size_t pos = 0;
  while (pos < sources_.size()) {
    if (!(sources_[pos].id == new_sources[pos].id)) {
      // Find the source that should be moved to |pos|, starting from |pos + 1|
      // of |sources_|, because entries before |pos| should have been sorted.
      size_t old_pos = pos + 1;
      for (; old_pos < sources_.size(); ++old_pos) {
        if (sources_[old_pos].id == new_sources[pos].id)
          break;
      }
      DCHECK(sources_[old_pos].id == new_sources[pos].id);

      // Move the source from |old_pos| to |pos|.
      Source temp = sources_[old_pos];
      sources_.erase(sources_.begin() + old_pos);
      sources_.insert(sources_.begin() + pos, temp);

      observer_->OnSourceMoved(old_pos, pos);
    }

    if (sources_[pos].name != new_sources[pos].name) {
      sources_[pos].name = new_sources[pos].name;
      observer_->OnSourceNameChanged(pos);
    }
    ++pos;
  }
}

void NativeDesktopMediaList::OnSourceThumbnail(
    int index,
    const gfx::ImageSkia& image) {
  DCHECK_LT(index, static_cast<int>(sources_.size()));
  sources_[index].thumbnail = image;
  observer_->OnSourceThumbnailChanged(index);
}

void NativeDesktopMediaList::OnRefreshFinished() {
  // Give a chance to the observer to stop the refresh work.
  bool is_continue = observer_->OnRefreshFinished();
  if (is_continue) {
    BrowserThread::PostDelayedTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&NativeDesktopMediaList::Refresh,
                   weak_factory_.GetWeakPtr()),
        update_period_);
  }
}
