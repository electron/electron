#!/usr/bin/env python3

from collections import OrderedDict
import json
import os
import sys

dir_path = os.path.dirname(os.path.realpath(__file__))

SENTINEL = "dL7pKGdnNz796PbbjQWNKmHXBZaB9tsX"

# Fuse states. Each fuse is one byte on the fuse wire.
STATE_DISABLED = "0"
STATE_ENABLED = "1"
STATE_REMOVED = "r"
# Enabled when the app is not packaged (e.g. running from the default
# "electron" executable) and disabled when it is packaged.
STATE_ENABLED_UNPACKAGED = "d"
# Disabled when the app is not packaged and enabled when it is packaged.
STATE_ENABLED_PACKAGED = "p"
VALID_STATES = (STATE_DISABLED, STATE_ENABLED, STATE_REMOVED,
                STATE_ENABLED_UNPACKAGED, STATE_ENABLED_PACKAGED)

TEMPLATE_H = """
#ifndef ELECTRON_FUSES_H_
#define ELECTRON_FUSES_H_

#if defined(WIN32)
#define FUSE_EXPORT __declspec(dllexport)
#else
#define FUSE_EXPORT __attribute__((visibility("default")))
#endif

namespace electron::fuses {

extern const volatile char kFuseWire[];

{getters}

}  // namespace electron::fuses

#endif  // ELECTRON_FUSES_H_
"""

TEMPLATE_CC = """
#include "electron/fuses.h"
#include "base/command_line.h"
#include "base/dcheck_is_on.h"
#include "build/build_config.h"
#include "shell/browser/api/electron_api_app.h"

#if DCHECK_IS_ON()
#include <string>
#endif

namespace electron::fuses {

const volatile char kFuseWire[] = { /* sentinel */ {sentinel}, /* fuse_version */ {fuse_version}, /* fuse_wire_length */ {fuse_wire_length}, /* fuse_wire */ {initial_config}};

namespace {

bool IsPackagedForFuses() {
#if BUILDFLAG(IS_MAC)
  // On macOS App::IsPackaged() needs the process type, which is read from
  // the command line. Fuses that are read before the command line is
  // initialized treat the app as packaged.
  if (!base::CommandLine::InitializedForCurrentProcess())
    return true;
#endif
  return electron::api::App::IsPackaged();
}

bool IsFuseStateEnabled(char state) {
  switch (state) {
    case '{state_enabled}':
      return true;
    case '{state_enabled_unpackaged}':
      return !IsPackagedForFuses();
    case '{state_enabled_packaged}':
      return IsPackagedForFuses();
    default:
      // '{state_disabled}', '{state_removed}' and any unknown state.
      return false;
  }
}

}  // namespace

{getters}

}  // namespace electron:fuses
"""

with open(os.path.join(dir_path, "fuses.json5"), 'r') as f:
  fuse_defaults = json.loads(''.join(line for line in f.readlines() if not line.strip()[0] == "/"), object_pairs_hook=OrderedDict)

fuse_version = fuse_defaults['_version']
del fuse_defaults['_version']
del fuse_defaults['_schema']
del fuse_defaults['_comment']

if fuse_version >= pow(2, 8):
  raise Exception("Fuse version can not exceed one byte in size")

fuses = fuse_defaults.keys()

initial_config = ""
getters_h = ""
getters_cc = ""
index = len(SENTINEL) + 1
for fuse in fuses:
  index += 1
  if fuse_defaults[fuse] not in VALID_STATES:
    raise Exception(f"Fuse '{fuse}' has invalid state '{fuse_defaults[fuse]}'")
  initial_config += fuse_defaults[fuse]
  name = ''.join(word.title() for word in fuse.split('_'))
  getters_h += "FUSE_EXPORT bool Is{name}Enabled();\n".replace("{name}", name)
  getters_cc += """
bool Is{name}Enabled() {
#if DCHECK_IS_ON()
  // RunAsNode is checked so early that base::CommandLine isn't yet
  // initialized, so guard here to avoid a CHECK.
  if (base::CommandLine::InitializedForCurrentProcess()) {
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    if (command_line->HasSwitch("{switch_name}")) {
      std::string switch_value = command_line->GetSwitchValueASCII("{switch_name}");
      return switch_value.size() == 1 && IsFuseStateEnabled(switch_value[0]);
    }
  }
#endif
  return IsFuseStateEnabled(kFuseWire[{index}]);
}
""".replace("{name}", name).replace("{switch_name}", f"set-fuse-{fuse.lower()}").replace("{index}", str(index))

def c_hex(n):
  s = hex(n)[2:]
  return "0x" + s.rjust(2, '0')

def hex_arr(s):
  arr = []
  for char in s:
    arr.append(c_hex(ord(char)))
  return ",".join(arr)

header = TEMPLATE_H.replace("{getters}", getters_h.strip())
impl = TEMPLATE_CC.replace("{sentinel}", hex_arr(SENTINEL))
impl = impl.replace("{fuse_version}", c_hex(fuse_version))
impl = impl.replace("{fuse_wire_length}", c_hex(len(fuses)))
impl = impl.replace("{initial_config}", hex_arr(initial_config))
impl = impl.replace("{getters}", getters_cc.strip())
impl = impl.replace("{state_disabled}", STATE_DISABLED)
impl = impl.replace("{state_enabled}", STATE_ENABLED)
impl = impl.replace("{state_removed}", STATE_REMOVED)
impl = impl.replace("{state_enabled_unpackaged}", STATE_ENABLED_UNPACKAGED)
impl = impl.replace("{state_enabled_packaged}", STATE_ENABLED_PACKAGED)

with open(sys.argv[1], 'w') as f:
  f.write(header)

with open(sys.argv[2], 'w') as f:
  f.write(impl)
