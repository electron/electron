"""python-dbusmock template for the xdg-desktop-portal FileChooser interface.

Loaded onto the fake session bus by script/dbus_mock.py so that Linux tests
exercise Chromium's portal-based file dialogs (SelectFileDialogLinuxPortal)
instead of falling back to the GTK implementation. Every OpenFile/SaveFile
request is automatically answered with a "user cancelled" Response so
dialogs resolve instead of waiting for interaction; tests inspect the
request options via the org.freedesktop.DBus.Mock interface (GetCalls).
"""

import dbus

from dbusmock import mockobject
from gi.repository import GLib

BUS_NAME = 'org.freedesktop.portal.Desktop'
MAIN_OBJ = '/org/freedesktop/portal/desktop'
MAIN_IFACE = 'org.freedesktop.portal.FileChooser'
SYSTEM_BUS = False

REQUEST_IFACE = 'org.freedesktop.portal.Request'

# https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Request.html
RESPONSE_USER_CANCELLED = 1


def load(mock, parameters):
    """Set up the FileChooser interface on the mock object."""
    mock.request_serial = 0
    mock.AddProperty(MAIN_IFACE, 'version',
                     dbus.UInt32(parameters.get('version', 4)))
    mock.AddMethods(MAIN_IFACE, [
        ('OpenFile', 'ssa{sv}', 'o', handle_file_chooser_call),
        ('SaveFile', 'ssa{sv}', 'o', handle_file_chooser_call),
    ])


def handle_file_chooser_call(self, _parent_window, _title, _options):
    """Create a Request object and cancel it shortly afterwards."""
    self.request_serial += 1
    request_path = f'{MAIN_OBJ}/request/mock/{self.request_serial}'
    self.AddObject(request_path, REQUEST_IFACE, {},
                   [('Close', '', '', 'self.response_pending = False')])
    request = mockobject.objects[request_path]
    request.response_pending = True

    # The client sees that the returned request path differs from the one it
    # derived from its handle_token and re-subscribes to the Response signal
    # on the returned path, per the portal spec. Emit the signal on a delay
    # so the re-subscription has completed, and repeat it a bounded number of
    # times so a slow subscriber cannot miss it. The client Closes the
    # request once it has processed a Response, which stops the emissions.
    state = {'remaining': 25}

    def emit_response():
        if not request.response_pending:
            return False
        request.EmitSignal(
            REQUEST_IFACE, 'Response', 'ua{sv}',
            [dbus.UInt32(RESPONSE_USER_CANCELLED),
             dbus.Dictionary({}, signature='sv')])
        state['remaining'] -= 1
        return state['remaining'] > 0

    GLib.timeout_add(200, emit_response)

    return dbus.ObjectPath(request_path)
