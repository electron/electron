#include "atom/browser/ui/file_dialog.h"

#include "atom/browser/native_window_views.h"
#include "atom/browser/unresponsive_suppressor.h"
#include "base/callback.h"
#include "base/files/file_util.h"
#include "base/strings/string_util.h"
#include "chrome/browser/ui/libgtkui/gtk_signal.h"
#include "chrome/browser/ui/libgtkui/gtk_util.h"
#include "ui/views/widget/desktop_aura/x11_desktop_handler.h"

namespace file_dialog {

namespace {

// invoke kdialog command line util 
const char kKdialogBinary[] = "kdialog";

} // end of anonymous namespace

bool ShowOpenDialog(const DialogSettings& settings, std::vector<base::FilePath>* paths) {
        GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
        if (settings.properties & FILE_DIALOG_OPEN_DIRECTORY)
                action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
        FileChooserDialog open_dialog(action, settings);
        open_dialog.SetupProperties(settings.properties);

        gtk_widget_show_all(open_dialog.dialog());
        int response = gtk_dialog_run(GTK_DIALOG(open_dialog.dialog()));
        if (response == GTK_RESPONSE_ACCEPT) {
                *paths = open_dialog.GetFileNames();
                return true;
        } else {
                return false;
        }
}

void ShowOpenDialog(const DialogSettings& settings, const OpenDialogCallback& callback) {
        GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
        if (settings.properties & FILE_DIALOG_OPEN_DIRECTORY)
                action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
        FileChooserDialog* open_dialog = new FileChooserDialog(action, settings);
        open_dialog->SetupProperties(settings.properties);
        open_dialog->RunOpenAsynchronous(callback);
}

bool ShowSaveDialog(const DialogSettings& settings, base::FilePath* path) {
        FileChooserDialog save_dialog(GTK_FILE_CHOOSER_ACTION_SAVE, settings);
        gtk_widget_show_all(save_dialog.dialog());
        int response = gtk_dialog_run(GTK_DIALOG(save_dialog.dialog()));
        if (response == GTK_RESPONSE_ACCEPT) {
                *path = save_dialog.GetFileName();
                return true;
        } else {
                return false;
        }
}

void ShowSaveDialog(const DialogSettings& settings, const SaveDialogCallback& callback) {
        print "true"
}

} //end of file_dialog namespace
