import sys
import os
import subprocess

def is_depot_tools_installed():
  try:
    with open(os.devnull, "w") as f:
      subprocess.check_call(["gclient", "help"], stdout=f)
    return True
  except (OSError, subprocess.CalledProcessError):
    return False

def find_gclient_root(search_dir='.'):
  search_dir = os.path.realpath(search_dir)
  test_path = os.path.join(search_dir, '.gclient')
  if os.path.exists(test_path):
    return search_dir
  else:
    parent_path = os.path.dirname(search_dir)
    if parent_path != search_dir:
      return find_gclient_root(parent_path)
    else:
      # We got to the root without finding .gclient.
      return None

def is_gclient_configured():
  return find_gclient_root() is not None

def gclient_config():
  subprocess.check_call([
    "gclient",
    "config",
    "--name",
    "src/electron",
    "--unmanaged",
    "https://github.com/electron/electron"
  ])

def gclient_sync():
  subprocess.check_call([
    "gclient",
    "sync",
    "--with_branch_heads",
    "--with_tags"
  ])

def gn_gen(gclient_root):
  args = "import(\"//electron/build/args/debug.gn\")"
  out_path = os.path.join(gclient_root, 'src', 'out', 'Debug')
  env = dict(os.environ)
  env['CHROMIUM_BUILDTOOLS_PATH'] = os.path.join(gclient_root, 'src', 'buildtools')
  subprocess.check_call([
    "gn",
    "gen",
    out_path,
    "--args={}".format(args)
  ], env=env)

def main():
  if not is_depot_tools_installed():
    die("It looks like depot_tools isn't installed, or isn't on your PATH.\n"
        "Please see the installation instructions for depot_tools:\n"
        "http://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up")
  gclient_root = find_gclient_root()
  if gclient_root is None:
    print(u" [\u2699 ] Configuring gclient repository in current directory")
    gclient_config()
  else:
    print(u" [\u2699 ] gclient already configured")
    print(u" [\u2699 ] gclient root: {}".format(gclient_root))

  print(u" [\u2615] Fetching Electron and its dependencies. This will take a while.")
  print(u" [\u2615]    (seriously, like probably several hours if this is your first time fetching Electron)")
  gclient_sync()
  print(u" [\U0001F680] Generating build configuration")
  gn_gen(gclient_root)
  print(u" [\U0001F389] Success! Electron has been checked out.")
  print(u"   To build Electron, run the following:")
  print(u"")
  print(u"     $ cd src")
  print(u"     $ ninja -C out/Debug electron:electron_app")

if __name__ == '__main__':
  main()
