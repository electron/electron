import sys

in_start = sys.argv.index("--in") + 1
out_start = sys.argv.index("--out") + 1

in_bundles = sys.argv[in_start:out_start - 1]
out_bundles = sys.argv[out_start:]

if len(in_bundles) is not len(out_bundles):
  print("--out and --in must provide the same number of arguments")
  sys.exit(1)

for i in range(len(in_bundles)):
  in_bundle = in_bundles[i]
  out_path = out_bundles[i]
  with open(in_bundle, 'r') as f:
    lines = ["(function(){var exports={},module={exports};"] + f.readlines() + ["})();"]
    with open(out_path, 'w') as out_f:
      out_f.writelines(lines)
