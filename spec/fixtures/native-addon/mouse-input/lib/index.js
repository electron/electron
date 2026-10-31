// Real OS level mouse input for specs: SendInput() on Windows
// (src/main_win.cc), CGEventPost() on macOS (src/impl_mac.cc) and XTEST on
// Linux X11 (src/impl_linux.cc). Coordinates are what the platform's input
// APIs use: physical screen pixels on Windows and X11 (convert DIPs with
// screen.dipToScreenPoint() first), points (DIPs) on macOS.
const binding = require('../build/Release/mouse_input.node');

const check = (what) => (result) => {
  if (result.sent !== result.expected) {
    throw new Error(`mouse-input ${what}: injected ${result.sent}/${result.expected} events, error ${result.error}`);
  }
  return result;
};

module.exports = {
  move: (x, y) => binding.move(Math.round(x), Math.round(y)).then(check('move')),
  down: (button = 'left') => binding.button(button, 'down').then(check('down')),
  up: (button = 'left') => binding.button(button, 'up').then(check('up')),
  click: (button = 'left') => binding.button(button, 'click').then(check('click')),
  wheel: (delta, horizontal = false) => binding.wheel(delta, horizontal).then(check('wheel')),
  getCursorPos: binding.getCursorPos,
  // macOS only: a left click at screen point (x, y) posted straight to window
  // |windowNumber| of process |pid| with CGEventPostToPid().
  postClickToWindow: (pid, windowNumber, x, y) =>
    check('postClickToWindow')(binding.postClickToWindow(pid, windowNumber, Math.round(x), Math.round(y))),
  isWindowAtPoint: binding.isWindowAtPoint,
  // Windows only; elsewhere returns null.
  describeWindowAtPoint: binding.describeWindowAtPoint ?? (() => null),
  getDiagnostics: binding.getDiagnostics
};
