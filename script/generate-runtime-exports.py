#!/usr/bin/env python3

import mmap
from pathlib import Path
import re
import struct
import sys


def read_direct_exports(contents):
  # Accept only bare export names to avoid misinterpreting other DEF syntax.
  lines = [line.split(';', 1)[0].strip() for line in contents.splitlines()]
  lines = [line for line in lines if line]
  if (len(lines) < 3 or not re.fullmatch(r'NAME\s+[A-Za-z0-9_.-]+', lines[0])
      or lines[1] != 'EXPORTS'):
    raise ValueError('Expected NAME followed by a nonempty EXPORTS section')
  exports = set()
  for name in lines[2:]:
    if (not re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*', name)
        or name in {'NAME', 'EXPORTS'} or name in exports):
      raise ValueError(f'Unsupported or duplicate direct export: {name}')
    exports.add(name)
  return exports


def read_exports(data):
  if data[:2] != b'MZ':
    raise ValueError('Not a PE file')
  pe = struct.unpack_from('<I', data, 0x3c)[0]
  if data[pe:pe + 4] != b'PE\0\0':
    raise ValueError('Invalid PE signature')
  section_count = struct.unpack_from('<H', data, pe + 6)[0]
  optional_size = struct.unpack_from('<H', data, pe + 20)[0]
  optional = pe + 24
  magic = struct.unpack_from('<H', data, optional)[0]
  directories = {0x10b: 96, 0x20b: 112}
  if magic not in directories:
    raise ValueError('Unsupported PE optional header')
  exports_rva = struct.unpack_from('<I', data, optional + directories[magic])[0]
  if not exports_rva:
    raise ValueError('Runtime has no exports')

  sections = []
  for index in range(section_count):
    section = optional + optional_size + index * 40
    rva, size, offset = struct.unpack_from('<III', data, section + 12)
    sections.append((rva, size, offset))

  def offset_for(rva, size=1):
    for start, length, offset in sections:
      if start <= rva and rva + size <= start + length:
        result = offset + rva - start
        if result + size <= len(data):
          return result
    raise ValueError('Export data is outside the PE sections')

  exports = offset_for(exports_rva, 40)
  name_count = struct.unpack_from('<I', data, exports + 24)[0]
  names_rva = struct.unpack_from('<I', data, exports + 32)[0]
  names = offset_for(names_rva, name_count * 4)
  result = []
  for index in range(name_count):
    name_rva = struct.unpack_from('<I', data, names + index * 4)[0]
    start = offset_for(name_rva)
    end = data.find(b'\0', start)
    if end < 0:
      raise ValueError('Unterminated export name')
    offset_for(name_rva, end - start + 1)
    name = data[start:end].decode('ascii')
    if not name or any(char.isspace() or char in '"=' for char in name):
      raise ValueError('Invalid export name')
    result.append(name)
  if not result:
    raise ValueError('Runtime has no named exports')
  return sorted(result)


def main():
  runtime, base_def, output = map(Path, sys.argv[1:])
  with runtime.open('rb') as file:
    with mmap.mmap(file.fileno(), 0, access=mmap.ACCESS_READ) as data:
      exports = read_exports(data)
  contents = base_def.read_text(encoding='utf-8').rstrip() + '\n'
  direct_exports = read_direct_exports(contents)
  for name in exports:
    if name not in direct_exports:
      contents += f'  "{name}"="{runtime.stem}.{name}"\n'
  output.write_text(contents, encoding='utf-8')


if __name__ == '__main__':
  main()
