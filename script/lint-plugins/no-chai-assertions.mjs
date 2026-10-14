// vitest's expect() still answers to chai's chainable language
// (`expect(x).to.be.true`, `.to.have.property()`), but those chains end in
// bare property reads: a typo or a dropped call asserts nothing and passes.
// The specs use vitest's matchers (`toBe()`, `toHaveProperty()`), which are
// always calls; this keeps the chai form from coming back.
const CHAI_CHAIN_WORDS = new Set([
  'to',
  'be',
  'been',
  'is',
  'that',
  'which',
  'and',
  'has',
  'have',
  'with',
  'same',
  'does',
  'eventually',
  'deep',
  'nested',
  'own',
  'ordered',
  'itself'
]);

// The expect() call a member expression hangs off, if any, and whether a chai
// word was already seen closer to it (so each chain is reported once).
function expectRoot(node) {
  let n = node;
  while (n) {
    if (n.type === 'CallExpression') {
      if (n.callee.type === 'Identifier' && n.callee.name === 'expect') return n;
      n = n.callee;
    } else if (n.type === 'MemberExpression') {
      if (!n.computed && n.property.type === 'Identifier' && CHAI_CHAIN_WORDS.has(n.property.name)) return null;
      n = n.object;
    } else if (n.type === 'TSNonNullExpression' || n.type === 'ParenthesizedExpression') {
      n = n.expression;
    } else {
      return null;
    }
  }
  return null;
}

export default {
  meta: { name: 'no-chai-assertions' },
  rules: {
    'no-chai-assertions': {
      meta: { type: 'problem' },
      create(context) {
        return {
          MemberExpression(node) {
            if (node.computed || node.property.type !== 'Identifier') return;
            if (!CHAI_CHAIN_WORDS.has(node.property.name)) return;
            if (!expectRoot(node.object)) return;
            context.report({
              node: node.property,
              message: `chai-style .${node.property.name} chain on expect(); use a vitest matcher (toBe, toEqual, toHaveProperty, ...) instead`
            });
          }
        };
      }
    }
  }
};
