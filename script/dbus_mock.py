#!/usr/bin/env python3

import os
import subprocess
import sys

import dbus
from dbusmock import DBusTestCase

from lib.config import is_verbose_mode


PORTAL_SERVICE = 'org.freedesktop.portal.Desktop'
PORTAL_PATH = '/org/freedesktop/portal/desktop'
BACKGROUND_INTERFACE = 'org.freedesktop.portal.Background'
REGISTRY_INTERFACE = 'org.freedesktop.host.portal.Registry'
MOCK_INTERFACE = 'org.freedesktop.DBus.Mock'


REQUEST_BACKGROUND_CODE = '''
import dbus
import threading

options = args[1]
autostart = bool(options.get('autostart', False))
handle_token = str(options.get('handle_token', 'electron_login_item'))
request_path = '/org/freedesktop/portal/desktop/request/mock/' + handle_token

def respond(mock=self, path=request_path, enabled=autostart, dbus_module=dbus):
    mock._emit_signal(
        'org.freedesktop.portal.Request',
        'Response',
        'ua{sv}',
        [
            dbus_module.UInt32(0),
            {
                'background': dbus_module.Boolean(True, variant_level=1),
                'autostart': dbus_module.Boolean(enabled, variant_level=1),
            },
        ],
        {'path': path},
    )

threading.Timer(0.05, respond).start()
ret = dbus.ObjectPath(request_path)
'''


def stop():
    if hasattr(DBusTestCase, 'stop_dbus'):
        if DBusTestCase.system_bus_pid is not None:
            DBusTestCase.stop_dbus(DBusTestCase.system_bus_pid)
        if DBusTestCase.session_bus_pid is not None:
            DBusTestCase.stop_dbus(DBusTestCase.session_bus_pid)
    else:
        DBusTestCase.tearDownClass()


def start():
    with sys.stdout if is_verbose_mode() \
            else open(os.devnull, 'w', encoding='utf-8') as log:
        DBusTestCase.start_system_bus()
        DBusTestCase.spawn_server_template('logind', None, log)

        DBusTestCase.start_session_bus()
        DBusTestCase.spawn_server_template('notification_daemon', None, log)
        DBusTestCase.spawn_server_template(
            os.path.join(os.path.dirname(os.path.abspath(__file__)),
                         'dbusmock_xdg_file_chooser_portal.py'), None, log)
        DBusTestCase.wait_for_bus_object(PORTAL_SERVICE, PORTAL_PATH)

        portal = DBusTestCase.get_dbus().get_object(PORTAL_SERVICE, PORTAL_PATH)
        mock = dbus.Interface(portal, MOCK_INTERFACE)
        mock.AddMethod(REGISTRY_INTERFACE, 'Register', 'sa{sv}', '', '')
        mock.AddMethod(BACKGROUND_INTERFACE, 'RequestBackground', 'sa{sv}', 'o',
                       REQUEST_BACKGROUND_CODE)


if __name__ == '__main__':
    start()
    try:
        subprocess.check_call(sys.argv[1:])
    finally:
        stop()
