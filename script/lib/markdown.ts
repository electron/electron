import * as fs from 'fs';
import * as path from 'path';

import * as MarkdownIt from 'markdown-it';
import {
  githubSlugifier,
  resolveInternalDocumentLink,
  ExternalHref,
  FileStat,
  HrefKind,
  InternalHref,
  IMdLinkComputer,
  IMdParser,
  ITextDocument,
  IWorkspace,
  MdLink,
  MdLinkKind
} from '@dsanders11/vscode-markdown-languageservice';
import { Emitter, Range } from 'vscode-languageserver';
import { TextDocument } from 'vscode-languageserver-textdocument';
import { URI } from 'vscode-uri';

import { findMatchingFiles } from './utils';

import type { Definition, ImageReference, Link, LinkReference } from 'mdast';
import type { fromMarkdown as FromMarkdownFunction } from 'mdast-util-from-markdown';
import type { Node, Position } from 'unist';
import type { visit as VisitFunction } from 'unist-util-visit';

// Helper function to work around import issues with ESM modules and ts-node
// eslint-disable-next-line no-new-func
const dynamicImport = new Function('specifier', 'return import(specifier)');

// Helper function from `vscode-markdown-languageservice` codebase
function tryDecodeUri (str: string): string {
  try {
    return decodeURI(str);
  } catch {
    return str;
  }
}

// Helper function from `vscode-markdown-languageservice` codebase
function createHref (
  sourceDocUri: URI,
  link: string,
  workspace: IWorkspace
): ExternalHref | InternalHref | undefined {
  if (/^[a-z-][a-z-]+:/i.test(link)) {
    // Looks like a uri
    return { kind: HrefKind.External, uri: URI.parse(tryDecodeUri(link)) };
  }

  const resolved = resolveInternalDocumentLink(sourceDocUri, link, workspace);
  if (!resolved) {
    return undefined;
  }

  return {
    kind: HrefKind.Internal,
    path: resolved.resource,
    fragment: resolved.linkFragment
  };
}

function positionToRange (position: Position): Range {
  return {
    start: {
      character: position.start.column - 1,
      line: position.start.line - 1
    },
    end: { character: position.end.column - 1, line: position.end.line - 1 }
  };
}

const mdIt = MarkdownIt({ html: true });

export class MarkdownParser implements IMdParser {
  slugifier = githubSlugifier;

  async tokenize (document: TextDocument) {
    return mdIt.parse(document.getText(), {});
  }
}

export class DocsWorkspace implements IWorkspace {
  private readonly documentCache: Map<string, TextDocument>;
  readonly root: string;

  constructor (root: string) {
    this.documentCache = new Map();
    this.root = root;
  }

  get workspaceFolders () {
    return [URI.file(this.root)];
  }

  async getAllMarkdownDocuments (): Promise<Iterable<ITextDocument>> {
    const files = await findMatchingFiles(this.root, (file) =>
      file.endsWith('.md')
    );

    for (const file of files) {
      const document = TextDocument.create(
        URI.file(file).toString(),
        'markdown',
        1,
        fs.readFileSync(file, 'utf8')
      );

      this.documentCache.set(file, document);
    }

    return this.documentCache.values();
  }

  hasMarkdownDocument (resource: URI) {
    const relativePath = path.relative(this.root, resource.path);
    return (
      !relativePath.startsWith('..') &&
      !path.isAbsolute(relativePath) &&
      fs.existsSync(resource.path)
    );
  }

  async openMarkdownDocument (resource: URI) {
    if (!this.documentCache.has(resource.path)) {
      const document = TextDocument.create(
        resource.toString(),
        'markdown',
        1,
        fs.readFileSync(resource.path, 'utf8')
      );

      this.documentCache.set(resource.path, document);
    }

    return this.documentCache.get(resource.path);
  }

  async stat (resource: URI): Promise<FileStat | undefined> {
    if (this.hasMarkdownDocument(resource)) {
      const stats = fs.statSync(resource.path);
      return { isDirectory: stats.isDirectory() };
    }

    return undefined;
  }

  async readDirectory (): Promise<Iterable<readonly [string, FileStat]>> {
    throw new Error('Not implemented');
  }

  //
  // These events are defined to fulfill the interface, but are never emitted
  // by this implementation since it's not meant for watching a workspace
  //

