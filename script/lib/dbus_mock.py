from dbusmock import DBusTestCase

import atexit

def cleanup():
    DBusTestCase.stop_dbus(DBusTestCase.system_bus_pid)


atexit.register(cleanup)
DBusTestCase.start_system_bus()
# create a mock for "org.freedesktop.login1" using python-dbusmock
# preconfigured template
(logind_mock, logind) = DBusTestCase.spawn_server_template('logind')
