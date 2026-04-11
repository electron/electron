#ifndef SRC_DIALOG_HELPER_H_
#define SRC_DIALOG_HELPER_H_

#include <cstddef>
#include <string>

namespace dialog_helper {

struct DialogInfo {
  // "message-box", "open-dialog", "save-dialog", or "none"
  std::string type;
  // Button titles for message boxes
  std::string buttons;  // JSON array string, e.g. '["OK","Cancel"]'
  // Message text (NSAlert messageText or panel title)
  std::string message;
  // Detail / informative text (NSAlert informativeText)
  std::string detail;
  // Checkbox (suppression button) label, empty if none
  std::string checkbox_label;
  // Whether the checkbox is checked
  bool checkbox_checked = false;

  // File dialog properties (open/save panels)
  std::string prompt;         // Button label (NSSavePanel prompt)
  std::string panel_message;  // Panel message text (NSSavePanel message)
  std::string directory;      // Current directory URL path

  // NSSavePanel-specific properties
  std::string name_field_label;  // Label for the name field
  std::string name_field_value;  // Current value of the name field
  bool shows_tag_field = true;

  // NSOpenPanel-specific properties
  bool can_choose_files = false;
  bool can_choose_directories = false;
  bool allows_multiple_selection = false;

  // Shared panel properties (open and save)
  bool shows_hidden_files = false;
  bool resolves_aliases = true;
  bool treats_packages_as_directories = false;
  bool can_create_directories = false;
};

// Get information about the dialog attached to the window identified by the
// given native handle buffer (NSView* on macOS, HWND on Windows).
DialogInfo GetDialogInfo(char* handle, size_t size);

// Click a button at the given index on a message box dialog attached to the
// window. Returns true if a message box was found and the button was clicked.
bool ClickMessageBoxButton(char* handle, size_t size, int button_index);

// Toggle the checkbox (suppression button) on a message box dialog.
// Returns true if a checkbox was found and clicked.
bool ClickCheckbox(char* handle, size_t size);

// Cancel the file dialog (open/save) attached to the window.
// Returns true if a file dialog was found and cancelled.
bool CancelFileDialog(char* handle, size_t size);

// Accept the file dialog attached to the window.
// For save dialogs, |filename| is set in the name field before accepting.
// Returns true if a file dialog was found and accepted.
bool AcceptFileDialog(char* handle, size_t size, const std::string& filename);

// Starts a background task that waits for a message box attached to the given
// window, clicks the button at |button_index|, waits for the dialog to close,
// and then sends a Tab key to the active window after |post_close_delay_ms|.
// Returns true if the automation task was started.
bool ClickMessageBoxButtonAndSendTabLater(char* handle,
                                          size_t size,
                                          int button_index,
                                          int post_close_delay_ms);

}  // namespace dialog_helper

#endif  // SRC_DIALOG_HELPER_H_