  #onDidChangeMarkdownDocument = new Emitter<ITextDocument>();
  onDidChangeMarkdownDocument = this.#onDidChangeMarkdownDocument.event;

  #onDidCreateMarkdownDocument = new Emitter<ITextDocument>();
  onDidCreateMarkdownDocument = this.#onDidCreateMarkdownDocument.event;

  #onDidDeleteMarkdownDocument = new Emitter<URI>();
  onDidDeleteMarkdownDocument = this.#onDidDeleteMarkdownDocument.event;
}

export class MarkdownLinkComputer implements IMdLinkComputer {
  private readonly workspace: IWorkspace;

  constructor (workspace: IWorkspace) {
    this.workspace = workspace;
  }

  async getAllLinks (document: ITextDocument): Promise<MdLink[]> {
    const { fromMarkdown } = (await dynamicImport(
      'mdast-util-from-markdown'
    )) as { fromMarkdown: typeof FromMarkdownFunction };

    const tree = fromMarkdown(document.getText());

    const links = [
      ...(await this.#getInlineLinks(document, tree)),
      ...(await this.#getReferenceLinks(document, tree)),
      ...(await this.#getLinkDefinitions(document, tree))
    ];

    return links;
  }

  async #getInlineLinks (
    document: ITextDocument,
    tree: Node
  ): Promise<MdLink[]> {
    const { visit } = (await dynamicImport('unist-util-visit')) as {
      visit: typeof VisitFunction;
    };

    const documentUri = URI.parse(document.uri);
    const links: MdLink[] = [];

    visit(
      tree,
      (node) => node.type === 'link',
      (node: Node) => {
        const link = node as Link;
        const href = createHref(documentUri, link.url, this.workspace);

        if (href) {
          const range = positionToRange(link.position!);

          // NOTE - These haven't been implemented properly, but their
          //        values aren't used for the link linting use-case
          const targetRange = range;
          const hrefRange = range;
          const fragmentRange = undefined;

          links.push({
            kind: MdLinkKind.Link,
            href,
            source: {
              hrefText: link.url,
              resource: documentUri,
              range,
              targetRange,
              hrefRange,
              fragmentRange,
              pathText: link.url.split('#')[0]
            }
          });
        }
      }
    );

    return links;
  }

  async #getReferenceLinks (
    document: ITextDocument,
    tree: Node
  ): Promise<MdLink[]> {
    const { visit } = (await dynamicImport('unist-util-visit')) as {
      visit: typeof VisitFunction;
    };

    const links: MdLink[] = [];

    visit(
      tree,
      (node) => ['imageReference', 'linkReference'].includes(node.type),
      (node: Node) => {
        const link = node as ImageReference | LinkReference;
        const range = positionToRange(link.position!);

        // NOTE - These haven't been implemented properly, but their
        //        values aren't used for the link linting use-case
        const targetRange = range;
        const hrefRange = range;

        links.push({
          kind: MdLinkKind.Link,
          href: {
            kind: HrefKind.Reference,
            ref: link.label!
          },
          source: {
            hrefText: link.label!,
            resource: URI.parse(document.uri),
            range,
            targetRange,
            hrefRange,
            fragmentRange: undefined,
            pathText: link.label!
          }
        });
      }
    );

    return links;
  }

  async #getLinkDefinitions (
    document: ITextDocument,
    tree: Node
  ): Promise<MdLink[]> {
    const { visit } = (await dynamicImport('unist-util-visit')) as {
      visit: typeof VisitFunction;
    };

    const documentUri = URI.parse(document.uri);
    const links: MdLink[] = [];

    visit(
      tree,
      (node) => node.type === 'definition',
      (node: Node) => {
        const definition = node as Definition;
        const href = createHref(documentUri, definition.url, this.workspace);

        if (href) {
          const range = positionToRange(definition.position!);

          // NOTE - These haven't been implemented properly, but their
          //        values aren't used for the link linting use-case
          const targetRange = range;
          const hrefRange = range;
          const fragmentRange = undefined;

          links.push({
            kind: MdLinkKind.Definition,
            href,
            ref: {
              range,
              text: definition.label!
            },
            source: {
              hrefText: definition.url,
              resource: documentUri,
              range,
              targetRange,
              hrefRange,
              fragmentRange,
              pathText: definition.url.split('#')[0]
            }
          });
        }
      }
    );

    return links;
  }
}
