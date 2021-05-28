import sys
import os

def main():
  output_path = os.path.join(sys.argv[2], 'arch')
  arch_file = open(output_path, "w")
  arch_file.write(sys.argv[1])
  arch_file.close()

if __name__ == '__main__':
  sys.exit(main())
