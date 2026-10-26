// Spec helpers whose only purpose is the promise they return - discarding it
// silently skips the wait (and any assertion inside it).
const MUST_AWAIT = new Set([
  'waitUntil',
  'repeatedly',
  'closeWindow',
  'closeAllWindows',
  'cleanupWebContents',
  'runCleanupFunctions'
]);

export default {
  meta: { name: 'no-floating-spec-helpers' },
  rules: {
    'no-floating-spec-helpers': {
      meta: { type: 'problem' },
      create(context) {
        return {
          ExpressionStatement(node) {
            const call = node.expression;
            if (call.type !== 'CallExpression') return;
            if (call.callee.type !== 'Identifier') return;
            if (!MUST_AWAIT.has(call.callee.name)) return;

            context.report({
              node: call.callee,
              message: `${call.callee.name}() returns a promise that must be awaited`
            });
          }
        };
      }
    }
  }
};
