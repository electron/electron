import { createRequire } from 'node:module';

// The names Node.js itself adds to the namespace of a CommonJS module that is
// import-ed, which have no counterpart on the require() side.
const SYNTHETIC_EXPORTS = new Set(['default', 'module.exports']);

// Loads `id` through both `import()` and `require()` from the calling module
// and returns a description of every way the two disagree: an export that only
// one of them has, an export that is not the very same value on both, or a
// default import that is not the require() exports object.
export async function compareModule(id, parentURL) {
  const require = createRequire(parentURL);
  const esm = await import(id);
  const cjs = require(id);

  const problems = [];
  const esmKeys = Object.keys(esm).filter((key) => !SYNTHETIC_EXPORTS.has(key));
  const cjsKeys = Object.keys(cjs);

  for (const key of cjsKeys) {
    if (!esmKeys.includes(key)) {
      problems.push(`${id}: '${key}' is available from require() but not from import`);
    } else if (esm[key] !== cjs[key]) {
      problems.push(`${id}: '${key}' is not the same value from import and require()`);
    }
  }
  for (const key of esmKeys) {
    if (!cjsKeys.includes(key)) {
      problems.push(`${id}: '${key}' is available from import but not from require()`);
    }
  }
  if (esm.default !== cjs) {
    problems.push(`${id}: the default import is not the require() exports object`);
  }

  return problems;
}

export async function compareModules(ids, parentURL) {
  const problems = [];
  for (const id of ids) {
    problems.push(...(await compareModule(id, parentURL)));
  }
  return problems;
}
