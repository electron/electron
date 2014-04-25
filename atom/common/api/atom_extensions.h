// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Multiply-included file, no traditional include guard.

// Used by atom_extensions.cc to declare a list of built-in modules of Atom.

NODE_EXT_LIST_START

// Module names start with `atom_browser_` can only be used by browser process.
NODE_EXT_LIST_ITEM(atom_browser_app)
NODE_EXT_LIST_ITEM(atom_browser_auto_updater)
NODE_EXT_LIST_ITEM(atom_browser_dialog)
NODE_EXT_LIST_ITEM(atom_browser_menu)
NODE_EXT_LIST_ITEM(atom_browser_power_monitor)
NODE_EXT_LIST_ITEM(atom_browser_protocol)
NODE_EXT_LIST_ITEM(atom_browser_window)

// Module names start with `atom_renderer_` can only be used by renderer
// process.
NODE_EXT_LIST_ITEM(atom_renderer_ipc)

// Module names start with `atom_common_` can be used by both browser and
// renderer processes.
NODE_EXT_LIST_ITEM(atom_common_clipboard)
NODE_EXT_LIST_ITEM(atom_common_crash_reporter)
NODE_EXT_LIST_ITEM(atom_common_id_weak_map)
NODE_EXT_LIST_ITEM(atom_common_screen)
NODE_EXT_LIST_ITEM(atom_common_shell)
NODE_EXT_LIST_ITEM(atom_common_v8_util)

NODE_EXT_LIST_END
