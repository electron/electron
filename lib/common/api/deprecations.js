const deprecate = require('electron').deprecate

exports.setHandler = function (deprecationHandler) {
  deprecate.setHandler(deprecationHandler)
}

exports.getHandler = function () {
  return deprecate.getHandler()
}
