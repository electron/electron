#!/usr/bin/env python3
"""Run a command with a disposable printer credential on a private session bus.

Requires dbus-run-session, python3-dbus and python3-gi. Starts a private bus so
the test service never accesses the user's normal credential store.
"""

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
from urllib.parse import urlsplit

import dbus
import dbus.mainloop.glib
import dbus.service
from gi.repository import GLib

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--printer", required=True)
parser.add_argument("--report", type=Path, required=True)
parser.add_argument("command", nargs=argparse.REMAINDER)
args = parser.parse_args()
command = args.command[1:] if args.command[:1] == ["--"] else args.command
if not command:
    parser.error("a command is required")

if os.environ.get("ELECTRON_PRIVATE_PRINT_TEST_BUS") != "1":
    environment = dict(os.environ, ELECTRON_PRIVATE_PRINT_TEST_BUS="1")
    raise SystemExit(subprocess.call(
        ["dbus-run-session", "--", sys.executable, __file__, *sys.argv[1:]],
        env=environment))

dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
bus = dbus.SessionBus()
name = dbus.service.BusName("org.freedesktop.secrets", bus, do_not_queue=True)
service_path = "/org/freedesktop/secrets"
item_path = service_path + "/collection/printing/item"
session_path = service_path + "/session/printing"
report = {"searches": 0, "secretReads": 0}


def save_report():
    temporary = args.report.with_suffix(".tmp")
    temporary.write_text(json.dumps(report), encoding="utf8")
    temporary.replace(args.report)


class Service(dbus.service.Object):
    @dbus.service.method("org.freedesktop.Secret.Service", "sv", "vo")
    def OpenSession(self, algorithm, _input):
        if algorithm != "plain":
            raise dbus.exceptions.DBusException("This fixture supports plain sessions")
        return dbus.String("", variant_level=1), dbus.ObjectPath(session_path)

    @dbus.service.method("org.freedesktop.Secret.Service", "a{ss}", "aoao")
    def SearchItems(self, attributes):
        report["searches"] += 1
        save_report()
        uri = urlsplit(str(attributes.get("uri", "")))
        matches = (uri.hostname in ("localhost", "127.0.0.1") and
                   uri.path == "/printers/" + args.printer)
        return ([dbus.ObjectPath(item_path)] if matches else []), []


class Item(dbus.service.Object):
    @dbus.service.method("org.freedesktop.DBus.Properties", "s", "a{sv}")
    def GetAll(self, interface):
        if interface != "org.freedesktop.Secret.Item":
            return {}
        return {
            "Locked": dbus.Boolean(False),
            "Attributes": dbus.Dictionary({"user": "electron-print-fixture"}, signature="ss"),
        }

    @dbus.service.method("org.freedesktop.Secret.Item", "o", "(oayays)")
    def GetSecret(self, session):
        if session != session_path:
            raise dbus.exceptions.DBusException("Unknown fixture session")
        report["secretReads"] += 1
        save_report()
        return (dbus.ObjectPath(session_path), dbus.ByteArray(b""),
                dbus.ByteArray(b"fixture-only"), "text/plain")


service = Service(bus, service_path)
item = Item(bus, item_path)
save_report()
child = subprocess.Popen(command)
loop = GLib.MainLoop()


def check_child():
    if child.poll() is None:
        return True
    loop.quit()
    return False


GLib.timeout_add(100, check_child)
try:
    loop.run()
finally:
    if child.poll() is None:
        child.terminate()
        child.wait(timeout=10)
    save_report()
raise SystemExit(child.returncode)
