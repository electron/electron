enum PrintType {
  PDF = 'PDF',
  REGULAR = 'REGULAR'
}

const marginValues: Record<string, (0 | 1 | 2 | 3)> = {
  default: 0,
  none: 1,
  printableArea: 2,
  custom: 3
}

const duplexValues: Record<string, (0 | 1 | 2)> = {
  simplex: 0,
  longEdge: 1,
  shortEdge: 2
}

// Stock page sizes
const PDFPageSizes: Record<string, PageSize> = {
  A5: {
    custom_display_name: 'A5',
    height_microns: 210000,
    name: 'ISO_A5',
    width_microns: 148000
  },
  A4: {
    custom_display_name: 'A4',
    height_microns: 297000,
    name: 'ISO_A4',
    is_default: 'true',
    width_microns: 210000
  },
  A3: {
    custom_display_name: 'A3',
    height_microns: 420000,
    name: 'ISO_A3',
    width_microns: 297000
  },
  Legal: {
    custom_display_name: 'Legal',
    height_microns: 355600,
    name: 'NA_LEGAL',
    width_microns: 215900
  },
  Letter: {
    custom_display_name: 'Letter',
    height_microns: 279400,
    name: 'NA_LETTER',
    width_microns: 215900
  },
  Tabloid: {
    height_microns: 431800,
    name: 'NA_LEDGER',
    width_microns: 279400,
    custom_display_name: 'Tabloid'
  }
}

let nextId = 0
const getNextId = () => ++nextId

// Default printing setting
const defaultPrintSettings: PrintSettings = {
  pageRange: [] as any as PageRange[],
  mediaSize: PDFPageSizes['A4'],
  landscape: false,
  generateDraftData: true,
  headerFooterEnabled: false,
  marginsType: 0,
  scaleFactor: 100,
  shouldPrintBackgrounds: false,
  shouldPrintSelectionOnly: false,
  printWithCloudPrint: false,
  printWithPrivet: false,
  printWithExtension: false,
  pagesPerSheet: 1,
  // True if this is the first preview request.
  isFirstRequest: false,
  // Unique ID to identify a print preview UI.
  previewUIID: 0,
  previewModifiable: true,
  dpiHorizontal: 72,
  dpiVertical: 72,
  // Whether to rasterize the PDF for printing.
  rasterizePDF: false,
  duplex: 0,
  copies: 1,
  color: 2,
  silent: false,
  collate: true
}

