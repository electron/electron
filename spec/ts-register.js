// Registers a require() hook so the spec runner can load .ts files directly.
// Each file is transpiled on its own by the TypeScript compiler API using
// tsconfig.spec.json; type checking happens separately via `tsc`.

const ts = require('typescript');

const fs = require('node:fs');
const path = require('node:path');
const { pathToFileURL } = require('node:url');

const tsconfigPath = path.resolve(__dirname, '..', 'tsconfig.spec.json');
const formatHost = {
  getCanonicalFileName: (fileName) => fileName,
  getCurrentDirectory: ts.sys.getCurrentDirectory,
  getNewLine: () => ts.sys.newLine
};
const onConfigError = (diagnostic) => {
  throw new Error(ts.formatDiagnostic(diagnostic, formatHost));
};
const parsed = ts.getParsedCommandLineOfConfigFile(
  tsconfigPath,
  {},
  { ...ts.sys, onUnRecoverableConfigFileDiagnostic: onConfigError }
);
if (parsed.errors.length > 0) onConfigError(parsed.errors[0]);

// tsconfig.spec.json targets `nodenext`, which emits CommonJS for these files
// (spec/package.json has no "type": "module") while leaving dynamic import()
// intact and enabling esModuleInterop, matching how Node itself treats them.
const compilerOptions = {
  ...parsed.options,
  module: ts.ModuleKind.NodeNext,
  noEmit: false,
  sourceMap: true,
  inlineSourceMap: false,
  inlineSources: true
};

process.setSourceMapsEnabled(true);

require.extensions['.ts'] = (module, filename) => {
  const source = fs.readFileSync(filename, 'utf8');
  const { outputText, sourceMapText } = ts.transpileModule(source, {
    compilerOptions,
    fileName: filename,
    reportDiagnostics: false
  });
  // Point the source map back at the absolute path of the original file so
  // stack traces resolve to it, then inline the map for Node to pick up.
  const sourceMap = JSON.parse(sourceMapText);
  sourceMap.sources = [pathToFileURL(filename).href];
  delete sourceMap.sourceRoot;
  const inlineMap = Buffer.from(JSON.stringify(sourceMap), 'utf8').toString('base64');
  const code = outputText.replace(
    /\/\/# sourceMappingURL=.*$/,
    `//# sourceMappingURL=data:application/json;charset=utf-8;base64,${inlineMap}`
  );
  module._compile(code, filename);
};
