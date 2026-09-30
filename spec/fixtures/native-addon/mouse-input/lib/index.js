// Real (SendInput based) mouse input for specs. Windows only, see
// src/main.cc. Coordinates are physical screen pixels: convert DIPs with
// screen.dipToScreenPoint() first.
const binding = require('../build/Release/mouse_input.node');

const check = (what) => (result) => {
  if (result.sent !== result.expected) {
    throw new Error(`SendInput(${what}) injected ${result.sent}/${result.expected} events, GetLastError() = ${result.error}`);
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
  isWindowAtPoint: binding.isWindowAtPoint,
  getDiagnostics: binding.getDiagnostics
};
