(function () {
  if (location.protocol === 'file:') {
    window.test = 'preload'
  }
})()
