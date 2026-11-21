"""wptrunner product "electron".

Drives an Electron binary through Electron's own ChromeDriver, with a small
shell app (--electron-app=<dir>, passed to Electron as --app=<dir>).
Everything is reused from wptrunner's built-in `chrome` product except:

  * --app=<shell dir> is added and --headless is never passed (Electron has
  no headless mode); --electron-xvfb gives every browser instance its own
  X server instead.
  * Experimental web platform features are off unless asked for, either with
  --enable-experimental or by passing the switches through --binary-arg.
  The chrome product turns them on when nothing is said.
  * Reftests do not resize the window unless --electron-reftest-resize is
  given; the shell app makes the content area 800x600 itself.
  * run_info gets the Electron and Chromium versions, so reports say what
  was tested.

It is registered through the `wptrunner.products` entry point (see
pyproject.toml), so `wpt run electron ...` works without changes to wpt.
"""

import json
import os
import subprocess

from wptrunner.browsers import chrome as chrome_product
from wptrunner.browsers.base import get_timeout_multiplier, require_arg
from wptrunner.browsers.chrome import ChromeBrowser
from wptrunner.executors.base import PytestExecutor
from wptrunner.executors.executorchrome import (
    ChromeDriverCrashTestExecutor,
    ChromeDriverPrintRefTestExecutor,
    ChromeDriverRefTestExecutor,
    ChromeDriverTestharnessExecutor,
)
from wptrunner.products import Product

EXPERIMENTAL_FLAGS = ("--enable-experimental-web-platform-features",
                      "--enable-blink-test-features")


def add_arguments(parser):
  group = parser.add_argument_group("Electron-specific")
  group.add_argument(
      "--electron-app",
      dest="electron_app",
      default=None,
      help="Path to the Electron WPT shell app directory (passed as --app)"
  )
  group.add_argument(
      "--electron-explicit-webdriver-port",
      dest="electron_explicit_webdriver_port",
      action="store_true",
      default=False,
      help="Do not pass --ignore-explicit-port to chromedriver. Needed on hosts "
      "without IPv6, where chromedriver then reports port 0.")
  group.add_argument(
      "--electron-xvfb",
      dest="electron_xvfb",
      action="store_true",
      default=False,
      help=
      "Start a private Xvfb for every browser instance. Windows sharing one X "
      "display take focus, fullscreen and the clipboard from each other.")
  group.add_argument(
      "--electron-reftest-resize",
      dest="electron_reftest_resize",
      action="store_true",
      default=False,
      help=
      "Resize the window for reftests through WebDriver like the chrome product "
      "does. Needs CDP Browser.getWindowForTarget and Browser.setWindowBounds "
      "(https://github.com/electron/electron/pull/55945).")
  group.add_argument(
      "--electron-strip-feature-params",
      dest="electron_strip_feature_params",
      action="store_true",
      default=False,
      help=
      "Drop ':param/value' from --enable-features entries. Builds with DCHECKs "
      "abort on them (https://github.com/electron/electron/issues/55990).")


def check_args(**kwargs):
  require_arg(kwargs, "webdriver_binary")
  require_arg(kwargs, "binary")
  require_arg(kwargs, "electron_app")
  if not os.path.isfile(os.path.join(kwargs["electron_app"], "package.json")):
    raise ValueError("--electron-app must be a directory with a package.json")


def browser_kwargs(logger, test_type, run_info_data, config, subsuite,
                   **kwargs):  # pylint: disable=unused-argument
  rv = chrome_product.browser_kwargs(logger, test_type, run_info_data, config,
                                     **kwargs)
  rv["explicit_port"] = kwargs.get("electron_explicit_webdriver_port", False)
  rv["xvfb"] = kwargs.get("electron_xvfb", False)
  return rv


def executor_kwargs(logger, test_type, test_environment, run_info_data,
                    subsuite, **kwargs):
  kwargs = dict(kwargs)
  explicit_experimental = kwargs.get("enable_experimental")
  if explicit_experimental is None:
    kwargs["enable_experimental"] = False
  kwargs["headless"] = False
  rv = chrome_product.executor_kwargs(logger, test_type, test_environment,
                                      run_info_data, subsuite, **kwargs)
  requested = kwargs.get("binary_args") or []
  opts = rv["capabilities"]["goog:chromeOptions"]
  args = [
      a for a in opts["args"]
      if not a.startswith("--headless") and not a.startswith("--app=")
  ]
  if not explicit_experimental:
    args = [a for a in args if a not in EXPERIMENTAL_FLAGS or a in requested]
  if kwargs.get("electron_strip_feature_params"):
    args = [_strip_feature_params(a) for a in args]
  args.insert(0, f"--app={os.path.abspath(kwargs['electron_app'])}")
  opts["args"] = args
  if "binary_args" in rv:
    rv["binary_args"] = args
  if test_type == "reftest":
    rv["electron_reftest_resize"] = kwargs.get("electron_reftest_resize", False)
  return rv


