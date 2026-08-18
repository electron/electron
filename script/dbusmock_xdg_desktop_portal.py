"""python-dbusmock template for the xdg-desktop-portal interfaces the specs use.

Loaded onto the fake session bus by script/dbus_mock.py.

* FileChooser: records every OpenFile/SaveFile request (inspectable via
  org.freedesktop.DBus.Mock GetCalls) and answers it with a "user cancelled"
  Response so dialogs resolve without interaction.
* GlobalShortcuts: CreateSession/BindShortcuts/ListShortcuts backed by an
  in-memory set of approved shortcuts that survives across sessions, like a
  compositor's store does across application launches. Specs seed and reset it
  through the org.electron.spec.GlobalShortcutsMock control interface and
  inspect the requests via GetCalls.
* host.portal.Registry: accepts Register so clients do not log a warning.
"""

import dbus

from dbusmock import mockobject
from gi.repository import GLib

BUS_NAME = 'org.freedesktop.portal.Desktop'
MAIN_OBJ = '/org/freedesktop/portal/desktop'
MAIN_IFACE = 'org.freedesktop.portal.FileChooser'
SYSTEM_BUS = False

GLOBAL_SHORTCUTS_IFACE = 'org.freedesktop.portal.GlobalShortcuts'
REGISTRY_IFACE = 'org.freedesktop.host.portal.Registry'
REQUEST_IFACE = 'org.freedesktop.portal.Request'
SESSION_IFACE = 'org.freedesktop.portal.Session'
CONTROL_IFACE = 'org.electron.spec.GlobalShortcutsMock'

# https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Request.html
RESPONSE_SUCCESS = 0
RESPONSE_USER_CANCELLED = 1
RESPONSE_OTHER = 2


def load(mock, parameters):
    """Set up the portal interfaces on the mock object."""
    mock.request_serial = 0
    mock.AddProperty(MAIN_IFACE, 'version',
                     dbus.UInt32(parameters.get('version', 4)))
    mock.AddMethods(MAIN_IFACE, [
        ('OpenFile', 'ssa{sv}', 'o', handle_file_chooser_call),
        ('SaveFile', 'ssa{sv}', 'o', handle_file_chooser_call),
    ])

    reset_global_shortcuts(mock)
    mock.AddProperty(GLOBAL_SHORTCUTS_IFACE, 'version', dbus.UInt32(1))
    mock.AddMethods(GLOBAL_SHORTCUTS_IFACE, [
        ('CreateSession', 'a{sv}', 'o', handle_create_session),
        ('BindShortcuts', 'oa(sa{sv})sa{sv}', 'o', handle_bind_shortcuts),
        ('ListShortcuts', 'oa{sv}', 'o', handle_list_shortcuts),
    ])
    mock.AddMethods(REGISTRY_IFACE, [
        ('Register', 'sa{sv}', '', ''),
    ])
    mock.AddMethods(CONTROL_IFACE, [
        ('Reset', '', '', reset_global_shortcuts),
        ('SetShortcuts', 'a(sss)', '', handle_set_shortcuts),
        ('GetShortcuts', '', 'a(sss)', handle_get_shortcuts),
        ('FailNextListShortcuts', 'u', '', handle_fail_next_list_shortcuts),
    ])


def send_response(self, code, results):
    """Create a Request object and deliver `results` on it shortly afterwards.

    The Response is emitted on a delay so the client has re-subscribed to the
    returned request path, and repeated until the client acknowledges by
    closing the request (a slow subscriber cannot miss it; clients drop their
    subscription once they have seen one Response).
    """
    self.request_serial += 1
    request_path = f'{MAIN_OBJ}/request/mock/{self.request_serial}'
    self.AddObject(request_path, REQUEST_IFACE, {},
                   [('Close', '', '', 'self.response_pending = False')])
    request = mockobject.objects[request_path]
    request.response_pending = True

    state = {'remaining': 25}

    def emit_response():
        if not request.response_pending:
            return False
        request.EmitSignal(
            REQUEST_IFACE, 'Response', 'ua{sv}',
            [dbus.UInt32(code), dbus.Dictionary(results, signature='sv')])
        state['remaining'] -= 1
        return state['remaining'] > 0

    GLib.timeout_add(100, emit_response)

    return dbus.ObjectPath(request_path)


