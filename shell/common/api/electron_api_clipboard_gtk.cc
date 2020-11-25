// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/api/electron_api_clipboard.h"

#include <gtk/gtk.h>

namespace electron {

namespace api {

namespace {

void ClipboardGet(GtkClipboard* clipboard,
                  GtkSelectionData* selection,
                  guint info,
                  gchar** uris) {
  DCHECK_EQ(info, 0u);
  gtk_selection_data_set_uris(selection, uris);
}

void ClipboardClear(GtkClipboard* clipboard, gchar** uris) {
  g_strfreev(uris);
}

}  // namespace

std::vector<base::FilePath> Clipboard::ReadFilePaths() {
  std::vector<base::FilePath> result;
  GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  gchar** uris = gtk_clipboard_wait_for_uris(clipboard);
  if (!uris)
    return result;
  for (int i = 0; uris[i]; i++) {
    if (gchar* path = g_filename_from_uri(uris[i], nullptr, nullptr)) {
      result.emplace_back(path);
      g_free(path);
    }
  }
  g_strfreev(uris);
  return result;
}

void Clipboard::WriteFilePaths(const std::vector<base::FilePath>& paths) {
  // Convert vector to gchar* array, will be destroyed by ClipboardClear.
  gchar** uris = g_new(gchar*, paths.size() + 1);
  if (!uris)
    return;
  size_t i;
  for (i = 0; i < paths.size(); ++i) {
    uris[i] = g_filename_to_uri(paths[i].value().c_str(), nullptr, nullptr);
  }
  uris[i] = nullptr;

  GtkTargetList* targets = gtk_target_list_new(nullptr, 0);
  gtk_target_list_add_uri_targets(targets, 0);
  int number = 0;
  GtkTargetEntry* table = gtk_target_table_new_from_list(targets, &number);
  DCHECK_EQ(number, 1);

  GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_set_with_data(clipboard, table, number,
                              (GtkClipboardGetFunc)ClipboardGet,
                              (GtkClipboardClearFunc)ClipboardClear, uris);

  gtk_target_list_unref(targets);
  gtk_target_table_free(table, number);
}

}  // namespace api

}  // namespace electron
