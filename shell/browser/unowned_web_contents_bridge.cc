// Copyright (c) 2026 Electron contributors.
// Use of this source code is governed by the MIT license.
#include "shell/browser/unowned_web_contents_bridge.h"

#include <utility>

#include "base/feature_list.h"
#include "base/unguessable_token.h"
#include "content/browser/renderer_host/frame_tree_node.h"         // nogncheck
#include "content/browser/renderer_host/render_frame_host_impl.h"  // nogncheck
#include "content/browser/renderer_host/render_frame_host_manager.h"  // nogncheck
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/unowned_inner_web_contents_client.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"

namespace electron {
namespace {
content::RenderFrameHostImpl* Impl(content::RenderFrameHost* frame) {
  return static_cast<content::RenderFrameHostImpl*>(frame);
}
bool ValidFrame(content::RenderFrameHost* frame,
                content::RenderFrameHost* owner) {
  return frame && owner && frame->IsActive() && owner->IsActive() &&
         frame->GetParent() == owner &&
         frame->GetLastCommittedURL().IsAboutBlank() &&
         !Impl(frame)->frame_tree_node()->HasPendingCrossDocumentNavigation() &&
         !Impl(frame)->frame_tree_node()->HasPendingCommitNavigation();
}
}  // namespace

UnownedWebContentsBridge::UnownedWebContentsBridge(
    content::WebContents* inner,
    base::RepeatingCallback<void(std::string)> detached)
    : inner_(inner->GetWeakPtr()), detached_(std::move(detached)) {}

UnownedWebContentsBridge::~UnownedWebContentsBridge() {
  Detach();
}

bool UnownedWebContentsBridge::IsAttached() const {
  return inner_ && outer_ && !attachment_id_.empty() &&
         inner_->GetOuterWebContents() == outer_.get();
}

std::string UnownedWebContentsBridge::Attach(content::RenderFrameHost* owner,
                                             const std::string& frame_token,
                                             std::string* error) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!base::FeatureList::IsEnabled(features::kAttachUnownedInnerWebContents) ||
      !inner_ || inner_->IsBeingDestroyed() || inner_->GetOuterWebContents()) {
    *error = "WebContents cannot be attached";
    return {};
  }
  auto token = base::Token::FromString(frame_token);
  if (!owner || !token) {
    *error = "Invalid attachment owner or frame token";
    return {};
  }
  auto unguessable =
      base::UnguessableToken::Deserialize(token->high(), token->low());
  auto* frame = unguessable ? content::RenderFrameHost::FromFrameToken(
                                  content::GlobalRenderFrameHostToken(
                                      owner->GetProcess()->GetDeprecatedID(),
                                      blink::LocalFrameToken(*unguessable)))
                            : nullptr;
  auto* outer = content::WebContents::FromRenderFrameHost(owner);
  if (!ValidFrame(frame, owner) || !frame->IsRenderFrameLive() ||
      !Impl(frame)->is_initial_empty_document() ||
      frame->GetSiteInstance() != owner->GetSiteInstance() ||
      !frame->GetLastCommittedURL().IsAboutBlank() ||
      Impl(frame)->inner_tree_main_frame_tree_node_id() ||
      Impl(frame)
          ->frame_tree_node()
          ->render_manager()
          ->is_attaching_inner_delegate() ||
      !outer || outer->IsBeingDestroyed() || outer == inner_.get() ||
      outer->GetBrowserContext() != inner_->GetBrowserContext() ||
      !outer->GetOrCreateWebPreferences().webview2_tag) {
    *error = "Attachment requires a fresh, empty local iframe in the host";
    return {};
  }
  for (auto* ancestor = outer; ancestor;
       ancestor = ancestor->GetOuterWebContents()) {
    if (ancestor == inner_.get()) {
      *error = "Attachment would create a contents cycle";
      return {};
    }
  }
  attachment_id_ = base::UnguessableToken::Create().ToString();
  outer_ = outer->GetWeakPtr();
  Observe(outer);
  outer->AttachUnownedInnerWebContents(
      content::UnownedInnerWebContentsClient::GetPassKey(), inner_.get(),
      frame);
  return attachment_id_;
}

bool UnownedWebContentsBridge::Detach() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!IsAttached())
    return false;
  outer_->DetachUnownedInnerWebContents(
      content::UnownedInnerWebContentsClient::GetPassKey(), inner_.get());
  return true;
}

void UnownedWebContentsBridge::UnownedInnerWebContentsDetached(
    content::WebContents* inner) {
  if (inner == inner_.get())
    CompleteDetach();
}

void UnownedWebContentsBridge::CompleteDetach() {
  auto id = std::exchange(attachment_id_, {});
  outer_.reset();
  Observe(nullptr);
  if (!id.empty())
    detached_.Run(std::move(id));
}

void UnownedWebContentsBridge::PrimaryMainFrameRenderProcessGone(
    base::TerminationStatus status) {
  Detach();
}

void UnownedWebContentsBridge::WebContentsDestroyed() {
  // Normally FrameTreeNode teardown already delivered the complete detach.
  CompleteDetach();
}
}  // namespace electron
