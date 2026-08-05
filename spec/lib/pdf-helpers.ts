import * as cp from 'node:child_process';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';

const pdfReaderPath = path.resolve(__dirname, '..', 'fixtures', 'api', 'pdf-reader.mjs');

// Parses a printToPDF result buffer with pdf.js in a subprocess and returns
// info about the document and its first page.
export const readPDF = async (data: any) => {
  const tmpDir = await fs.promises.mkdtemp(path.resolve(os.tmpdir(), 'e-spec-printtopdf-'));
  const pdfPath = path.resolve(tmpDir, 'test.pdf');
  await fs.promises.writeFile(pdfPath, data);

  const result = cp.spawn(process.execPath, [pdfReaderPath, pdfPath], {
    stdio: 'pipe'
  });

  const stdout: Buffer[] = [];
  const stderr: Buffer[] = [];
  result.stdout.on('data', (chunk) => stdout.push(chunk));
  result.stderr.on('data', (chunk) => stderr.push(chunk));

  const [code, signal] = await new Promise<[number | null, NodeJS.Signals | null]>((resolve) => {
    result.on('close', (code, signal) => {
      resolve([code, signal]);
    });
  });
  await fs.promises.rm(tmpDir, { force: true, recursive: true });
  if (code !== 0) {
    const errMsg = Buffer.concat(stderr).toString().trim();
    console.error(`Error parsing PDF file, exit code was ${code}; signal was ${signal}, error: ${errMsg}`);
  }
  try {
    return JSON.parse(Buffer.concat(stdout).toString().trim());
  } catch (err) {
    console.error('Error parsing PDF file:', err);
    console.error('Raw output:', Buffer.concat(stdout).toString().trim());
    throw err;
  }
};

export const containsText = (items: any[], text: RegExp) => {
  return items.some(({ str }: { str: string }) => str.match(text));
};
