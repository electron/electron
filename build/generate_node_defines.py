import os
import re
import sys

DEFINE_EXTRACT_REGEX = re.compile('^ *# *define (\w*)', re.MULTILINE)

def main(out_dir, headers):
  defines = []
  for filename in headers:
    with open(filename, 'r') as f:
      content = f.read()
      defines += read_defines(content)

  push_and_undef = ''
  for define in defines:
    push_and_undef += '#pragma push_macro("%s")\n' % define
    push_and_undef += '#undef %s\n' % define
  with open(os.path.join(out_dir, 'push_and_undef_node_defines.h'), 'w') as o:
    o.write(push_and_undef)

  pop = ''
  for define in defines:
    pop += '#pragma pop_macro("%s")\n' % define
  with open(os.path.join(out_dir, 'pop_node_defines.h'), 'w') as o:
    o.write(pop)

def read_defines(content):
  defines = []
  for match in DEFINE_EXTRACT_REGEX.finditer(content):
    defines.append(match.group(1))
  return defines

if __name__ == '__main__':
  main(sys.argv[1], sys.argv[2:])
