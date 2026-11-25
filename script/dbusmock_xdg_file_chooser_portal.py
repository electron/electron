"""python-dbusmock template for the xdg-desktop-portal FileChooser interface.

Loaded onto the fake session bus by script/dbus_mock.py. Records every
OpenFile/SaveFile request (inspectable via org.freedesktop.DBus.Mock
GetCalls) and answers it with a "user cancelled" Response so dialogs
resolve without interaction.

A spec can make the next request succeed instead by calling QueueResponse
(on the org.freedesktop.DBus.Mock interface) with the file:// URIs it should
return. Each queued response answers exactly one request; once the queue is
empty, requests are cancelled again. ClearQueuedResponses drops any responses
still queued, so one spec's leftovers cannot answer another spec's request.
"""

import dbus

from dbusmock import MOCK_IFACE, mockobject
from gi.repository import GLib

BUS_NAME = 'org.freedesktop.portal.Desktop'
MAIN_OBJ = '/org/freedesktop/portal/desktop'
MAIN_IFACE = 'org.freedesktop.portal.FileChooser'
SYSTEM_BUS = False

REQUEST_IFACE = 'org.freedesktop.portal.Request'

# https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Request.html
RESPONSE_SUCCESS = 0
RESPONSE_USER_CANCELLED = 1

# Chromium treats an accept that arrives less than 500ms after it made the
# request (kFileDialogMinDisplayDuration) as a cancellation, since no user
# could have seen the dialog by then. Answer well after that.
RESPONSE_DELAY_MS = 1000
# Interval at which an unacknowledged Response is repeated.
RESPONSE_REPEAT_MS = 200


def load(mock, parameters):
    """Set up the FileChooser interface on the mock object."""
    mock.request_serial = 0
    mock.queued_responses = []
    mock.AddProperty(MAIN_IFACE, 'version',
                     dbus.UInt32(parameters.get('version', 4)))
    mock.AddMethods(MAIN_IFACE, [
        ('OpenFile', 'ssa{sv}', 'o', handle_file_chooser_call),
        ('SaveFile', 'ssa{sv}', 'o', handle_file_chooser_call),
    ])


@dbus.service.method(MOCK_IFACE, in_signature='as', out_signature='')
def QueueResponse(self, uris):
    """Answer the next OpenFile/SaveFile request with the given URIs."""
    self.queued_responses.append(list(uris))


@dbus.service.method(MOCK_IFACE, in_signature='', out_signature='')
def ClearQueuedResponses(self):
    """Drop any responses queued by QueueResponse."""
    self.queued_responses.clear()


def handle_file_chooser_call(self, _parent_window, _title, _options):
    """Create a Request object and answer it shortly afterwards.

    The answer is the oldest queued response, or "user cancelled".
    """
    if self.queued_responses:
        response = dbus.UInt32(RESPONSE_SUCCESS)
        results = {'uris': dbus.Array(self.queued_responses.pop(0),
                                      signature='s')}
    else:
        response = dbus.UInt32(RESPONSE_USER_CANCELLED)
        results = {}

    self.request_serial += 1
    request_path = f'{MAIN_OBJ}/request/mock/{self.request_serial}'
    self.AddObject(request_path, REQUEST_IFACE, {},
                   [('Close', '', '', 'self.response_pending = False')])
    request = mockobject.objects[request_path]
    request.response_pending = True

    # Emit the Response on a delay so the client has re-subscribed to the
    # returned request path and will accept it, and repeat it until the
    # client acknowledges by closing the request (a slow subscriber cannot
    # miss it).
    state = {'remaining': 25}

    def emit_response():
        if not request.response_pending:
            return False
        request.EmitSignal(
            REQUEST_IFACE, 'Response', 'ua{sv}',
            [response, dbus.Dictionary(results, signature='sv')])
        state['remaining'] -= 1
        return state['remaining'] > 0

    def start_responding():
        if emit_response():
            GLib.timeout_add(RESPONSE_REPEAT_MS, emit_response)
        return False

    GLib.timeout_add(RESPONSE_DELAY_MS, start_responding)

    return dbus.ObjectPath(request_path)
