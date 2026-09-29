import { app } from 'electron';

// Chromium switches (e.g. --disable-gpu) follow the PDF paths on the command line.
const pdfPaths = process.argv.slice(2).filter((arg) => !arg.startsWith('-'));

async function getPDFInfo(pdfjs, pdfPath) {
  const doc = await pdfjs.getDocument(pdfPath).promise;
  const page = await doc.getPage(1);
  const { items } = await page.getTextContent();
  const markInfo = await doc.getMarkInfo();
  return {
    numPages: doc.numPages,
    view: page.view,
    textContent: items,
    markInfo
  };
}

async function getPDFDocs() {
  try {
    const pdfjs = await import('pdfjs-dist/legacy/build/pdf.mjs');
    const pdfInfos = [];
    for (const pdfPath of pdfPaths) {
      pdfInfos.push(await getPDFInfo(pdfjs, pdfPath));
    }
    console.log(JSON.stringify(pdfInfos));
    process.exit();
  } catch (ex) {
    console.error(ex);
    process.exit(1);
  }
}

app.whenReady().then(() => getPDFDocs());
