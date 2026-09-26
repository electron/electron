#!/usr/bin/env python3

from collections import OrderedDict
import json
import os
import sys

dir_path = os.path.dirname(os.path.realpath(__file__))

SENTINEL = "dL7pKGdnNz796PbbjQWNKmHXBZaB9tsX"

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
#include "base/dcheck_is_on.h"

#if DCHECK_IS_ON()
#include "base/command_line.h"
#include <string>
#endif

namespace electron::fuses {

const volatile char kFuseWire[] = { /* sentinel */ {sentinel}, /* fuse_version */ {fuse_version}, /* fuse_wire_length */ {fuse_wire_length}, /* fuse_wire */ {initial_config}};

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
      return switch_value == "1";
    }
  }
#endif
  return kFuseWire[{index}] == '1';
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

# On Windows the wire stays in the executable so existing fuse tools continue
# to patch the same file. The runtime receives its address at entry.
if len(sys.argv) == 4:
  wire_start = impl.index('const volatile char kFuseWire[]')
  wire_end = impl.index(';', wire_start) + 1
  wire_definition = impl[wire_start:wire_end]
  wire_impl = '#include "electron/fuses.h"\n\nnamespace electron::fuses {\n'
  wire_impl += wire_definition + '\n}  // namespace electron::fuses\n'
  with open(sys.argv[3], 'w') as f:
    f.write(wire_impl)
  header = header.replace('extern const volatile char kFuseWire[];',
                          'extern const volatile char kFuseWire[];\n'
                          'void SetFuseWire(const volatile char* wire);')
  impl = impl[:wire_start] + """namespace {
const volatile char* g_fuse_wire = nullptr;
}  // namespace

void SetFuseWire(const volatile char* wire) {
  g_fuse_wire = wire;
}""" + impl[wire_end:]
  impl = impl.replace('return kFuseWire[', 'return g_fuse_wire[')

with open(sys.argv[1], 'w') as f:
  f.write(header)

with open(sys.argv[2], 'w') as f:
  f.write(impl)
