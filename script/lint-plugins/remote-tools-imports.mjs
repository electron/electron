// Flags imports (other than from spec/lib/remote-tools) that are referenced
// inside itremote()/remotely() closures. vite's SSR transform rewrites import
// bindings to __vite_ssr_import_N__, which breaks when the closure is
// stringified and eval'd in a renderer/child process. Importing from
// remote-tools instead lets runRemote()'s __rt shim resolve them.

const REMOTE_TOOLS_RE = /[./]lib\/remote-tools(\.ts)?$/;
const REMOTE_TAG_RE = /@remote\b/;

function hasRemoteTag(node, sourceCode) {
  const comments = sourceCode.getCommentsBefore?.(node) ?? node.leadingComments ?? [];
  return comments.some((c) => REMOTE_TAG_RE.test(c.value));
}

function isRemoteCall(callee, taggedCallees) {
  // itremote(name, fn), itremote.only(name, fn), itremote.skip(name, fn)
  if (callee.type === 'Identifier' && callee.name === 'itremote') return { fnArg: 1 };
  if (
    callee.type === 'MemberExpression' &&
    callee.object.type === 'Identifier' &&
    callee.object.name === 'itremote' &&
    callee.property.type === 'Identifier' &&
    (callee.property.name === 'only' || callee.property.name === 'skip')
  ) {
    return { fnArg: 1 };
  }
  // <anything>.remotely(fn, ...args)
  if (
    callee.type === 'MemberExpression' &&
    callee.property.type === 'Identifier' &&
    callee.property.name === 'remotely'
  ) {
    return { fnArg: 0 };
  }
  // Calls to identifiers tagged /** @remote */ (function decls, const bindings,
  // or for-of loop vars). fn arg is assumed to be the first function-expression
  // argument, resolved at the call site.
  if (callee.type === 'Identifier' && taggedCallees.has(callee.name)) {
    return { fnArg: 'firstFunction' };
  }
  return null;
}

function collectDeclared(node, declared) {
  if (!node) return;
  if (node.type === 'Identifier') {
    declared.add(node.name);
  } else if (node.type === 'ObjectPattern') {
    for (const p of node.properties) collectDeclared(p.value ?? p.argument, declared);
  } else if (node.type === 'ArrayPattern') {
    for (const e of node.elements) collectDeclared(e, declared);
  } else if (node.type === 'RestElement') {
    collectDeclared(node.argument, declared);
  } else if (node.type === 'AssignmentPattern') {
    collectDeclared(node.left, declared);
  }
}

function walkClosure(node, onIdentifierRef, declared) {
  if (!node || typeof node !== 'object') return;
  switch (node.type) {
    case 'Identifier':
      if (!declared.has(node.name)) onIdentifierRef(node);
      return;
    case 'MemberExpression':
      walkClosure(node.object, onIdentifierRef, declared);
      if (node.computed) walkClosure(node.property, onIdentifierRef, declared);
      return;
    case 'Property':
      if (node.computed) walkClosure(node.key, onIdentifierRef, declared);
      walkClosure(node.value, onIdentifierRef, declared);
      return;
    case 'VariableDeclarator':
      collectDeclared(node.id, declared);
      walkClosure(node.init, onIdentifierRef, declared);
      return;
    case 'FunctionDeclaration':
    case 'FunctionExpression':
    case 'ArrowFunctionExpression': {
      const inner = new Set(declared);
      if (node.id) inner.add(node.id.name);
      for (const p of node.params) collectDeclared(p, inner);
      walkClosure(node.body, onIdentifierRef, inner);
      return;
    }
    case 'CatchClause': {
      const inner = new Set(declared);
      collectDeclared(node.param, inner);
      walkClosure(node.body, onIdentifierRef, inner);
      return;
    }
  }
  for (const key of Object.keys(node)) {
    if (
      key === 'type' ||
      key === 'loc' ||
      key === 'range' ||
      key === 'start' ||
      key === 'end' ||
      key === 'parent' ||
      key === 'typeAnnotation' ||
      key === 'typeParameters' ||
      key === 'returnType'
    ) {
      continue;
    }
    const child = node[key];
    if (Array.isArray(child)) {
      for (const c of child) walkClosure(c, onIdentifierRef, declared);
    } else if (child && typeof child === 'object' && typeof child.type === 'string') {
      walkClosure(child, onIdentifierRef, declared);
    }
  }
}

export default {
  meta: { name: 'remote-tools-imports' },
  rules: {
    'no-foreign-imports-in-remote-closure': {
      meta: { type: 'problem' },
      create(context) {
        // Map of import-binding name -> { source, node } for everything NOT
        // from remote-tools.
        const foreignImports = new Map();
        // Names of functions/bindings tagged /** @remote */ in this file.
        const taggedCallees = new Set();
        const sourceCode = context.sourceCode ?? context.getSourceCode?.();

        const collectTagged = (idNode, commentTarget) => {
          if (idNode?.type === 'Identifier' && hasRemoteTag(commentTarget, sourceCode)) {
            taggedCallees.add(idNode.name);
          }
        };

        return {
          ImportDeclaration(node) {
            const source = node.source.value;
            if (REMOTE_TOOLS_RE.test(source)) return;
            if (node.importKind === 'type') return;
            for (const spec of node.specifiers) {
              if (spec.importKind === 'type') continue;
              foreignImports.set(spec.local.name, { source, node: spec.local });
            }
          },
          FunctionDeclaration(node) {
            collectTagged(node.id, node);
          },
          VariableDeclaration(node) {
            if (!hasRemoteTag(node, sourceCode)) return;
            for (const d of node.declarations) collectTagged(d.id, node);
          },
          ForOfStatement(node) {
            if (!hasRemoteTag(node, sourceCode)) return;
            const decl = node.left;
            if (decl.type === 'VariableDeclaration') {
              for (const d of decl.declarations) collectTagged(d.id, node);
            }
          },
          CallExpression(node) {
            const match = isRemoteCall(node.callee, taggedCallees);
            if (!match) return;
            const fnArg =
              match.fnArg === 'firstFunction'
                ? node.arguments.find(
                    (a) => a && (a.type === 'FunctionExpression' || a.type === 'ArrowFunctionExpression')
                  )
                : node.arguments[match.fnArg];
            if (!fnArg || (fnArg.type !== 'FunctionExpression' && fnArg.type !== 'ArrowFunctionExpression')) {
              return;
            }
            const reported = new Set();
            walkClosure(
              fnArg,
              (id) => {
                const hit = foreignImports.get(id.name);
                if (hit && !reported.has(id.name)) {
                  reported.add(id.name);
                  context.report({
                    node: id,
                    message:
                      `'${id.name}' is imported from '${hit.source}' but used inside a ` +
                      `stringified remote closure. Import it from './lib/remote-tools' ` +
                      `instead (add it there if missing).`
                  });
                }
              },
              new Set()
            );
          }
        };
      }
    }
  }
};