def handle_file_chooser_call(self, _parent_window, _title, _options):
    """Cancel every file dialog."""
    return send_response(self, RESPONSE_USER_CANCELLED, {})


# --- GlobalShortcuts -------------------------------------------------------

def reset_global_shortcuts(self):
    """Forget all approved shortcuts and pending failure injections."""
    # id -> (description, trigger_description), in approval order.
    self.global_shortcuts = {}
    self.fail_next_list_shortcuts = 0


def shortcuts_payload(self):
    """The approved shortcuts as the a(sa{sv}) the portal returns."""
    return dbus.Array([
        dbus.Struct((dbus.String(shortcut_id), dbus.Dictionary({
            'description': dbus.String(description),
            'trigger_description': dbus.String(trigger_description),
        }, signature='sv')))
        for shortcut_id, (description, trigger_description)
        in self.global_shortcuts.items()
    ], signature='(sa{sv})')


def handle_create_session(self, options):
    """Create a Session object named after the client's token."""
    token = str(options.get('session_handle_token', 'session'))
    session_path = f'{MAIN_OBJ}/session/mock/{token}'
    # Clients may close and re-create a session with the same token.
    if session_path in mockobject.objects:
        self.RemoveObject(session_path)
    self.AddObject(session_path, SESSION_IFACE, {}, [
        ('Close', '', '',
         f'self.remove_from_connection(); del objects["{session_path}"]'),
    ])
    return send_response(self, RESPONSE_SUCCESS,
                         {'session_handle': dbus.String(session_path)})


def handle_bind_shortcuts(self, _session, shortcuts, _parent_window, _options):
    """Approve every requested shortcut, keeping earlier approvals."""
    for shortcut_id, properties in shortcuts:
        shortcut_id = str(shortcut_id)
        description = str(properties.get('description', ''))
        trigger = str(properties.get('preferred_trigger', ''))
        if shortcut_id in self.global_shortcuts:
            # Re-binding an approved shortcut keeps the compositor's trigger.
            _, existing_trigger = self.global_shortcuts[shortcut_id]
            trigger = trigger or existing_trigger
        self.global_shortcuts[shortcut_id] = (description, trigger)
    return send_response(self, RESPONSE_SUCCESS,
                         {'shortcuts': shortcuts_payload(self)})


def handle_list_shortcuts(self, session, _options):
    """Return the approved shortcuts, or an injected failure."""
    if self.fail_next_list_shortcuts > 0:
        self.fail_next_list_shortcuts -= 1
        return send_response(self, RESPONSE_OTHER, {})
    if str(session) not in mockobject.objects:
        return send_response(self, RESPONSE_OTHER, {})
    return send_response(self, RESPONSE_SUCCESS,
                         {'shortcuts': shortcuts_payload(self)})


def handle_set_shortcuts(self, shortcuts):
    """Replace the approved shortcuts with [(id, description, trigger)]."""
    self.global_shortcuts = {
        str(shortcut_id): (str(description), str(trigger))
        for shortcut_id, description, trigger in shortcuts
    }


def handle_get_shortcuts(self):
    """Return the approved shortcuts as [(id, description, trigger)]."""
    return dbus.Array([
        (shortcut_id, description, trigger)
        for shortcut_id, (description, trigger)
        in self.global_shortcuts.items()
    ], signature='(sss)')


def handle_fail_next_list_shortcuts(self, count):
    """Answer the next `count` ListShortcuts requests with an error."""
    self.fail_next_list_shortcuts = int(count)
