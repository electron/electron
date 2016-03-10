(function () {
  return function(process, require, asarSource) {
    // Make asar.coffee accessible via "require".
    process.binding('natives').ATOM_SHELL_ASAR = asarSource;

    // Monkey-patch the fs module.
    require('ATOM_SHELL_ASAR').wrapFsWithAsar(require('fs'));

    // Make graceful-fs work with asar.
    var source = process.binding('natives');
    source['original-fs'] = source.fs;
    return source['fs'] = "var src = '(function (exports, require, module, __filename, __dirname) { ' +\n          process.binding('natives')['original-fs'] +\n          ' });';\nvar vm = require('vm');\nvar fn = vm.runInThisContext(src, { filename: 'fs.js' });\nfn(exports, require, module);\nvar asar = require('ATOM_SHELL_ASAR');\nasar.wrapFsWithAsar(exports);";
  };
})();
