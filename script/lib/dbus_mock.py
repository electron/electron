from dbusmock import DBusTestCase

import atexit
import os


def cleanup():
    DBusTestCase.stop_dbus(DBusTestCase.system_bus_pid)
    DBusTestCase.stop_dbus(DBusTestCase.session_bus_pid)


atexit.register(cleanup)

DBusTestCase.start_system_bus()
print('dbusmock system bus: ' + os.environ['DBUS_SYSTEM_BUS_ADDRESS'])
DBusTestCase.spawn_server_template('logind')

DBusTestCase.start_session_bus()
print('dbusmock session bus: ' + os.environ['DBUS_SESSION_BUS_ADDRESS'])
DBusTestCase.spawn_server_template('notification_daemon')
