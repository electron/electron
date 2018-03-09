from config import is_verbose_mode
from dbusmock import DBusTestCase

import atexit
import os
import sys


def cleanup():
    DBusTestCase.stop_dbus(DBusTestCase.system_bus_pid)
    DBusTestCase.stop_dbus(DBusTestCase.session_bus_pid)


atexit.register(cleanup)

dbusmock_log = sys.stdout if is_verbose_mode() else open(os.devnull, 'w')

DBusTestCase.start_system_bus()
DBusTestCase.spawn_server_template('logind', None, dbusmock_log)

DBusTestCase.start_session_bus()
DBusTestCase.spawn_server_template('notification_daemon', None, dbusmock_log)
