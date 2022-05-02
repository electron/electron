// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/serial/electron_serial_delegate.h"

#include <utility>

#include "base/feature_list.h"
#include "content/public/browser/web_contents.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/serial/serial_chooser_context.h"
#include "shell/browser/serial/serial_chooser_context_factory.h"
#include "shell/browser/serial/serial_chooser_controller.h"
#include "shell/browser/web_contents_permission_helper.h"

namespace electron {

SerialChooserContext* GetChooserContext(content::RenderFrameHost* frame) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(frame);
  auto* browser_context = web_contents->GetBrowserContext();
  return SerialChooserContextFactory::GetForBrowserContext(browser_context);
}

ElectronSerialDelegate::ElectronSerialDelegate() = default;

ElectronSerialDelegate::~ElectronSerialDelegate() = default;

std::unique_ptr<content::SerialChooser> ElectronSerialDelegate::RunChooser(
    content::RenderFrameHost* frame,
    std::vector<blink::mojom::SerialPortFilterPtr> filters,
    content::SerialChooser::Callback callback) {
  SerialChooserController* controller = ControllerForFrame(frame);
  if (controller) {
    DeleteControllerForFrame(frame);
  }
  AddControllerForFrame(frame, std::move(filters), std::move(callback));

  // Return a nullptr because the return value isn't used for anything, eg
  // there is no mechanism to cancel navigator.serial.requestPort(). The return
  // value is simply used in Chromium to cleanup the chooser UI once the serial
  // service is destroyed.
  return nullptr;
}

bool ElectronSerialDelegate::CanRequestPortPermission(
    content::RenderFrameHost* frame) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(frame);
  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  return permission_helper->CheckSerialAccessPermission(
      web_contents->GetMainFrame()->GetLastCommittedOrigin());
}

bool ElectronSerialDelegate::HasPortPermission(
    content::RenderFrameHost* frame,
    const device::mojom::SerialPortInfo& port) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(frame);
  auto* browser_context = web_contents->GetBrowserContext();
  auto* chooser_context =
      SerialChooserContextFactory::GetForBrowserContext(browser_context);
  return chooser_context->HasPortPermission(
      web_contents->GetMainFrame()->GetLastCommittedOrigin(), port, frame);
}

void ElectronSerialDelegate::RevokePortPermissionWebInitiated(
    content::RenderFrameHost* frame,
    const base::UnguessableToken& token) {
  return GetChooserContext(frame)->RevokePortPermissionWebInitiated(
      frame->GetMainFrame()->GetLastCommittedOrigin(), token);
}

const device::mojom::SerialPortInfo* ElectronSerialDelegate::GetPortInfo(
    content::RenderFrameHost* frame,
    const base::UnguessableToken& token) {
  return GetChooserContext(frame)->GetPortInfo(token);
}

device::mojom::SerialPortManager* ElectronSerialDelegate::GetPortManager(
    content::RenderFrameHost* frame) {
  return GetChooserContext(frame)->GetPortManager();
}

void ElectronSerialDelegate::AddObserver(content::RenderFrameHost* frame,
                                         Observer* observer) {
  return GetChooserContext(frame)->AddPortObserver(observer);
}

void ElectronSerialDelegate::RemoveObserver(content::RenderFrameHost* frame,
                                            Observer* observer) {
  SerialChooserContext* serial_chooser_context = GetChooserContext(frame);
  if (serial_chooser_context) {
    return serial_chooser_context->RemovePortObserver(observer);
  }
}

SerialChooserController* ElectronSerialDelegate::ControllerForFrame(
    content::RenderFrameHost* render_frame_host) {
  auto mapping = controller_map_.find(render_frame_host);
  return mapping == controller_map_.end() ? nullptr : mapping->second.get();
}

SerialChooserController* ElectronSerialDelegate::AddControllerForFrame(
    content::RenderFrameHost* render_frame_host,
    std::vector<blink::mojom::SerialPortFilterPtr> filters,
    content::SerialChooser::Callback callback) {
  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  auto controller = std::make_unique<SerialChooserController>(
      render_frame_host, std::move(filters), std::move(callback), web_contents,
      weak_factory_.GetWeakPtr());
  controller_map_.insert(
      std::make_pair(render_frame_host, std::move(controller)));
  return ControllerForFrame(render_frame_host);
}

void ElectronSerialDelegate::DeleteControllerForFrame(
    content::RenderFrameHost* render_frame_host) {
  controller_map_.erase(render_frame_host);
}

}  // namespace electron
