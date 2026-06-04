/**
 * Zip extraction module that replaces extract-zip for Electron's install script.
 *
 * extract-zip@2.0.1 depends on yauzl@2.10.0 which uses fd-slicer@1.1.0.
 * On Node.js >= 24.16.0, fd-slicer's ReadStream has a broken destroy()
 * lifecycle that causes stream.pipeline() to hang forever on STORED entries.
 *
 * This module uses yauzl directly but avoids stream.pipeline() by manually
 * consuming the read stream with event listeners, which does not depend on
 * the 'close' event that fd-slicer fails to emit.
 *
 * @see https://github.com/electron/electron/issues/51619
 * @see https://github.com/thejoshwolfe/yauzl/issues/169
 */

'use strict';

const { promises: fs } = require('fs');
const path = require('path');
const { promisify } = require('util');
const yauzl = require('yauzl');

const openZip = promisify(yauzl.open);

/**
 * Extract a zip file to a directory.
 *
 * @param {string} zipPath - Absolute path to the zip file
 * @param {{ dir: string }} opts - Options with target directory (must be absolute)
 * @returns {Promise<void>}
 */
async function extract(zipPath, opts) {
  if (!path.isAbsolute(opts.dir)) {
    throw new Error('Target directory is expected to be absolute');
  }

  await fs.mkdir(opts.dir, { recursive: true });
  const dir = await fs.realpath(opts.dir);

  const zipfile = await openZip(zipPath, { lazyEntries: true });

  return new Promise((resolve, reject) => {
    zipfile.on('error', reject);

    zipfile.on('close', () => {
      resolve();
    });

    zipfile.readEntry();

    zipfile.on('entry', async (entry) => {
      try {
        if (entry.fileName.startsWith('__MACOSX/')) {
          zipfile.readEntry();
          return;
        }

        const destDir = path.dirname(path.join(dir, entry.fileName));
        await fs.mkdir(destDir, { recursive: true });

        // Guard against zip slip
        const canonicalDestDir = await fs.realpath(destDir);
        const relativeDestDir = path.relative(dir, canonicalDestDir);
        if (relativeDestDir.split(path.sep).includes('..')) {
          throw new Error(`Out of bound path "${canonicalDestDir}" found while processing file ${entry.fileName}`);
        }

        await extractEntry(zipfile, entry, dir);
        zipfile.readEntry();
      } catch (err) {
        zipfile.close();
        reject(err);
      }
    });
  });
}

/**
 * Extract a single zip entry to the filesystem.
 *
 * Uses manual stream consumption (readable 'data'/'end'/'error' events)
 * instead of stream.pipeline() to avoid the fd-slicer destroy() lifecycle
 * bug on Node.js >= 24.16.0.
 */
async function extractEntry(zipfile, entry, dir) {
  const dest = path.join(dir, entry.fileName);

  // Parse file attributes
  const mode = (entry.externalFileAttributes >> 16) & 0xffff;
  const IFMT = 61440;
  const IFDIR = 16384;
  const IFLNK = 40960;
  const isSymlink = (mode & IFMT) === IFLNK;
  let isDir = (mode & IFMT) === IFDIR;

  if (!isDir && entry.fileName.endsWith('/')) {
    isDir = true;
  }

  // Windows directory detection
  const madeBy = entry.versionMadeBy >> 8;
  if (!isDir) isDir = madeBy === 0 && entry.externalFileAttributes === 16;

  if (isDir) {
    await fs.mkdir(dest, { recursive: true });
    return;
  }

  const fileMode = getExtractedMode(mode) & 0o777;
  const readStream = await promisify(zipfile.openReadStream.bind(zipfile))(entry);

  if (isSymlink) {
    // Read symlink target using 'data'/'end' events (not async iteration,
    // which also depends on the broken 'close' event on Node 24.16+)
    const link = await new Promise((resolve, reject) => {
      const chunks = [];
      readStream.on('data', (chunk) => chunks.push(chunk));
      readStream.on('end', () => resolve(Buffer.concat(chunks).toString('utf8')));
      readStream.on('error', reject);
    });
    await fs.symlink(link, dest);
  } else {
    // Write file by collecting chunks and writing with fs.writeFile.
    // We cannot use stream.pipeline(), pipe(), or even manual pause/resume
    // with a WriteStream because on Node.js >= 24.16.0, fd-slicer's ReadStream
    // has a broken destroy() lifecycle that interacts badly with backpressure:
    // calling pause() on the ReadStream after fd-slicer has set this.destroyed
    // causes the stream to never resume (Node 24.16+ treats pause/resume as
    // no-ops on destroyed streams). Collecting in memory avoids this entirely.
    const data = await new Promise((resolve, reject) => {
      const chunks = [];
      readStream.on('data', (chunk) => chunks.push(chunk));
      readStream.on('end', () => resolve(Buffer.concat(chunks)));
      readStream.on('error', reject);
    });
    await fs.writeFile(dest, data, { mode: fileMode });
  }
}

function getExtractedMode(entryMode) {
  return entryMode === 0 ? 0o644 : entryMode;
}

module.exports = extract;
