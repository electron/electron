import sys


def main(argv):
  out_file, version = argv
  with open(out_file, 'w') as f:
    f.write(version)


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