export function updatePrintSettings (settings: PrintSettings, type: PrintType) {
  const printSettings: PrintSettings = { ...defaultPrintSettings }

  settings.printToPDF = type === PrintType.PDF

  // Page orientation (true for landscape, false for portrait).
  if (settings.landscape !== undefined) {
    if (typeof settings.landscape !== 'boolean') {
      const error = new Error('landscape must be a Boolean')
      return { error }
    }
    printSettings.landscape = settings.landscape
  }

  // Scaling factor of the page (0-100).
  if (settings.scaleFactor !== undefined) {
    if (typeof settings.scaleFactor !== 'number') {
      const error = new Error('scaleFactor must be a Number')
      return { error }
    }
    printSettings.scaleFactor = settings.scaleFactor
  }

  // Chromium expects this as a 0-100 range number & not float, so ensure that.
  printSettings.scaleFactor = Math.ceil(printSettings.scaleFactor) % 100

  // See DuplexMode in //printing/print_job_constants.h.
  // Default is `simplex` -> 0.
  if (settings.duplexMode !== undefined) {
    if (!['simplex', 'longEdge', 'shortEdge'].includes(settings.duplexMode)) {
      const error = new Error(`duplexMode must be one of simplex', 'longEdge', or 'shortEdge'.`)
      return { error }
    }
    printSettings.duplex = duplexValues[settings.duplexMode]
  }

  // See ColorModel in //printing/print_job_constants.h.
  // Default is `true` -> 2 (color) and `false` -> 1 (gray)
  if (!settings.color !== undefined) {
    if (typeof settings.color !== 'boolean') {
      const error = new Error('color must be a Boolean')
      return { error }
    }
    printSettings.color = settings.color ? 2 : 1
  }

  // See MarginType in //printing/print_job_constants.h.
  if (settings.margins !== undefined) {
    const margins = settings.margins
    if (typeof margins !== 'object') {
      const error = new Error('margins must be an Object')
      return { error }
    }

    if (!Object.keys(marginValues).includes(margins.marginType)) {
      const error = new Error(`marginsType must be one of 'default', 'none', 'printableArea', or 'custom'.`)
      return { error }
    }

    // If 'custom' is specified, users must specify margin for all directions.
    if (margins.marginType === 'custom') {
      const { top, bottom, left, right } = margins
      if (![top, bottom, left, right].every(d => d !== undefined)) {
        const error = new Error(`marginsType 'custom' must define 'top', 'bottom', 'left', and 'right'.`)
        return { error }
      }
      printSettings.top = margins.top
      printSettings.bottom = margins.bottom
      printSettings.left = margins.left
      printSettings.right = margins.right
    } else {
      printSettings.marginsType = marginValues[margins.marginType]
    }
  }

  // Whether to print selection only.
  if (settings.printSelectionOnly !== undefined) {
    if (typeof settings.printSelectionOnly !== 'boolean') {
      const error = new Error('printSelectionOnly must be a Boolean')
      return { error }
    }
    printSettings.shouldPrintSelectionOnly = settings.printSelectionOnly
  }

  // Whether to print CSS backgrounds.
  if (settings.printBackground !== undefined) {
    if (typeof settings.printBackground !== 'boolean') {
      const error = new Error('printBackground must be a Boolean')
      return { error }
    }
    printSettings.shouldPrintBackgrounds = settings.printBackground
  }

  // Number of pages per sheet.
  if (settings.pagesPerSheet !== undefined) {
    if (typeof settings.pagesPerSheet !== 'boolean') {
      const error = new Error('pagesPerSheet must be a Number')
      return { error }
    }
    printSettings.pagesPerSheet = settings.pagesPerSheet
  }

  // Whether to collate (organize) the composited pages.
  if (settings.collate !== undefined) {
    if (typeof settings.collate !== 'boolean') {
      const error = new Error('collate must be a Boolean')
      return { error }
    }
    printSettings.collate = settings.collate
  }

  // The number of individual copies to print.
  if (settings.copies !== undefined) {
    if (typeof settings.copies !== 'boolean') {
      const error = new Error('copies must be a Boolean')
      return { error }
    }
    printSettings.copies = settings.copies
  }

  // The page ranges to print.
  if (settings.pageRanges !== undefined) {
    const pageRanges: PageRange = settings.pageRanges as any as PageRange
    if (!pageRanges.hasOwnProperty('from') || !pageRanges.hasOwnProperty('to')) {
      const error = new Error(`pageRanges must be an Object with 'from' and 'to' properties`)
      return { error }
    }

    if (typeof pageRanges.from !== 'number') {
      const error = new Error('pageRanges.from must be a Number')
      return { error }
    }

    if (typeof pageRanges.to !== 'number') {
      const error = new Error('pageRanges.to must be a Number')
      return { error }
    }

    // Chromium uses 1-based page ranges, so increment each by 1.
    printSettings.pageRange = [{
      // The first page of a page range. (1-based)
      from: pageRanges.from + 1,
      // The last page of a page range. (1-based)
      to: pageRanges.to + 1
    }]
  }

  // The name of the printer to print to.
  if (settings.deviceName !== undefined) {
    if (typeof settings.deviceName !== 'string') {
      const error = new Error('deviceName must be a string')
      return { error }
    }
    printSettings.deviceName = settings.deviceName
  }

  // The horizontal and vertical dots per inch (dpi) setting.
  if (settings.dpi !== undefined) {
    const dpi = settings.dpi
    if (typeof dpi === 'object') {
      if (!dpi.vertical || !dpi.horizontal) {
        const error = new Error('vertical and horizontal properties are required for dpi')
        return { error }
      }
    } else {
      const error = new Error('dpi must be an Object')
      return { error }
    }
  }

  // The header and footer to specify for the printed document.
  if (settings.headerFooter !== undefined) {
    const headerFooter = settings.headerFooter
    printSettings.headerFooterEnabled = true
    if (typeof headerFooter === 'object') {
      if (!headerFooter.url || !headerFooter.title) {
        const error = new Error('url and title properties are required for headerFooter')
        return { error }
      }
      if (typeof headerFooter.title !== 'string') {
        const error = new Error('headerFooter.title must be a String')
        return { error }
      }
      printSettings.title = headerFooter.title

      if (typeof headerFooter.url !== 'string') {
        const error = new Error('headerFooter.url must be a String')
        return { error }
      }
      printSettings.url = headerFooter.url
    } else {
      const error = new Error('headerFooter must be an Object')
      return { error }
    }
  }

  // Specifies the requested media size - see PDFPageSizes.
  if (settings.pageSize !== undefined) {
    const pageSize = settings.pageSize
    if (typeof pageSize === 'object') {
      if (!pageSize.height || !pageSize.width) {
        const error = new Error('height and width properties are required for pageSize')
        return { error }
      }
      // Dimensions in Microns (1 meter = 10^6 microns)
      printSettings.mediaSize = {
        name: 'CUSTOM',
        custom_display_name: 'Custom',
        height_microns: Math.ceil(pageSize.height),
        width_microns: Math.ceil(pageSize.width)
      }
    } else if (PDFPageSizes[pageSize]) {
      printSettings.mediaSize = PDFPageSizes[pageSize]
    } else {
      const error = new Error(`Unsupported pageSize: ${pageSize}`)
      return { error }
    }
  }

  // PDF-Specific Settings
  if (type === PrintType.PDF) {
    // Unique ID sent along every preview request.
    printSettings.requestID = getNextId()

    // Chromium-specified device name when printing to PDF.
    printSettings.deviceName = 'Save as PDF'

    // Set printing to Local Printer.
    printSettings.printerType = 2
  }

  // Regular print-specific settings
  if (type === PrintType.REGULAR) {
    if (printSettings.silent !== undefined) {
      if (typeof printSettings.silent !== 'boolean') {
        const error = new Error('silent must be a Boolean')
        return { error }
      }
      printSettings.silent = settings.silent
    }
  }

  return { settings: printSettings }
}
