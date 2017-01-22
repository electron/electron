// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_STREAM_MANAGER_H_
#define ATOM_BROWSER_STREAM_MANAGER_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "content/public/browser/stream_info.h"
#include "content/public/browser/web_contents_observer.h"

namespace atom {

// A container for streams that have not been claimed by associated
// WebContents.
class StreamManager {
 public:
  StreamManager();
  ~StreamManager();

  void AddStream(std::unique_ptr<content::StreamInfo> stream,
                 const std::string& view_id,
                 int render_process_id,
                 int render_frame_id);

  std::unique_ptr<content::StreamInfo> ReleaseStream(
      const std::string& view_id);

 private:
  // WebContents observer that deletes an unclaimed stream
  // associated with it, when destroyed.
  class EmbedderObserver : public content::WebContentsObserver {
   public:
    EmbedderObserver(StreamManager* stream_manager,
                     const std::string& view_id,
                     int render_process_id,
                     int render_frame_id);

   private:
    // content::WebContentsObserver:
    void RenderProcessGone(base::TerminationStatus status) override;
    void WebContentsDestroyed() override;

    void AbortStream();

    StreamManager* stream_manager_;
    std::string view_id_;

    DISALLOW_COPY_AND_ASSIGN(EmbedderObserver);
  };

  std::map<std::string, std::unique_ptr<content::StreamInfo>> streams_;

  std::map<std::string, std::unique_ptr<EmbedderObserver>> observers_;

  DISALLOW_COPY_AND_ASSIGN(StreamManager);
};

}  // namespace atom

#endif  // ATOM_BROWSER_STREAM_MANAGER_H_
