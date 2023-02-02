#!/usr/bin/env ts-node

import * as path from 'path';

import {
  createLanguageService,
  ILogger
} from '@dsanders11/vscode-markdown-languageservice';
import { CancellationTokenSource } from 'vscode-languageserver';
import { URI } from 'vscode-uri';

import { DocsWorkspace, MarkdownParser } from './lib/markdown';

class NoOpLogger implements ILogger {
  log (): void {}
}

async function main () {
  if (process.argv.slice(2).length !== 1) {
    console.log('Usage: script/find-docs-references.ts [file]');
    process.exit(0);
  }

  const workspace = new DocsWorkspace(path.resolve(__dirname, '..', 'docs'));
  const parser = new MarkdownParser();
  const languageService = createLanguageService({
    workspace,
    parser,
    logger: new NoOpLogger()
  });

  const uri = URI.file(path.resolve(process.argv[2]));

  if (!workspace.hasMarkdownDocument(uri)) {
    console.log(`Error: could not open file '${uri.path}'`);
    process.exit(1);
  }

  const cts = new CancellationTokenSource();

  try {
    const references = await languageService.getFileReferences(uri, cts.token);
    console.log(JSON.stringify(references, null, 4));
  } finally {
    cts.dispose();
  }
}

if (process.mainModule === module) {
  main().catch((error) => {
    console.error(error);
    process.exit(1);
  });
}
