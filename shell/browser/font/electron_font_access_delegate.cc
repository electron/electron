// Copyright (c) 2021 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <utility>

#include "shell/browser/font/electron_font_access_delegate.h"

#include "base/task/post_task.h"
#include "chrome/browser/browser_process.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/font_access_chooser.h"
#include "content/public/browser/render_frame_host.h"

class ElectronFontAccessChooser : public content::FontAccessChooser {
 public:
  explicit ElectronFontAccessChooser(base::OnceClosure close_callback) {
    base::PostTask(
        FROM_HERE, {content::BrowserThread::UI},
        base::BindOnce(
            [](base::OnceClosure closure) { std::move(closure).Run(); },
            std::move(close_callback)));
  }
  ~ElectronFontAccessChooser() override = default;
};

ElectronFontAccessDelegate::ElectronFontAccessDelegate() = default;
ElectronFontAccessDelegate::~ElectronFontAccessDelegate() = default;

std::unique_ptr<content::FontAccessChooser>
ElectronFontAccessDelegate::RunChooser(
    content::RenderFrameHost* frame,
    const std::vector<std::string>& selection,
    content::FontAccessChooser::Callback callback) {
  // TODO(codebytere) : implement with proper permissions model.
  return std::make_unique<ElectronFontAccessChooser>(base::BindOnce(
      [](content::FontAccessChooser::Callback callback) {
        std::move(callback).Run(
            blink::mojom::FontEnumerationStatus::kUnimplemented, {});
      },
      std::move(callback)));
}
