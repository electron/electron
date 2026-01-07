// Stock page sizes
export const PDFPageSizes: Record<string, ElectronInternal.MediaSize> = {
  Letter: {
    custom_display_name: 'Letter',
    height_microns: 279400,
    name: 'NA_LETTER',
    width_microns: 215900
  },
  Legal: {
    custom_display_name: 'Legal',
    height_microns: 355600,
    name: 'NA_LEGAL',
    width_microns: 215900
  },
  Tabloid: {
    height_microns: 431800,
    name: 'NA_LEDGER',
    width_microns: 279400,
    custom_display_name: 'Tabloid'
  },
  A0: {
    custom_display_name: 'A0',
    height_microns: 1189000,
    name: 'ISO_A0',
    width_microns: 841000
  },
  A1: {
    custom_display_name: 'A1',
    height_microns: 841000,
    name: 'ISO_A1',
    width_microns: 594000
  },
  A2: {
    custom_display_name: 'A2',
    height_microns: 594000,
    name: 'ISO_A2',
    width_microns: 420000
  },
  A3: {
    custom_display_name: 'A3',
    height_microns: 420000,
    name: 'ISO_A3',
    width_microns: 297000
  },
  A4: {
    custom_display_name: 'A4',
    height_microns: 297000,
    name: 'ISO_A4',
    width_microns: 210000,
    is_default: 'true'
  },
  A5: {
    custom_display_name: 'A5',
    height_microns: 210000,
    name: 'ISO_A5',
    width_microns: 148000
  },
  A6: {
    custom_display_name: 'A6',
    height_microns: 148000,
    name: 'ISO_A6',
    width_microns: 105000
  }
} as const;

export const paperFormats: Record<string, ElectronInternal.PageSize> = {
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

// The minimum micron size Chromium accepts is that where:
// Per printing/units.h:
//  * kMicronsPerInch - Length of an inch in 0.001mm unit.
//  * kPointsPerInch - Length of an inch in CSS's 1pt unit.
//
// Formula: (kPointsPerInch / kMicronsPerInch) * size >= 1
//
// Practically, this means microns need to be > 352 microns.
// We therefore need to verify this or it will silently fail.
export const isValidCustomPageSize = (width: number, height: number) => {
  return [width, height].every(x => x > 352);
};

export function checkType<T> (value: T, type: 'number' | 'boolean' | 'string' | 'object', name: string): T {
  // eslint-disable-next-line valid-typeof
  if (typeof value !== type) {
    throw new TypeError(`${name} must be a ${type}`);
  }

  return value;
}

export const parsePageSize = (pageSize: string | ElectronInternal.PageSize) => {
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
};

export function sanitizePrintOptions (options: any, getNextId: () => number) {
  const margins = checkType(options.margins ?? {}, 'object', 'margins');
  const pageSize = parsePageSize(options.pageSize ?? 'letter');

  const { top, bottom, left, right } = margins;
  const validHeight = [top, bottom].every(u => u === undefined || u <= pageSize.paperHeight);
  const validWidth = [left, right].every(u => u === undefined || u <= pageSize.paperWidth);

  if (!validHeight || !validWidth) {
    throw new Error('margins must be less than or equal to pageSize');
  }

  return {
    requestID: getNextId(),
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
}
