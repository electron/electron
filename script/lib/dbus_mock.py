from dbusmock import DBusTestCase

import atexit

def cleanup():
    DBusTestCase.stop_dbus(DBusTestCase.system_bus_pid)
    DBusTestCase.stop_dbus(DBusTestCase.session_bus_pid)


atexit.register(cleanup)

DBusTestCase.start_system_bus()
(logind_mock, logind) = DBusTestCase.spawn_server_template('logind')

DBusTestCase.start_session_bus()
(notify_mock, notify) = DBusTestCase.spawn_server_template('notification_daemon')
