(function () {
  window.require = require;
  if (location.protocol === 'file:') {
    window.test = 'preload';
  }
})();
