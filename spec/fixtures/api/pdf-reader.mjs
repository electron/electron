import * as pdfjs from 'pdfjs-dist';

async function getPDFDoc() {  
  try {
    const doc = await pdfjs.getDocument(process.argv[2]).promise;
    const page = await doc.getPage(1);
    const { items } = await page.getTextContent();
    const markInfo = await doc.getMarkInfo();
    const pdfInfo = {
      numPages: doc.numPages,
      view: page.view,
      textContent: items,
      markInfo
    }
    console.log(JSON.stringify(pdfInfo));
    process.exit();
  } catch (ex) {
    process.exit(1);
  }
}

getPDFDoc();