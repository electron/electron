"""python-dbusmock template for the xdg-desktop-portal interfaces Electron uses.

Loaded onto the fake session bus by script/dbus_mock.py. Implements
FileChooser and Inhibit.

FileChooser records every OpenFile/SaveFile request (inspectable via
org.freedesktop.DBus.Mock GetCalls) and answers it with a "user cancelled"
Response so dialogs resolve without interaction.

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
INHIBIT_IFACE = 'org.freedesktop.portal.Inhibit'
SESSION_IFACE = 'org.freedesktop.portal.Session'

# org.freedesktop.portal.Inhibit session states.
SESSION_STATE_RUNNING = 1
SESSION_STATE_QUERY_END = 2
SESSION_STATE_ENDING = 3

# https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Request.html
RESPONSE_SUCCESS = 0
RESPONSE_USER_CANCELLED = 1


def load(mock, parameters):
    """Set up the FileChooser and Inhibit interfaces on the mock object."""
    mock.request_serial = 0
    mock.queued_responses = []
    mock.monitor_serial = 0
    mock.monitor_sessions = []
    mock.AddProperty(MAIN_IFACE, 'version',
                     dbus.UInt32(parameters.get('version', 4)))
    mock.AddMethods(MAIN_IFACE, [
        ('OpenFile', 'ssa{sv}', 'o', handle_file_chooser_call),
        ('SaveFile', 'ssa{sv}', 'o', handle_file_chooser_call),
    ])
    mock.AddProperty(INHIBIT_IFACE, 'version',
                     dbus.UInt32(parameters.get('inhibit_version', 4)))


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
    # returned request path, and repeat it until the client acknowledges by
    # closing the request (a slow subscriber cannot miss it).
    state = {'remaining': 25}

    def emit_response():
        if not request.response_pending:
            return False
        request.EmitSignal(
            REQUEST_IFACE, 'Response', 'ua{sv}',
            [response, dbus.Dictionary(results, signature='sv')])
        state['remaining'] -= 1
        return state['remaining'] > 0

    GLib.timeout_add(200, emit_response)

    return dbus.ObjectPath(request_path)


@dbus.service.method(INHIBIT_IFACE, in_signature='sa{sv}', out_signature='o',
                     sender_keyword='sender')
def CreateMonitor(self, _parent_window, options, sender=None):
    """Create a session state monitor and answer its request.

    The session handle is reported through the Request Response signal, as the
    portal does. Like the portal, it is built from the caller's unique name and
    session_handle_token, so a client that predicts it (as Chromium does) sees
    StateChanged signals sent before the Response arrives.
    """
    self.monitor_serial += 1
    token = str(options.get('session_handle_token', f'mock{self.monitor_serial}'))
    sender_id = str(sender).removeprefix(':').replace('.', '_')
    session_path = f'{MAIN_OBJ}/session/{sender_id}/{token}'
    self.AddObject(session_path, SESSION_IFACE, {},
                   [('Close', '', '', 'pass')])
    self.monitor_sessions.append(session_path)

    self.request_serial += 1
    request_path = f'{MAIN_OBJ}/request/mock/{self.request_serial}'
    self.AddObject(request_path, REQUEST_IFACE, {}, [('Close', '', '', 'pass')])
    request = mockobject.objects[request_path]

    def emit_response():
        request.EmitSignal(
            REQUEST_IFACE, 'Response', 'ua{sv}',
            [dbus.UInt32(0),
             dbus.Dictionary({'session_handle': dbus.String(session_path)},
                             signature='sv')])
        return False

    GLib.timeout_add(200, emit_response)
    return dbus.ObjectPath(request_path)


@dbus.service.method(MOCK_IFACE, in_signature='u', out_signature='')
def EmitInhibitState(self, state):
    """Report a session state for every monitored session."""
    for handler in self.monitor_sessions:
        self.EmitSignal(
            INHIBIT_IFACE, 'StateChanged', 'oa{sv}',
            [dbus.ObjectPath(handler),
             dbus.Dictionary({'session-state': dbus.UInt32(state)},
                             signature='sv')])
