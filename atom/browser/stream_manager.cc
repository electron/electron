// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/stream_manager.h"

#include "base/memory/ptr_util.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"

namespace atom {

StreamManager::StreamManager() {}

StreamManager::~StreamManager() {}

void StreamManager::AddStream(std::unique_ptr<content::StreamInfo> stream,
                              const std::string& stream_id,
                              int render_process_id,
                              int render_frame_id) {
  streams_.insert(std::make_pair(stream_id, std::move(stream)));
  observers_[stream_id] = base::MakeUnique<EmbedderObserver>(
      this, stream_id, render_process_id, render_frame_id);
}

std::unique_ptr<content::StreamInfo> StreamManager::ReleaseStream(
    const std::string& stream_id) {
  auto stream = streams_.find(stream_id);
  if (stream == streams_.end())
    return nullptr;

  std::unique_ptr<content::StreamInfo> result =
      base::WrapUnique(stream->second.release());
  streams_.erase(stream);
  observers_.erase(stream_id);
  return result;
}

StreamManager::EmbedderObserver::EmbedderObserver(StreamManager* stream_manager,
                                                  const std::string& stream_id,
                                                  int render_process_id,
                                                  int render_frame_id)
    : stream_manager_(stream_manager), stream_id_(stream_id) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(
          content::RenderFrameHost::FromID(render_process_id, render_frame_id));
  content::WebContentsObserver::Observe(web_contents);
}

void StreamManager::EmbedderObserver::RenderProcessGone(
    base::TerminationStatus status) {
  AbortStream();
}

void StreamManager::EmbedderObserver::WebContentsDestroyed() {
  AbortStream();
}

void StreamManager::EmbedderObserver::AbortStream() {
  Observe(nullptr);
  stream_manager_->ReleaseStream(stream_id_);
}

}  // namespace atom
