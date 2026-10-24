// Parses printToPDF result buffers with pdf.js and returns info about each
// document and its first page, as plain JSON data. pdf.js runs in this
// process: starting a separate Electron for it took longer than the printing
// itself and pushed tests that check several PDFs past their timeout on slow
// CI hosts.
export const readPDFs = async (datas: Uint8Array[]) => {
  const pdfjs = await import('pdfjs-dist/legacy/build/pdf.mjs');
  const pdfInfos = [];
  for (const data of datas) {
    // pdf.js takes ownership of (detaches) the buffer it is given, so give it
    // a copy.
    const doc = await pdfjs.getDocument({ data: new Uint8Array(data) }).promise;
    try {
      const page = await doc.getPage(1);
      const { items } = await page.getTextContent();
      pdfInfos.push({
        numPages: doc.numPages,
        view: page.view,
        textContent: items,
        markInfo: await doc.getMarkInfo()
      });
    } finally {
      await doc.destroy();
    }
  }
  return JSON.parse(JSON.stringify(pdfInfos)) as any[];
};

export const readPDF = async (data: Uint8Array) => {
  const [pdfInfo] = await readPDFs([data]);
  return pdfInfo;
};

export const containsText = (items: any[], text: RegExp) => {
  return items.some(({ str }: { str: string }) => str.match(text));
};
