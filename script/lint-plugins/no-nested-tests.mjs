// Flags it()/ifit()/itremote() calls that appear inside the body of another
// it()/ifit()/itremote() call. Mocha tolerated this; vitest does not.

const TEST_CALLEES = new Set(['it', 'itremote', 'test', 'specify']);

function isTestCall(node) {
  if (node.type !== 'CallExpression') return null;
  let callee = node.callee;
  // unwrap it.only / it.skip / it.runIf(cond)(...)
  if (callee.type === 'MemberExpression' && callee.object.type === 'Identifier') {
    callee = callee.object;
  }
  // ifit(cond)(name, fn) — callee is CallExpression whose callee is Identifier 'ifit'
  if (callee.type === 'CallExpression') {
    const inner = callee.callee;
    if (inner.type === 'Identifier' && (inner.name === 'ifit' || inner.name === 'it')) {
      return { name: inner.name, fn: node.arguments.find((a) => isFunctionLike(a)) };
    }
    if (
      inner.type === 'MemberExpression' &&
      inner.object.type === 'Identifier' &&
      (inner.object.name === 'it' || inner.object.name === 'test')
    ) {
      return { name: inner.object.name, fn: node.arguments.find((a) => isFunctionLike(a)) };
    }
    return null;
  }
  if (callee.type === 'Identifier' && TEST_CALLEES.has(callee.name)) {
    return { name: callee.name, fn: node.arguments.find((a) => isFunctionLike(a)) };
  }
  return null;
}

function isFunctionLike(node) {
  return (
    node &&
    (node.type === 'FunctionExpression' ||
      node.type === 'ArrowFunctionExpression' ||
      (node.type === 'CallExpression' && node.callee.type === 'Identifier' && node.callee.name === 'withDone'))
  );
}

export default {
  meta: { name: 'no-nested-tests' },
  rules: {
    'no-nested-tests': {
      meta: { type: 'problem' },
      create(context) {
        let depth = 0;
        return {
          CallExpression(node) {
            const test = isTestCall(node);
            if (!test) return;
            if (depth > 0) {
              context.report({
                node: node.callee,
                message: `'${test.name}()' is nested inside another test body. vitest does not allow nested test definitions; hoist this to the enclosing describe().`
              });
            }
            depth++;
          },
          'CallExpression:exit'(node) {
            if (isTestCall(node)) depth--;
          }
        };
      }
    }
  }
};
