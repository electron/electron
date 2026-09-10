import { expect } from 'chai';

import * as fs from 'node:fs/promises';
import * as path from 'node:path';

import { waitUntil } from './spec-helpers';

export interface PrintCapture {
  deviceName: string;
  jobsDirectory?: string;
  jobsUrl?: string;
  authenticationReport?: string;
  inputTray: { id: string; ipp: string };
  mediaType: { id: string; ipp: string };
}

export interface CapturedPrintJob {
  CONTENT_TYPE: string;
  IPP_MEDIA_COL?: string;
  IPP_MEDIA_COL_DEFAULT?: string;
  document: { bytes: number; prefix: string };
}

export function getPrintCapture(): PrintCapture | undefined {
  const configuration = process.env.ELECTRON_TEST_PRINT_CAPTURE;
  if (!configuration) return;
  const capture: PrintCapture = JSON.parse(configuration);
  expect(capture.deviceName).to.equal(process.env.ELECTRON_TEST_PRINTER_NAME);
  expect(Boolean(capture.jobsDirectory) !== Boolean(capture.jobsUrl), 'configure one capture location').to.equal(true);
  for (const option of [capture.inputTray, capture.mediaType]) {
    expect(option.id).to.be.a('string').and.not.be.empty;
    expect(option.ipp).to.match(/^[a-z0-9-]+$/);
  }
  return capture;
}

async function fetchJson(url: string) {
  const response = await fetch(url, { signal: AbortSignal.timeout(10000) });
  if (!response.ok) throw new Error(`Print capture request failed: ${response.status}`);
  return response.json();
}

export async function listPrintCaptures(capture: PrintCapture): Promise<string[]> {
  const names: string[] = capture.jobsDirectory
    ? await fs.readdir(capture.jobsDirectory)
    : await fetchJson(capture.jobsUrl!);
  return names.filter((name) => /^[\w.-]+\.json$/.test(name));
}

export async function readNextPrintCapture(capture: PrintCapture, previous: string[]): Promise<CapturedPrintJob> {
  let job: CapturedPrintJob | undefined;
  await waitUntil(
    async () => {
      const added = (await listPrintCaptures(capture)).filter((name) => !previous.includes(name));
      expect(added.length, 'another job interfered with the dedicated virtual printer').to.be.at.most(1);
      if (added.length === 0) return false;
      job = capture.jobsDirectory
        ? JSON.parse(await fs.readFile(path.join(capture.jobsDirectory, added[0]), 'utf8'))
        : await fetchJson(`${capture.jobsUrl!}/${encodeURIComponent(added[0])}`);
      return true;
    },
    { timeout: 45000 }
  );
  expect(job!.CONTENT_TYPE).to.equal('application/pdf');
  expect(job!.document.bytes).to.be.greaterThan(100);
  expect(job!.document.prefix).to.match(/^%PDF-/);
  return job!;
}

export function mediaMember(collection: string | undefined, name: string): string | undefined {
  // The fixture's selected IPP keywords and integer dimensions need no quoting.
  return collection?.match(new RegExp(`(?:^|[\\s{])${name}=([^\\s}]+)`))?.[1];
}

export function effectiveMediaMember(job: CapturedPrintJob, name: string): string | undefined {
  return mediaMember(job.IPP_MEDIA_COL, name) ?? mediaMember(job.IPP_MEDIA_COL_DEFAULT, name);
}

export async function checkPrinterAuthentication(capture: PrintCapture) {
  if (!capture.authenticationReport) return;
  const report = JSON.parse(await fs.readFile(capture.authenticationReport, 'utf8'));
  expect(report.secretReads, 'GTK must retrieve the fixture printer credential').to.be.greaterThan(0);
}
