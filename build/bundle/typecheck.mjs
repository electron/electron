// Type checks a TypeScript project without emitting, for the code that
// bundle.mjs builds (rolldown only strips types, it does not check them).
//
//   node build/bundle/typecheck.mjs --project tsconfig.electron.json [--stamp file]

import ts from 'typescript';

import * as fs from 'node:fs';
import * as path from 'node:path';
import { parseArgs } from 'node:util';

const { values: args } = parseArgs({
  options: {
    project: { type: 'string' },
    stamp: { type: 'string' }
  },
  strict: true
});

if (!args.project) {
  throw new Error('--project is required');
}

// Diagnostics that come with pulling type information out of
// ../third_party/electron_node/lib/*.js via the `@node/*` path mapping.
const ignoredDiagnostics = new Set([
  // File '{0}' is not under 'rootDir' '{1}'.
  6059,
  // Private field '{0}' must be declared in an enclosing class.
  1111
]);

const formatHost = {
  getCanonicalFileName: (fileName) => fileName,
  getCurrentDirectory: ts.sys.getCurrentDirectory,
  getNewLine: () => ts.sys.newLine
};

const reportAndExit = (diagnostics) => {
  console.error(ts.formatDiagnosticsWithColorAndContext(diagnostics, formatHost));
  process.exit(1);
};

const configFile = path.resolve(args.project);
const config = ts.getParsedCommandLineOfConfigFile(
  configFile,
  { noEmit: true },
  { ...ts.sys, onUnRecoverableConfigFileDiagnostic: (diagnostic) => reportAndExit([diagnostic]) }
);
if (config.errors.length) reportAndExit(config.errors);

const program = ts.createProgram({
  rootNames: config.fileNames,
  options: config.options,
  projectReferences: config.projectReferences
});
const diagnostics = ts.getPreEmitDiagnostics(program).filter((diagnostic) => !ignoredDiagnostics.has(diagnostic.code));
if (diagnostics.length) reportAndExit(diagnostics);

// An existing stamp is left untouched: its dependents only need to rebuild when
// their own sources change, and ninja (GN actions are restat) can then tell.
if (args.stamp && !fs.existsSync(args.stamp)) {
  fs.mkdirSync(path.dirname(args.stamp), { recursive: true });
  fs.writeFileSync(args.stamp, '');
}
