from dbusmock import DBusTestCase

import atexit


def cleanup():
    DBusTestCase.stop_dbus(DBusTestCase.system_bus_pid)
    DBusTestCase.stop_dbus(DBusTestCase.session_bus_pid)


atexit.register(cleanup)

DBusTestCase.start_system_bus()
DBusTestCase.spawn_server_template('logind')

DBusTestCase.start_session_bus()
DBusTestCase.spawn_server_template('notification_daemon')