def _strip_feature_params(arg):
  if not arg.startswith("--enable-features="):
    return arg
  features = arg.split("=", 1)[1].split(",")
  return f"--enable-features={','.join((f.split(':', 1)[0] for f in features))}"


def env_extras(**kwargs):  # pylint: disable=unused-argument
  return []


def run_info_extras(logger, **kwargs):
  binary = kwargs.get("binary")
  if not binary:
    return {}
  env = dict(os.environ, ELECTRON_RUN_AS_NODE="1")
  env.pop("NODE_OPTIONS", None)
  try:
    out = subprocess.check_output([
        binary, "-p",
        "JSON.stringify({e: process.versions.electron, c: process.versions.chrome})"
    ],
                                  env=env,
                                  timeout=60)
    versions = json.loads(out)
    return {"electron_version": versions["e"], "browser_version": versions["c"]}
  except Exception as e:  # pylint: disable=broad-except
    logger.warning(f"Could not get the Electron version: {e}")
    return {}


class ElectronBrowser(ChromeBrowser):

  def __init__(self, logger, explicit_port=False, xvfb=False, **kwargs):
    super().__init__(logger, **kwargs)
    self._explicit_port = explicit_port
    self._use_xvfb = xvfb
    self._xvfb = None

  def start(self, group_metadata, **kwargs):
    if self._use_xvfb and (self._xvfb is None or self._xvfb.poll() is not None):
      read_fd, write_fd = os.pipe()
      self._xvfb = subprocess.Popen(  # pylint: disable=consider-using-with
          [
              "Xvfb", "-displayfd",
              str(write_fd), "-screen", "0", "1280x1024x24", "-nolisten", "tcp",
              "-noreset"
          ],
          pass_fds=[write_fd],
          stdout=subprocess.DEVNULL,
          stderr=subprocess.DEVNULL)
      os.close(write_fd)
      with os.fdopen(read_fd) as f:
        display = f.readline().strip()
      if not display:
        raise RuntimeError("Xvfb did not start")
      # WebDriverBrowser.__init__ sets self.env.
      # pylint: disable-next=attribute-defined-outside-init
      self.env = dict(self.env, DISPLAY=f":{display}")
      self.logger.info(f"Electron: private Xvfb on DISPLAY=:{display}")
    super().start(group_metadata, **kwargs)

  def cleanup(self):
    try:
      super().cleanup()
    finally:
      if self._xvfb is not None and self._xvfb.poll() is None:
        self._xvfb.terminate()
        self._xvfb = None

  def make_command(self):
    cmd = super().make_command()
    if self._explicit_port:
      cmd = [a for a in cmd if a != "--ignore-explicit-port"]
    return cmd


class ElectronRefTestExecutor(ChromeDriverRefTestExecutor):
  """Chrome's reftest executor without the window resize.

  ChromeDriver resizes through CDP Browser.setWindowBounds, which Electron
  does not implement, so the shell app sizes the content area to 800x600.

  TODO: delete this class and the shell app's fixed size once
  https://github.com/electron/electron/pull/55945 has landed on every
  branch this runs against.
  """

  def __init__(self, *args, electron_reftest_resize=False, **kwargs):
    super().__init__(*args, **kwargs)
    self._resize = electron_reftest_resize

  def do_test(self, test):
    if self._resize:
      return super().do_test(test)
    size = self.protocol.webdriver.execute_script(
        "return [window.innerWidth, window.innerHeight];")
    if list(size) != [800, 600]:
      self.logger.warning(
          f"Electron viewport is {size[0]}x{size[1]}, expected 800x600")
    result = self.implementation.run_test(test)
    if (leak_part := getattr(self.protocol, "leak",
                             None)) and (counters := leak_part.check()):
      result.setdefault("extra", {})["leak_counters"] = counters
    if self.debug_test and result["status"] in ["PASS", "FAIL", "ERROR"
                                                ] and "extra" in result:
      self.protocol.debug.load_reftest_analyzer(test, result)
    file_result, subtest_results = self.convert_result(test, result)
    if self.enable_tracing:
      file_result.extra["trace"] = self.protocol.tracing.get_trace()
    return file_result, subtest_results


def get_product():
  return Product(
      name="electron",
      browser_classes={None: ElectronBrowser},
      check_args=check_args,
      get_browser_kwargs=browser_kwargs,
      get_executor_kwargs=executor_kwargs,
      env_options=chrome_product.env_options(),
      get_env_extras=env_extras,
      get_timeout_multiplier=get_timeout_multiplier,
      executor_classes={
          "testharness": ChromeDriverTestharnessExecutor,
          "reftest": ElectronRefTestExecutor,
          "print-reftest": ChromeDriverPrintRefTestExecutor,
          "crashtest": ChromeDriverCrashTestExecutor,
          "wdspec": PytestExecutor,
          "test262": ChromeDriverTestharnessExecutor,
      },
      run_info_extras=run_info_extras,
      update_properties=chrome_product.update_properties(),
      add_arguments=add_arguments,
  )
