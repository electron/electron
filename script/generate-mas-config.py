import sys

def main(is_mas_build, out_file):
  is_mas_num = 0
  if is_mas_build:
    is_mas_num = 1
  with open(out_file, 'w', encoding="utf-8") as f:
    content = ''
    content += '#ifndef ELECTRON_GEN_MAS_BUILD_H_\n'
    content += '#define ELECTRON_GEN_MAS_BUILD_H_\n'
    content += '#define IS_MAS_BUILD() ' + str(is_mas_num) + '\n'
    content += '#endif\n'

    f.write(content)

if __name__ == '__main__':
  sys.exit(main(sys.argv[1] == "true", sys.argv[2]))
