import json
import sys
from string import Template

inpath = sys.argv[1]
outpath = sys.argv[2]
argpaths = sys.argv[3:]

with open(inpath, 'r') as infile, open(outpath, 'w') as outfile:
  data = {}
  for argpath in argpaths:
    with open(argpath, 'r') as argfile:
      data.update(json.load(argfile))

  s =  Template(infile.read()).substitute(data)
  outfile.write(s)
