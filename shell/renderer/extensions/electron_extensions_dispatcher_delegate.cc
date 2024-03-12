// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/renderer/extensions/electron_extensions_dispatcher_delegate.h"

ElectronExtensionsDispatcherDelegate::ElectronExtensionsDispatcherDelegate() =
    default;

ElectronExtensionsDispatcherDelegate::~ElectronExtensionsDispatcherDelegate() =
    default;

void ElectronExtensionsDispatcherDelegate::RequireWebViewModules(
    extensions::ScriptContext* context) {}

void ElectronExtensionsDispatcherDelegate::OnActiveExtensionsUpdated(
    const std::set<std::string>& extension_ids) {}
