// Flags loadURL()/loadFile() calls whose returned promise is neither awaited,
// returned, .then()/.catch()'d, nor assigned. These reject as unhandled when
// the load is aborted (e.g. the test moves on or the window closes).

const LOAD_METHODS = new Set(['loadURL', 'loadFile']);

function isHandled(node, parent) {
  if (!parent) return false;
  switch (parent.type) {
    case 'AwaitExpression':
      return true;
    case 'ReturnStatement':
      return true;
    case 'ArrowFunctionExpression':
      // Implicit return: (…) => w.loadURL(…)
      return parent.body === node;
    case 'VariableDeclarator':
      // const p = w.loadURL(…) — assume the promise is handled later.
      return parent.init === node;
    case 'AssignmentExpression':
      return parent.right === node;
    case 'MemberExpression':
      // w.loadURL(…).catch(…) / .then(…) — the MemberExpression is the object
      // of a surrounding CallExpression; treat any property access as handled
      // to avoid false positives on .then/.catch/.finally chains.
      return parent.object === node;
    case 'CallExpression':
      // Passed as an argument, e.g. expect(w.loadURL(...)).to.eventually…,
      // Promise.all([w.loadURL(...)]), once(w, 'x').then(w.loadURL(...))
      return parent.arguments?.includes(node) || parent.callee === node;
    case 'ArrayExpression':
      // [w.loadURL(...)] — typically Promise.all input.
      return true;
    case 'ConditionalExpression':
    case 'LogicalExpression':
      // cond ? w.loadURL(a) : w.loadURL(b) — handled if the whole expr is.
      return true;
    case 'UnaryExpression':
      // void w.loadURL(...) — explicit discard.
      return parent.operator === 'void';
  }
  return false;
}

export default {
  meta: { name: 'no-unawaited-load' },
  rules: {
    'no-unawaited-load': {
      meta: { type: 'suggestion' },
      create(context) {
        const sourceCode = context.sourceCode ?? context.getSourceCode?.();
        const ancestorsOf = (n) => sourceCode?.getAncestors?.(n) ?? context.getAncestors?.() ?? [];

        return {
          CallExpression(node) {
            const callee = node.callee;
            if (
              callee.type !== 'MemberExpression' ||
              callee.property.type !== 'Identifier' ||
              !LOAD_METHODS.has(callee.property.name)
            ) {
              return;
            }
            const ancestors = ancestorsOf(node);
            const parent = ancestors[ancestors.length - 1] ?? node.parent;
            if (isHandled(node, parent)) return;
            context.report({
              node: callee.property,
              message:
                `'${callee.property.name}()' returns a promise that is not awaited, ` +
                `returned, assigned, or .catch()'d. If the load may be aborted, add ` +
                `'.catch(() => {})' or 'void' to suppress the unhandled rejection.`
            });
          }
        };
      }
    }
  }
};
