import { app } from 'electron';

async function getPDFDoc () {
  try {
    const pdfjs = await import('pdfjs-dist');
    const doc = await pdfjs.getDocument(process.argv[2]).promise;
    const page = await doc.getPage(1);
    const { items } = await page.getTextContent();
    const markInfo = await doc.getMarkInfo();
    const pdfInfo = {
      numPages: doc.numPages,
      view: page.view,
      textContent: items,
      markInfo
    };
    console.log(JSON.stringify(pdfInfo));
    process.exit();
  } catch (ex) {
    console.error(ex);
    process.exit(1);
  }
}

app.whenReady().then(() => getPDFDoc());
