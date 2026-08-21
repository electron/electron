// Validates and defaults webContents.print(options); the native side only maps
// the result onto Chromium's job settings.

const pageSizes: Record<string, ElectronInternal.MediaSize> = {
  Letter: { custom_display_name: 'Letter', height_microns: 279400, name: 'NA_LETTER', width_microns: 215900 },
  Legal: { custom_display_name: 'Legal', height_microns: 355600, name: 'NA_LEGAL', width_microns: 215900 },
  Tabloid: { custom_display_name: 'Tabloid', height_microns: 431800, name: 'NA_LEDGER', width_microns: 279400 },
  A0: { custom_display_name: 'A0', height_microns: 1189000, name: 'ISO_A0', width_microns: 841000 },
  A1: { custom_display_name: 'A1', height_microns: 841000, name: 'ISO_A1', width_microns: 594000 },
  A2: { custom_display_name: 'A2', height_microns: 594000, name: 'ISO_A2', width_microns: 420000 },
  A3: { custom_display_name: 'A3', height_microns: 420000, name: 'ISO_A3', width_microns: 297000 },
  A4: { custom_display_name: 'A4', height_microns: 297000, name: 'ISO_A4', width_microns: 210000 },
  A5: { custom_display_name: 'A5', height_microns: 210000, name: 'ISO_A5', width_microns: 148000 },
  A6: { custom_display_name: 'A6', height_microns: 148000, name: 'ISO_A6', width_microns: 105000 }
};

const marginTypes = ['default', 'none', 'printableArea', 'custom'] as const;
const duplexModes = ['simplex', 'shortEdge', 'longEdge'] as const;

// printing/units.h: sizes convert to points as size * kPointsPerInch /
// kMicronsPerInch and must stay >= 1, i.e. more than 352 microns.
const kMinPageSizeMicrons = 353;

function withImageableArea(size: ElectronInternal.MediaSize): ElectronInternal.MediaSize {
  return {
    ...size,
    imageable_area_left_microns: 0,
    imageable_area_bottom_microns: 0,
    imageable_area_right_microns: size.width_microns,
    imageable_area_top_microns: size.height_microns
  };
}

// Values of the wrong type, and numbers Chromium cannot take as an int32, fall
// back to the default rather than throwing, as they always have.
function optional<T>(value: unknown, type: 'boolean' | 'string' | 'object', fallback: T): T {
  // eslint-disable-next-line valid-typeof
  return typeof value === type && value !== null ? (value as T) : fallback;
}

function integer(value: unknown, fallback: number, min = 1): number {
  return Number.isInteger(value) && (value as number) >= min && (value as number) < 2 ** 31
    ? (value as number)
    : fallback;
}

function oneOf<T extends string, F>(value: unknown, allowed: readonly T[], fallback: F): T | F {
  return allowed.includes(value as T) ? (value as T) : fallback;
}

function parsePageSize(pageSize: unknown): ElectronInternal.MediaSize | null {
  if (pageSize === undefined) return null;
  if (typeof pageSize === 'string') {
    const size = pageSizes[pageSize];
    if (!size) throw new Error(`Unsupported pageSize: ${pageSize}`);
    return withImageableArea(size);
  }
  if (typeof pageSize !== 'object' || pageSize === null) {
    throw new Error(`Unsupported pageSize: ${pageSize}`);
  }
  const size = pageSize as Electron.Size;
  if (!size.height || !size.width) {
    throw new Error('height and width properties are required for pageSize');
  }
  const height = Math.ceil(Number(size.height));
  const width = Math.ceil(Number(size.width));
  if (!(width >= kMinPageSizeMicrons) || !(height >= kMinPageSizeMicrons)) {
    throw new RangeError('height and width properties must be minimum 352 microns.');
  }
  return withImageableArea({
    name: 'CUSTOM',
    custom_display_name: 'Custom',
    height_microns: height,
    width_microns: width
  });
}

export function normalizePrintOptions(options: unknown): ElectronInternal.NormalizedPrintOptions | null {
  if (typeof options !== 'object' || options == null) {
    throw new TypeError('webContents.print(): Invalid print settings specified.');
  }
  const o = options as Electron.WebContentsPrintOptions;
  if (Object.keys(o).length === 0) return null;

  if (o.usePrinterDefaultPageSize !== undefined && o.pageSize !== undefined) {
    throw new Error('usePrinterDefaultPageSize cannot be combined with pageSize');
  }

  const margins = optional<Electron.Margins>(o.margins, 'object', {});
  const marginType = oneOf(margins.marginType, marginTypes, 'default');
  const dpi = optional<Record<string, unknown>>(o.dpi, 'object', {});
  const horizontalDpi = integer(dpi.horizontal, 0);
  const verticalDpi = integer(dpi.vertical, 0);
  const usePrinterDefaultPageSize = optional(o.usePrinterDefaultPageSize, 'boolean', false);

  return {
    silent: optional(o.silent, 'boolean', false),
    printBackground: optional(o.printBackground, 'boolean', false),
    deviceName: optional(o.deviceName, 'string', ''),
    color: optional(o.color, 'boolean', true),
    marginType,
    margins:
      marginType === 'custom'
        ? {
            top: integer(margins.top, 0, 0),
            bottom: integer(margins.bottom, 0, 0),
            left: integer(margins.left, 0, 0),
            right: integer(margins.right, 0, 0)
          }
        : null,
    landscape: optional(o.landscape, 'boolean', false),
    scaleFactor: integer(o.scaleFactor, 100),
    pagesPerSheet: integer(o.pagesPerSheet, 1),
    collate: optional(o.collate, 'boolean', true),
    copies: integer(o.copies, 1),
    pageRanges: (Array.isArray(o.pageRanges) ? o.pageRanges : [])
      .map((r) => ({ from: integer(r?.from, -1, 0), to: integer(r?.to, -1, 0) }))
      .filter((r) => r.from >= 0 && r.to >= r.from),
    duplexMode: oneOf(o.duplexMode, duplexModes, null),
    dpi:
      horizontalDpi || verticalDpi
        ? { horizontal: horizontalDpi || verticalDpi, vertical: verticalDpi || horizontalDpi }
        : null,
    header: optional(o.header, 'string', ''),
    footer: optional(o.footer, 'string', ''),
    mediaSize: parsePageSize(o.pageSize) ?? (usePrinterDefaultPageSize ? null : withImageableArea(pageSizes.A4)),
    usePrinterDefaultPageSize
  };
}
