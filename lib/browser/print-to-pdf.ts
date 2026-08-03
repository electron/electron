// Option translation and job queueing for printToPDF, shared between
// webContents.printToPDF and webFrameMain.printToPDF.

let nextRequestId = 0;

const paperFormats: Record<string, ElectronInternal.PageSize> = {
  letter: { width: 8.5, height: 11 },
  legal: { width: 8.5, height: 14 },
  tabloid: { width: 11, height: 17 },
  ledger: { width: 17, height: 11 },
  a0: { width: 33.1, height: 46.8 },
  a1: { width: 23.4, height: 33.1 },
  a2: { width: 16.54, height: 23.4 },
  a3: { width: 11.7, height: 16.54 },
  a4: { width: 8.27, height: 11.7 },
  a5: { width: 5.83, height: 8.27 },
  a6: { width: 4.13, height: 5.83 }
} as const;

function checkType<T>(value: T, type: 'number' | 'boolean' | 'string' | 'object', name: string): T {
  // eslint-disable-next-line valid-typeof
  if (typeof value !== type) {
    throw new TypeError(`${name} must be a ${type}`);
  }

  return value;
}

function parsePageSize(pageSize: string | ElectronInternal.PageSize) {
  if (typeof pageSize === 'string') {
    const format = paperFormats[pageSize.toLowerCase()];
    if (!format) {
      throw new Error(`Invalid pageSize ${pageSize}`);
    }

    return { paperWidth: format.width, paperHeight: format.height };
  } else if (typeof pageSize === 'object') {
    if (typeof pageSize.width !== 'number' || typeof pageSize.height !== 'number') {
      throw new TypeError('width and height properties are required for pageSize');
    }

    return { paperWidth: pageSize.width, paperHeight: pageSize.height };
  } else {
    throw new TypeError('pageSize must be a string or an object');
  }
}

type PrintToPDFOptions = Parameters<Electron.WebContents['printToPDF']>[0];

interface PrintToPDFTarget {
  _printToPDF?: (settings: any) => Promise<Buffer>;
  // WebFrameMain#top; used to serialize jobs within a frame tree.
  top?: Electron.WebFrameMain | null;
  // WebContents#mainFrame.
  mainFrame?: Electron.WebFrameMain;
  // WebFrameMain#frameTreeNodeId.
  frameTreeNodeId?: number;
}

// Concurrent PDF print jobs within the same frame tree conflict in the
// renderer, so queue them per frame tree. Frames in other webContents can
// print concurrently. Keyed by the top frame's frameTreeNodeId, which unlike
// WebFrameMain object identity is stable across navigations; settled entries
// are deleted, so the map only holds in-flight frame trees.
const printToPDFQueues = new Map<number, Promise<unknown>>();

export async function printToPDF(target: PrintToPDFTarget, options: PrintToPDFOptions): Promise<Buffer> {
  const margins = checkType(options.margins ?? {}, 'object', 'margins');
  const pageSize = parsePageSize(options.pageSize ?? 'letter');

  const { top, bottom, left, right } = margins;
  const validHeight = [top, bottom].every((u) => u === undefined || u <= pageSize.paperHeight);
  const validWidth = [left, right].every((u) => u === undefined || u <= pageSize.paperWidth);

  if (!validHeight || !validWidth) {
    throw new Error('margins must be less than or equal to pageSize');
  }

  const printSettings = {
    requestID: ++nextRequestId,
    landscape: checkType(options.landscape ?? false, 'boolean', 'landscape'),
    displayHeaderFooter: checkType(options.displayHeaderFooter ?? false, 'boolean', 'displayHeaderFooter'),
    headerTemplate: checkType(options.headerTemplate ?? '', 'string', 'headerTemplate'),
    footerTemplate: checkType(options.footerTemplate ?? '', 'string', 'footerTemplate'),
    printBackground: checkType(options.printBackground ?? false, 'boolean', 'printBackground'),
    scale: checkType(options.scale ?? 1.0, 'number', 'scale'),
    marginTop: checkType(margins.top ?? 0.4, 'number', 'margins.top'),
    marginBottom: checkType(margins.bottom ?? 0.4, 'number', 'margins.bottom'),
    marginLeft: checkType(margins.left ?? 0.4, 'number', 'margins.left'),
    marginRight: checkType(margins.right ?? 0.4, 'number', 'margins.right'),
    pageRanges: checkType(options.pageRanges ?? '', 'string', 'pageRanges'),
    preferCSSPageSize: checkType(options.preferCSSPageSize ?? false, 'boolean', 'preferCSSPageSize'),
    generateTaggedPDF: checkType(options.generateTaggedPDF ?? false, 'boolean', 'generateTaggedPDF'),
    generateDocumentOutline: checkType(options.generateDocumentOutline ?? false, 'boolean', 'generateDocumentOutline'),
    ...pageSize
  };

  if (!target._printToPDF) {
    throw new Error('Printing feature is disabled');
  }

  // mainFrame/top can be null mid-teardown; those jobs share one bucket.
  const queueKey = (target.mainFrame ?? target.top ?? target).frameTreeNodeId ?? -1;
  const prev = printToPDFQueues.get(queueKey) ?? Promise.resolve();
  const next = prev.catch(() => {}).then(() => target._printToPDF!(printSettings));
  printToPDFQueues.set(queueKey, next);
  next
    .finally(() => {
      if (printToPDFQueues.get(queueKey) === next) printToPDFQueues.delete(queueKey);
    })
    .catch(() => {});
  return next;
}
