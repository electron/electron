#!/usr/bin/env python3
"""Serve captured job JSON for a test runner on another machine (e.g. Windows)."""

import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import re
from urllib.parse import unquote, urlsplit

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("directory", type=Path)
parser.add_argument("--host", default="127.0.0.1")
parser.add_argument("--port", type=int, default=8633)
args = parser.parse_args()
directory = args.directory.resolve(strict=True)
filename_pattern = re.compile(r"[\w.-]+\.json")


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        route = unquote(urlsplit(self.path).path)
        if route == "/jobs":
            data = json.dumps(sorted(
                entry.name for entry in directory.iterdir()
                if filename_pattern.fullmatch(entry.name) and entry.is_file()
            )).encode("utf8")
        elif route.startswith("/jobs/") and filename_pattern.fullmatch(route[6:]):
            try:
                data = (directory / route[6:]).read_bytes()
            except FileNotFoundError:
                self.send_error(404)
                return
        else:
            self.send_error(404)
            return
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)


with ThreadingHTTPServer((args.host, args.port), Handler) as server:
    server.serve_forever()
