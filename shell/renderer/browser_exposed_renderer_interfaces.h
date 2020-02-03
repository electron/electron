// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_RENDERER_BROWSER_EXPOSED_RENDERER_INTERFACES_H_
#define SHELL_RENDERER_BROWSER_EXPOSED_RENDERER_INTERFACES_H_

namespace mojo {
class BinderMap;
}  // namespace mojo

namespace electron {
class RendererClientBase;
}  // namespace electron

class ChromeContentRendererClient;

void ExposeElectronRendererInterfacesToBrowser(
    electron::RendererClientBase* client,
    mojo::BinderMap* binders);

#endif  // SHELL_RENDERER_BROWSER_EXPOSED_RENDERER_INTERFACES_H_
