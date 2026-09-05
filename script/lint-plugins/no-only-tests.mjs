const BLOCK_NAMES = new Set(['describe', 'it', 'context', 'test', 'specify', 'suite']);

export default {
  meta: { name: 'no-only-tests' },
  rules: {
    'no-only-tests': {
      meta: { type: 'problem' },
      create(context) {
        return {
          MemberExpression(node) {
            if (
              node.property.type === 'Identifier' &&
              node.property.name === 'only' &&
              node.object.type === 'Identifier' &&
              BLOCK_NAMES.has(node.object.name)
            ) {
              context.report({
                node: node.property,
                message: `${node.object.name}.only not permitted`
              });
            }
          }
        };
      }
    }
  }
};
