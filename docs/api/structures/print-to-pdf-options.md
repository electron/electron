# PrintToPDFOptions Object

* `landscape` boolean (optional) - Paper orientation.`true` for landscape, `false` for portrait. Defaults to false.
* `displayHeaderFooter` boolean (optional) - Whether to display header and footer. Defaults to false.
* `printBackground` boolean (optional) - Whether to print background graphics. Defaults to false.
* `scale` number(optional)  - Scale of the webpage rendering. Defaults to 1.
* `pageSize` string | Size (optional) - Specify page size of the generated PDF. Can be `A0`, `A1`, `A2`, `A3`,
`A4`, `A5`, `A6`, `Legal`, `Letter`, `Tabloid`, `Ledger`, or an Object containing `height` and `width` in inches. Defaults to `Letter`.
* `margins` [PrintToPDFMargins](print-to-pdf-margins.md?inline) (optional)
* `pageRanges` string (optional) - Page ranges to print, e.g., '1-5, 8, 11-13'. Defaults to the empty string, which means print all pages.
* `headerTemplate` string (optional) - HTML template for the print header. Should be valid HTML markup with following classes used to inject printing values into them: `date` (formatted print date), `title` (document title), `url` (document location), `pageNumber` (current page number) and `totalPages` (total pages in the document). For example, `<span class=title></span>` would generate span containing the title.
* `footerTemplate` string (optional) - HTML template for the print footer. Should use the same format as the `headerTemplate`.
* `preferCSSPageSize` boolean (optional) - Whether or not to prefer page size as defined by css. Defaults to false, in which case the content will be scaled to fit the paper size.
* `generateTaggedPDF` boolean (optional) _Experimental_ - Whether or not to generate a tagged (accessible) PDF. Defaults to false. As this property is experimental, the generated PDF may not adhere fully to PDF/UA and WCAG standards.
* `generateDocumentOutline` boolean (optional) _Experimental_ - Whether or not to generate a PDF document outline from content headers. Defaults to false.
