import * as cp from 'node:child_process';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';

import { ciGpuArgs, defer } from './spec-helpers.ts';

const pdfReaderPath = path.resolve(import.meta.dirname, '..', 'fixtures', 'api', 'pdf-reader.mjs');

// Parses printToPDF result buffers with pdf.js in a single subprocess and
// returns info about each document and its first page. Each subprocess is a
// full Electron launch, so tests that check several PDFs should parse them in
// one call rather than one readPDF() per buffer.
export const readPDFs = async (datas: any[]) => {
  const tmpDir = await fs.promises.mkdtemp(path.resolve(os.tmpdir(), 'e-spec-printtopdf-'));
  const pdfPaths = await Promise.all(
    datas.map(async (data, i) => {
      const pdfPath = path.resolve(tmpDir, `test-${i}.pdf`);
      await fs.promises.writeFile(pdfPath, data);
      return pdfPath;
    })
  );

  const result = cp.spawn(process.execPath, [pdfReaderPath, ...pdfPaths, ...ciGpuArgs], {
    stdio: 'pipe'
  });
  // Register cleanup right away so a hung PDF read doesn't leak the child
  // into the in-job retry when mocha times out.
  defer(() => {
    if (result.exitCode === null && result.signalCode === null) {
      result.kill();
    }
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
    return JSON.parse(Buffer.concat(stdout).toString().trim()) as any[];
  } catch (err) {
    console.error('Error parsing PDF file:', err);
    console.error('Raw output:', Buffer.concat(stdout).toString().trim());
    throw err;
  }
};

export const readPDF = async (data: any) => {
  const [pdfInfo] = await readPDFs([data]);
  return pdfInfo;
};

export const containsText = (items: any[], text: RegExp) => {
  return items.some(({ str }: { str: string }) => str.match(text));
};
