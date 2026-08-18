const binding = require('../build/Release/echo.node');

module.exports = binding.Print;
module.exports.async = binding.PrintAsync;
