import { Buffer } from 'buffer';
import { Dirent, constants } from 'fs';
import * as path from 'path';
import * as util from 'util';

import type * as Crypto from 'crypto';
import type * as os from 'os';

const asar = process._linkedBinding('electron_common_asar');

const Module = require('module') as NodeJS.ModuleInternal;

const Promise: PromiseConstructor = global.Promise;

const envNoAsar = process.env.ELECTRON_NO_ASAR && process.type !== 'browser' && process.type !== 'renderer';
const isAsarDisabled = () => process.noAsar || envNoAsar;

const internalBinding = process.internalBinding!;
delete process.internalBinding;

const nextTick = (functionToCall: Function, args: any[] = []) => {
  process.nextTick(() => functionToCall(...args));
};

const binding = internalBinding('fs');
const dirBinding = internalBinding('fs_dir');
const { kUsePromises } = binding;

// Cache asar archive objects.
const cachedArchives = new Map<string, NodeJS.AsarArchive>();

const getOrCreateArchive = (archivePath: string) => {
  const isCached = cachedArchives.has(archivePath);
  if (isCached) {
    return cachedArchives.get(archivePath)!;
  }

  try {
    const newArchive = new asar.Archive(archivePath);
    cachedArchives.set(archivePath, newArchive);
    return newArchive;
  } catch {
    return null;
  }
};

process._getOrCreateArchive = getOrCreateArchive;

const asarRe = /\.asar/i;

const { getValidatedPath, getOptions, getDirent, getStatsFromBinding } = __non_webpack_require__(
  'internal/fs/utils'
) as typeof import('@node/lib/internal/fs/utils');

const { assignFunctionName } = __non_webpack_require__('internal/util') as typeof import('@node/lib/internal/util');

const { validateBoolean, validateFunction } = __non_webpack_require__(
  'internal/validators'
) as typeof import('@node/lib/internal/validators');

const { codes: errorCodes } = __non_webpack_require__('internal/errors') as typeof import('@node/lib/internal/errors');

// In the renderer node internals use the node global URL but we do not set that to be
// the global URL instance.  We need to do instanceof checks against the internal URL impl
const { URL: NodeURL } = __non_webpack_require__('internal/url') as typeof import('@node/lib/internal/url');

// Separate asar package's path from full path.
const splitPath = (archivePathOrBuffer: string | Buffer | URL) => {
  // Shortcut for disabled asar.
  if (isAsarDisabled()) return { isAsar: <const>false };

  // Check for a bad argument type.
  let archivePath = archivePathOrBuffer;
  if (Buffer.isBuffer(archivePathOrBuffer)) {
    archivePath = archivePathOrBuffer.toString();
  }
  if (archivePath instanceof NodeURL) {
    archivePath = getValidatedPath(archivePath);
  }
  if (typeof archivePath !== 'string') return { isAsar: <const>false };
  if (!asarRe.test(archivePath)) return { isAsar: <const>false };

  return asar.splitPath(path.normalize(archivePath));
};

// The on-disk location of an entry that was left outside of the archive
// (`asar.unpacked`) by the packager.  Mirrors what the native
// Archive::CopyFileOut() computes for unpacked entries, without taking the
// archive's external-files lock.
const getUnpackedPath = (asarPath: string, filePath: string) => path.join(`${asarPath}.unpacked`, filePath);

// Symbolic links inside an archive are stored with their target relative to
// the archive root.  Follows a chain of them (bounded, like the kernel's
// ELOOP limit) and returns the final entry and its stats, or null if any hop
// is missing or the chain is too long.
const kMaxSymlinkHops = 40;
function resolveAsarLinks(archive: NodeJS.AsarArchive, filePath: string) {
  let current = filePath;
  for (let hop = 0; hop < kMaxSymlinkHops; hop++) {
    const stats = archive.stat(current);
    if (!stats) return null;
    if (stats.type !== AsarFileType.kLink) return { filePath: current, stats };
    const target = archive.realpath(current);
    if (target === false) return null;
    current = target;
  }
  return null;
}

// Convert asar archive's Stats object to fs's Stats object.
let nextInode = 0;

const uid = process.getuid?.() ?? 0;
const gid = process.getgid?.() ?? 0;

const fakeTime = new Date();
const fakeTimeSec = Math.floor(fakeTime.getTime() / 1000);
const fakeTimeNsec = (fakeTime.getTime() % 1000) * 1e6;

enum AsarFileType {
  kFile = (constants as any).UV_DIRENT_FILE,
  kDirectory = (constants as any).UV_DIRENT_DIR,
  kLink = (constants as any).UV_DIRENT_LINK
}

const fileTypeToMode = new Map<AsarFileType, number>([
  [AsarFileType.kFile, constants.S_IFREG],
  [AsarFileType.kDirectory, constants.S_IFDIR],
  [AsarFileType.kLink, constants.S_IFLNK]
]);

// Literal permission bits rather than fs.constants.S_I*: the group/other and
// execute constants are not defined on Windows, and these fake modes should
// look the same on every platform.
const kReadOnlyMode = 0o644;
const kExecuteMode = 0o111;

// Builds the raw stat array that node's fs binding would have produced, so
// that node's own getStatsFromBinding() can turn it into a (BigInt)Stats.
// Permissions follow the usual defaults (0644 files, 0755 directories and
// executables, 0777 symlinks) so that copies of these entries made with the
// reported mode remain usable.
function makeStatArray(type: AsarFileType, size: number, ino: number, useBigint: boolean, executable = false) {
  let mode = kReadOnlyMode | fileTypeToMode.get(type)!;
  if (executable || type !== AsarFileType.kFile) mode |= kExecuteMode;
  if (type === AsarFileType.kLink) mode |= 0o022;
  const values = [
    1, // dev
    mode,
    1, // nlink
    uid,
    gid,
    0, // rdev
    4096, // blksize
    ino,
    size,
    Math.ceil(size / 512), // blocks (512-byte units)
    fakeTimeSec,
    fakeTimeNsec, // atime
    fakeTimeSec,
    fakeTimeNsec, // mtime
    fakeTimeSec,
    fakeTimeNsec, // ctime
    fakeTimeSec,
    fakeTimeNsec // birthtime
  ];
  return useBigint ? new BigInt64Array(values.map(BigInt)) : new Float64Array(values);
}

const asarStatsToFsStats = function (stats: NodeJS.AsarFileStat, options?: any) {
  const useBigint = Boolean(options && typeof options === 'object' && options.bigint);
  return getStatsFromBinding(makeStatArray(stats.type, stats.size, ++nextInode, useBigint, stats.executable));
};

const enum AsarError {
  NOT_FOUND = 'NOT_FOUND',
  NOT_DIR = 'NOT_DIR',
  IS_DIR = 'IS_DIR',
  NO_ACCESS = 'NO_ACCESS',
  INVALID_ARCHIVE = 'INVALID_ARCHIVE',
  NOT_LINK = 'NOT_LINK',
  EXISTS = 'EXISTS',
  NOT_SUPPORTED = 'NOT_SUPPORTED',
  TOO_MANY_OPEN = 'TOO_MANY_OPEN'
}

type AsarErrorObject = Error & { code?: string; errno?: number; syscall?: string; path?: string };

const createError = (
  errorType: AsarError,
  { asarPath, filePath, syscall }: { asarPath?: string; filePath?: string; syscall?: string } = {}
) => {
  let error: AsarErrorObject;
  switch (errorType) {
    case AsarError.NOT_FOUND:
      error = new Error(`ENOENT, ${filePath} not found in ${asarPath}`);
      error.code = 'ENOENT';
      error.errno = -2;
      break;
    case AsarError.NOT_DIR:
      error = new Error('ENOTDIR, not a directory');
      error.code = 'ENOTDIR';
      error.errno = -20;
      break;
    case AsarError.IS_DIR:
      error = new Error(`EISDIR: illegal operation on a directory, ${filePath} in ${asarPath}`);
      error.code = 'EISDIR';
      error.errno = -21;
      break;
    case AsarError.NO_ACCESS:
      error = new Error(`EACCES: permission denied, access '${filePath}'`);
      error.code = 'EACCES';
      error.errno = -13;
      break;
    case AsarError.INVALID_ARCHIVE:
      error = new Error(`Invalid package ${asarPath}`);
      break;
    case AsarError.NOT_LINK:
      error = new Error(`EINVAL: invalid argument, ${filePath} in ${asarPath} is not a symbolic link`);
      error.code = 'EINVAL';
      error.errno = -22;
      break;
    case AsarError.EXISTS:
      error = new Error(`EEXIST: file already exists, ${filePath}`);
      error.code = 'EEXIST';
      error.errno = -17;
      break;
    case AsarError.NOT_SUPPORTED:
      error = new Error(`ENOTSUP: operation not supported on asar archive entry ${filePath}`);
      error.code = 'ENOTSUP';
      error.errno = -45;
      break;
    case AsarError.TOO_MANY_OPEN:
      error = new Error(`EMFILE: too many open files, could not open ${filePath} in ${asarPath}`);
      error.code = 'EMFILE';
      error.errno = -24;
      break;
    default:
      throw new Error(`Invalid error type "${errorType}" passed to createError.`);
  }
  if (syscall) error.syscall = syscall;
  if (asarPath !== undefined && filePath !== undefined && errorType !== AsarError.INVALID_ARCHIVE) {
    error.path = path.join(asarPath, filePath);
  }
  return error;
};

// Delivers a result the way a native fs binding call would have: thrown /
// returned synchronously when there is no request object, as a promise when
// node passed `kUsePromises`, and via `req.oncomplete` for an FSReqCallback.
// `oncomplete` must be invoked as a method so that handlers which read
// `this.context` (e.g. readFile's internals) keep working.
function completeRequest<T>(req: any, error: Error | null, value?: T): any {
  if (req === undefined) {
    if (error) throw error;
    return value;
  }
  if (req === kUsePromises) {
    return error ? Promise.reject(error) : Promise.resolve(value);
  }
  process.nextTick(() => {
    if (error) req.oncomplete(error);
    else req.oncomplete(null, value);
  });
}

// Like completeRequest, but for a value that is produced asynchronously.
function completeRequestWithPromise<T>(req: any, promise: globalThis.Promise<T>): any {
  if (req === kUsePromises) return promise;
  promise.then(
    (value) => req.oncomplete(null, value),
    (error) => req.oncomplete(error)
  );
}

// A faithful copy of a native binding object (own properties, including
// symbols and non-enumerables, with their descriptors).
function cloneBinding<T extends object>(source: T): T {
  const clone = Object.create(Object.getPrototypeOf(source));
  for (const key of Reflect.ownKeys(source)) {
    Object.defineProperty(clone, key, Object.getOwnPropertyDescriptor(source, key)!);
  }
  return clone;
}

// `original-fs` is generated from the same sources as `fs` and would
// otherwise share these binding objects (and therefore the archive-aware
// overrides installed below).  Stash pristine copies for it to use instead;
// script/node/generate_original_fs.py points the generated modules here.
if (!Object.prototype.hasOwnProperty.call(binding, '_electronOriginalBindings')) {
  Object.defineProperty(binding, '_electronOriginalBindings', {
    value: Object.freeze({ fs: cloneBinding(binding), fs_dir: cloneBinding(dirBinding) }),
    enumerable: false,
    configurable: false,
    writable: false
  });
}

// The subset of the original fs binding we call into.  Captured before any of
// them are overridden below.
const originalBinding = {
  open: binding.open,
  openFileHandle: binding.openFileHandle,
  read: binding.read,
  readBuffers: binding.readBuffers,
  fstat: binding.fstat,
  close: binding.close,
  readFileUtf8: binding.readFileUtf8,
  writeFileUtf8: binding.writeFileUtf8,
  fchmod: binding.fchmod,
  fchown: binding.fchown,
  futimes: binding.futimes,
  copyFile: binding.copyFile,
  readlink: binding.readlink,
  writeBuffer: binding.writeBuffer,
  writeBuffers: binding.writeBuffers,
  writeString: binding.writeString,
  ftruncate: binding.ftruncate,
  cpSyncCheckPaths: binding.cpSyncCheckPaths,
  cpSyncOverrideFile: binding.cpSyncOverrideFile,
  cpSyncCopyDir: binding.cpSyncCopyDir
};
const originalDirBinding = {
  opendir: dirBinding.opendir,
  opendirSync: dirBinding.opendirSync
};

const overrideAPISync = function (
  module: Record<string, any>,
  name: string,
  pathArgumentIndex?: number | null,
  fromAsync: boolean = false
) {
  if (pathArgumentIndex == null) pathArgumentIndex = 0;
  const old = module[name];
  const func = function (this: any, ...args: any[]) {
    const pathArgument = args[pathArgumentIndex!];
    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) return old.apply(this, args);
    const { asarPath, filePath } = pathInfo;

    const archive = getOrCreateArchive(asarPath);
    if (!archive) throw createError(AsarError.INVALID_ARCHIVE, { asarPath });

    const newPath = archive.copyFileOut(filePath);
    if (!newPath) throw createError(AsarError.NOT_FOUND, { asarPath, filePath });

    args[pathArgumentIndex!] = newPath;
    return old.apply(this, args);
  };
  if (fromAsync) {
    return func;
  }
  module[name] = func;
};

const overrideAPI = function (module: Record<string, any>, name: string, pathArgumentIndex?: number | null) {
  if (pathArgumentIndex == null) pathArgumentIndex = 0;
  const old = module[name];
  module[name] = function (this: any, ...args: any[]) {
    const pathArgument = args[pathArgumentIndex!];
    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) return old.apply(this, args);
    const { asarPath, filePath } = pathInfo;

    const callback = args[args.length - 1];
    if (typeof callback !== 'function') {
      return overrideAPISync(module, name, pathArgumentIndex!, true)!.apply(this, args);
    }

    const archive = getOrCreateArchive(asarPath);
    if (!archive) {
      const error = createError(AsarError.INVALID_ARCHIVE, { asarPath });
      nextTick(callback, [error]);
      return;
    }

    const newPath = archive.copyFileOut(filePath);
    if (!newPath) {
      const error = createError(AsarError.NOT_FOUND, { asarPath, filePath });
      nextTick(callback, [error]);
      return;
    }

    args[pathArgumentIndex!] = newPath;
    return old.apply(this, args);
  };

  if (old[util.promisify.custom]) {
    module[name][util.promisify.custom] = assignFunctionName(
      name,
      makePromiseFunction(old[util.promisify.custom], pathArgumentIndex)
    );
  }

  if (module.promises && module.promises[name]) {
    module.promises[name] = makePromiseFunction(module.promises[name], pathArgumentIndex);
  }
};

let crypto: typeof Crypto;
// Delay load crypto to improve app boot performance
// when integrity protection is not enabled
const getCrypto = () => {
  crypto = crypto || require('crypto');
  return crypto;
};

// Terminates the process on an integrity failure.  This must be synchronous
// and must never return to the caller: in the main process `process.exit()`
// is mapped to the graceful, asynchronous `app.exit()`, which would let JS
// keep running (and consume the tampered bytes) until the quit lands.  The
// message is written with a blocking write so it is not lost on the way out.
const integrityViolation = (actual: string, expected: string): never => {
  const message = `ASAR Integrity Violation: got a hash mismatch (${actual} vs ${expected})\n`;
  try {
    (require('fs') as typeof import('fs')).writeSync(2, message);
  } catch {
    console.error(message);
  }
  const reallyExit = (process as any).reallyExit;
  if (typeof reallyExit === 'function') reallyExit(1);
  process.exit(1);
  // Neither call above returns; keep TypeScript (and any monkey-patched
  // process.exit) honest by never continuing.
  throw new Error('ASAR Integrity Violation');
};

const sha256HexSync = (buffer: Uint8Array) => getCrypto().createHash('sha256').update(buffer).digest('hex');

// Hashes off the JS thread; used by the asynchronous read paths so that
// validating a block never stalls the event loop.
const sha256HexAsync = async (buffer: Uint8Array) => {
  const digest = await getCrypto().webcrypto.subtle.digest('SHA-256', buffer);
  return Buffer.from(digest).toString('hex');
};

function validateBufferIntegrity(buffer: Buffer, integrity: NodeJS.AsarFileInfo['integrity']) {
  if (!integrity) return;
  const actual = sha256HexSync(buffer);
  if (actual !== integrity.hash.toLowerCase()) integrityViolation(actual, integrity.hash);
}

async function validateBufferIntegrityAsync(buffer: Buffer, integrity: NodeJS.AsarFileInfo['integrity']) {
  if (!integrity) return;
  const actual = await sha256HexAsync(buffer);
  if (actual !== integrity.hash.toLowerCase()) integrityViolation(actual, integrity.hash);
}

const makePromiseFunction = function (orig: Function, pathArgumentIndex: number) {
  return function (this: any, ...args: any[]) {
    const pathArgument = args[pathArgumentIndex];
    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) return orig.apply(this, args);
    const { asarPath, filePath } = pathInfo;

    const archive = getOrCreateArchive(asarPath);
    if (!archive) {
      return Promise.reject(createError(AsarError.INVALID_ARCHIVE, { asarPath }));
    }

    const newPath = archive.copyFileOut(filePath);
    if (!newPath) {
      return Promise.reject(createError(AsarError.NOT_FOUND, { asarPath, filePath }));
    }

    args[pathArgumentIndex] = newPath;
    return orig.apply(this, args);
  };
};

//
// Direct, integrity-validated reads of packed entries.
//
// An AsarEntryReader serves reads of a single packed entry straight from the
// archive's retained file handle: positions are translated to archive offsets
// and clamped to the entry.  The descriptor number the caller gets back from
// open() is a separate sentinel (a write-only handle on the null device) that
// only identifies the reader within this module: reading it directly fails
// with EBADF, so code that bypasses fs fails loudly rather than being handed
// archive bytes, and no handle to the archive itself is ever exposed.
// When the archive carries integrity information, bytes are only ever handed
// out from a buffer whose block hash has just been verified - the reader
// keeps the most recently validated block so that sequential consumers such
// as streams pay for one read and one hash per block, and never re-reads a
// block from disk without re-validating it.
//

interface ValidatedBlock {
  index: number;
  buffer: Buffer;
}

// Upper bound on the number of validated blocks retained across all live
// readers, so that many concurrently open handles cannot pin unbounded
// memory. Past this, readers still validate every block they serve, they
// just do not keep it around for the next read.
const kMaxRetainedBlocks = 32;
let retainedBlockCount = 0;

// Reads exactly `length` bytes at `position` from `fd`, looping over short
// reads. Returns the number of bytes actually read (less than `length` only
// at end of file).
function readFullySync(fd: number, buffer: Uint8Array, offset: number, length: number, position: number) {
  let total = 0;
  while (total < length) {
    const n = originalBinding.read(fd, buffer, offset + total, length - total, position + total);
    if (n === 0) break;
    total += n;
  }
  return total;
}

async function readFully(fd: number, buffer: Uint8Array, offset: number, length: number, position: number) {
  let total = 0;
  while (total < length) {
    const n =
      (await originalBinding.read(fd, buffer, offset + total, length - total, position + total, kUsePromises)) || 0;
    if (n === 0) break;
    total += n;
  }
  return total;
}

class AsarEntryReader {
  // The sentinel descriptor handed to the caller; owned by whoever owns the
  // open (fs.close / FileHandle), never read or written by this module.
  readonly fd: number;
  // The archive's retained handle; all reads go through it, positionally.
  readonly archiveFd: number;
  readonly size: number;
  readonly offset: number;
  readonly executable: boolean;
  readonly ino: number;
  // Position used by reads that do not specify one (like a kernel file
  // position).  Advanced when such a read is issued.
  position = 0;

  readonly integrity: NonNullable<NodeJS.AsarFileInfo['integrity']> | null;
  private validatedBlock: ValidatedBlock | null = null;
  private closed = false;

  constructor(fd: number, archiveFd: number, info: NodeJS.AsarFileInfo) {
    this.fd = fd;
    this.archiveFd = archiveFd;
    this.size = info.size;
    this.offset = info.offset;
    this.executable = info.executable;
    this.ino = ++nextInode;
    this.integrity = info.integrity && info.integrity.blockSize > 0 ? info.integrity : null;
  }

  statArray(useBigint: boolean) {
    return makeStatArray(AsarFileType.kFile, this.size, this.ino, useBigint, this.executable);
  }

  // Resolves the (position, length) of a read: `position < 0` means "use and
  // advance the reader's own position".
  resolveRange(length: number, position: number) {
    const start = position < 0 ? this.position : position;
    const available = Math.max(0, this.size - start);
    const count = Math.min(length, available);
    if (position < 0) this.position = start + count;
    return { start, count };
  }

  private retainBlock(block: ValidatedBlock) {
    if (this.validatedBlock !== null) {
      this.validatedBlock = null;
      retainedBlockCount--;
    }
    // A validation that completes after the reader was closed must not pin
    // anything.
    if (!this.closed && retainedBlockCount < kMaxRetainedBlocks) {
      this.validatedBlock = block;
      retainedBlockCount++;
    }
  }

  private blockRange(index: number) {
    const start = index * this.integrity!.blockSize;
    return { start, length: Math.min(this.integrity!.blockSize, this.size - start) };
  }

  private checkBlock(index: number, buffer: Buffer, actual: string) {
    const expected = this.integrity!.blocks[index];
    if (typeof expected !== 'string' || actual !== expected.toLowerCase()) {
      integrityViolation(actual, String(expected));
    }
    const block = { index, buffer };
    this.retainBlock(block);
    return block;
  }

  private validateBlockSync(index: number): ValidatedBlock {
    const { start, length } = this.blockRange(index);
    const buffer = Buffer.allocUnsafeSlow(length);
    readFullySync(this.archiveFd, buffer, 0, length, this.offset + start);
    return this.checkBlock(index, buffer, sha256HexSync(buffer));
  }

  private async validateBlock(index: number): Promise<ValidatedBlock> {
    const { start, length } = this.blockRange(index);
    const buffer = Buffer.allocUnsafeSlow(length);
    await readFully(this.archiveFd, buffer, 0, length, this.offset + start);
    return this.checkBlock(index, buffer, await sha256HexAsync(buffer));
  }

  // Copies `count` bytes starting at entry offset `start` out of validated
  // blocks into `buffer`.
  private copyFromBlocks(
    buffer: Uint8Array,
    bufferOffset: number,
    start: number,
    count: number,
    getBlock: (i: number) => ValidatedBlock
  ) {
    const { blockSize } = this.integrity!;
    let done = 0;
    while (done < count) {
      const current = start + done;
      const index = Math.floor(current / blockSize);
      const block = this.validatedBlock?.index === index ? this.validatedBlock : getBlock(index);
      const inBlock = current - index * blockSize;
      const take = Math.min(count - done, block.buffer.length - inBlock);
      block.buffer.copy(buffer, bufferOffset + done, inBlock, inBlock + take);
      done += take;
    }
    return count;
  }

  readSync(buffer: Uint8Array, bufferOffset: number, length: number, position: number): number {
    const { start, count } = this.resolveRange(length, position);
    if (count === 0) return 0;
    if (!this.integrity) return readFullySync(this.archiveFd, buffer, bufferOffset, count, this.offset + start);
    return this.copyFromBlocks(buffer, bufferOffset, start, count, (i) => this.validateBlockSync(i));
  }

  async read(buffer: Uint8Array, bufferOffset: number, length: number, position: number): Promise<number> {
    const { start, count } = this.resolveRange(length, position);
    if (count === 0) return 0;
    if (!this.integrity) return readFully(this.archiveFd, buffer, bufferOffset, count, this.offset + start);

    // Validate every block the read touches up front (asynchronously), then
    // copy out synchronously.
    const { blockSize } = this.integrity;
    const first = Math.floor(start / blockSize);
    const last = Math.floor((start + count - 1) / blockSize);
    const blocks = new Map<number, ValidatedBlock>();
    for (let index = first; index <= last; index++) {
      blocks.set(index, this.validatedBlock?.index === index ? this.validatedBlock : await this.validateBlock(index));
    }
    return this.copyFromBlocks(buffer, bufferOffset, start, count, (i) => blocks.get(i)!);
  }

  // Reads from the current position to the end of the entry.
  readToEndSync() {
    const buffer = Buffer.allocUnsafeSlow(Math.max(0, this.size - this.position));
    const n = this.readSync(buffer, 0, buffer.length, -1);
    return n === buffer.length ? buffer : buffer.subarray(0, n);
  }

  // Releases retained validation state; does not close the fd.
  dispose() {
    if (this.closed) return;
    this.closed = true;
    if (this.validatedBlock !== null) {
      this.validatedBlock = null;
      retainedBlockCount--;
    }
  }
}

// Every fd handed out by open()/openSync()/promises.open() for a packed
// entry, keyed by fd number.  For FileHandles the fd is owned by node's C++
// FileHandle (which also closes it on GC), so the entry additionally holds a
// weak reference to that handle and is only considered live while the handle
// is alive and still owns that fd number - a stale entry can therefore never
// capture reads of an unrelated file that later reuses the number.
interface OpenAsarFile {
  reader: AsarEntryReader;
  handle?: WeakRef<{ fd: number }>;
}
const openAsarFiles = new Map<number, OpenAsarFile>();

function lookupAsarFd(fd: number): AsarEntryReader | undefined {
  if (openAsarFiles.size === 0 || typeof fd !== 'number') return undefined;
  const entry = openAsarFiles.get(fd);
  if (entry === undefined) return undefined;
  if (entry.handle !== undefined) {
    const handle = entry.handle.deref();
    if (handle === undefined || handle.fd !== fd) {
      openAsarFiles.delete(fd);
      entry.reader.dispose();
      return undefined;
    }
  }
  return entry.reader;
}

function forgetAsarFd(fd: number) {
  const entry = openAsarFiles.get(fd);
  if (entry === undefined) return;
  openAsarFiles.delete(fd);
  entry.reader.dispose();
}

// A descriptor number handed out (or handed back) by native fs may still have
// an entry here if the previous owner of that number was closed behind our
// back (a native FileHandle built on it, e.g. by http2's respondWithFile, a
// FileHandle transferred to a worker, or one dropped and closed on GC).  Such
// an entry must never capture the new owner's reads.
function forgetStaleAsarFd(fd: unknown) {
  if (openAsarFiles.size !== 0 && typeof fd === 'number') forgetAsarFd(fd);
}

// Calls a native open-like binding (args[3] being its request) and drops any
// stale entry for the descriptor it produces.  The check is only wired up
// while entries exist, so the common case stays a plain pass-through.
function passThroughOpen(fn: Function, args: any[], getFd: (value: any) => unknown) {
  if (openAsarFiles.size === 0) return fn.apply(undefined, args);
  const req = args[3];
  if (req === undefined) {
    const fd = fn.apply(undefined, args);
    forgetStaleAsarFd(getFd(fd));
    return fd;
  }
  if (req === kUsePromises) {
    return fn.apply(undefined, args).then((value: any) => {
      forgetStaleAsarFd(getFd(value));
      return value;
    });
  }
  const oncomplete = req.oncomplete;
  req.oncomplete = function (this: any, error: any, value: any) {
    if (!error) forgetStaleAsarFd(getFd(value));
    return oncomplete.call(this, error, value);
  };
  return fn.apply(undefined, args);
}

function sweepStaleAsarFds() {
  for (const [fd, entry] of openAsarFiles) {
    if (entry.handle === undefined) continue;
    const handle = entry.handle.deref();
    if (handle === undefined || handle.fd !== fd) forgetAsarFd(fd);
  }
}

// Applies an fs `encoding` option to a name the way the native bindings do:
// 'buffer' yields a Buffer, other encodings re-encode the UTF-8 name.
const encodeName = (name: string, encoding: any) => {
  if (encoding === 'buffer') return Buffer.from(name);
  if (encoding && encoding !== 'utf8' && encoding !== 'utf-8') return Buffer.from(name).toString(encoding);
  return name;
};

// The fs binding accepts a position as a number, a bigint, or null /
// undefined / -1 for "the current file position".
const normalizePosition = (position: number | bigint | null | undefined) => {
  if (position == null) return -1;
  if (typeof position === 'bigint') return Number(position);
  return position;
};

// Node's fs.constants values for the "opens for writing" bits.  Any of these
// on a packed entry is refused: archives are read-only, and silently
// redirecting the write elsewhere would be worse than failing.
const kWriteFlags = constants.O_WRONLY | constants.O_RDWR | constants.O_APPEND | constants.O_TRUNC;

// Resolves an asar path for open()-like calls.  Returns either a reader for
// a packed entry, the real path of an unpacked entry, or throws.
// `withSentinel: false` is for internal consumers (copyFile) that read the
// entry themselves and never hand a descriptor number to anyone; the reader's
// `fd` is -1 then.
function openAsarEntry(
  asarPath: string,
  filePath: string,
  flags: number,
  syscall: string,
  withSentinel = true
): { unpackedPath: string } | { reader: AsarEntryReader } {
  const archive = getOrCreateArchive(asarPath);
  if (!archive) throw createError(AsarError.INVALID_ARCHIVE, { asarPath, syscall });

  const info = archive.getFileInfo(filePath);
  if (!info) {
    // getFileInfo follows links itself, so a link to a directory ends up here
    // as well; resolve links to tell EISDIR from ENOENT.
    const resolved = resolveAsarLinks(archive, filePath);
    if (resolved && resolved.stats.type === AsarFileType.kDirectory) {
      throw createError(AsarError.IS_DIR, { asarPath, filePath, syscall });
    }
    throw createError(AsarError.NOT_FOUND, { asarPath, filePath, syscall });
  }

  if (info.unpacked) return { unpackedPath: getUnpackedPath(asarPath, filePath) };

  if (flags & kWriteFlags) throw createError(AsarError.NO_ACCESS, { asarPath, filePath, syscall });
  if ((flags & constants.O_CREAT) !== 0 && (flags & constants.O_EXCL) !== 0) {
    throw createError(AsarError.EXISTS, { asarPath, filePath, syscall });
  }

  const archiveFd = archive.getFdAndValidateIntegrityLater();
  if (!(archiveFd >= 0)) throw createError(AsarError.INVALID_ARCHIVE, { asarPath, syscall });
  if (!withSentinel) return { reader: new AsarEntryReader(-1, archiveFd, info) };
  const fd = asar.createSentinelFd();
  if (!(fd >= 0)) throw createError(AsarError.TOO_MANY_OPEN, { asarPath, filePath, syscall });
  return { reader: new AsarEntryReader(fd, archiveFd, info) };
}

// Override fs APIs.
export const wrapFsWithAsar = (fs: Record<string, any>) => {
  const logFDs = new Map<string, number>();
  const logASARAccess = (asarPath: string, filePath: string, offset: number) => {
    if (!process.env.ELECTRON_LOG_ASAR_READS) return;
    if (!logFDs.has(asarPath)) {
      const logFilename = `${path.basename(asarPath, '.asar')}-access-log.txt`;
      const logPath = path.join((require('os') as typeof os).tmpdir(), logFilename);
      logFDs.set(asarPath, fs.openSync(logPath, 'a'));
    }
    fs.writeSync(logFDs.get(asarPath), `${offset}: ${filePath}\n`);
  };

  const shouldThrowStatError = (options: any) => {
    if (options && typeof options === 'object' && options.throwIfNoEntry === false) {
      return false;
    }

    return true;
  };

  // internalModuleStat-shaped check (1 = directory, 0 = file, negative =
  // missing) used by the recursive readdir implementations below.  Unlike
  // internalModuleStat itself, symbolic links inside archives are not
  // followed: recursive readdir descends into whatever this reports as a
  // directory, and archives may legitimately contain link cycles (a real
  // filesystem walk would only be stopped by ENAMETOOLONG).
  const statTypeForReaddir = (pathArgument: string): number => {
    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) return internalModuleStat(pathArgument);
    const archive = getOrCreateArchive(pathInfo.asarPath);
    if (!archive) return -34;
    const stats = archive.stat(pathInfo.filePath);
    if (!stats) return -34;
    return stats.type === AsarFileType.kDirectory ? 1 : 0;
  };

  const { lstatSync } = fs;
  fs.lstatSync = (pathArgument: string, options: any) => {
    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) return lstatSync(pathArgument, options);
    const { asarPath, filePath } = pathInfo;

    const archive = getOrCreateArchive(asarPath);
    if (!archive) {
      if (shouldThrowStatError(options)) {
        throw createError(AsarError.INVALID_ARCHIVE, { asarPath });
      }
      return null;
    }

    const stats = archive.stat(filePath);
    if (!stats) {
      if (shouldThrowStatError(options)) {
        throw createError(AsarError.NOT_FOUND, { asarPath, filePath });
      }
      return null;
    }

    return asarStatsToFsStats(stats, options);
  };

  const { lstat } = fs;
  fs.lstat = (pathArgument: string, options: any, callback: any) => {
    const pathInfo = splitPath(pathArgument);
    if (typeof options === 'function') {
      callback = options;
      options = {};
    }
    if (!pathInfo.isAsar) return lstat(pathArgument, options, callback);
    const { asarPath, filePath } = pathInfo;

    const archive = getOrCreateArchive(asarPath);
    if (!archive) {
      const error = createError(AsarError.INVALID_ARCHIVE, { asarPath });
      nextTick(callback, [error]);
      return;
    }

    const stats = archive.stat(filePath);
    if (!stats) {
      const error = createError(AsarError.NOT_FOUND, { asarPath, filePath });
      nextTick(callback, [error]);
      return;
    }

    const fsStats = asarStatsToFsStats(stats, options);
    nextTick(callback, [null, fsStats]);
  };

  fs.promises.lstat = util.promisify(fs.lstat);

  const { statSync } = fs;
  fs.statSync = (pathArgument: string, options: any) => {
    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) return statSync(pathArgument, options);
    const { asarPath, filePath } = pathInfo;

    const archive = getOrCreateArchive(asarPath);
    if (!archive) {
      if (shouldThrowStatError(options)) {
        throw createError(AsarError.INVALID_ARCHIVE, { asarPath });
      }
      return null;
    }

    const resolved = resolveAsarLinks(archive, filePath);
    if (!resolved) {
      if (shouldThrowStatError(options)) {
        throw createError(AsarError.NOT_FOUND, { asarPath, filePath });
      }
      return null;
    }

    return asarStatsToFsStats(resolved.stats, options);
  };

  const { stat } = fs;
  fs.stat = (pathArgument: string, options: any, callback: any) => {
    const pathInfo = splitPath(pathArgument);
    if (typeof options === 'function') {
      callback = options;
      options = {};
    }
    if (!pathInfo.isAsar) return stat(pathArgument, options, callback);
    const { asarPath, filePath } = pathInfo;

    const archive = getOrCreateArchive(asarPath);
    if (!archive) {
      nextTick(callback, [createError(AsarError.INVALID_ARCHIVE, { asarPath })]);
      return;
    }

    const resolved = resolveAsarLinks(archive, filePath);
    if (!resolved) {
      nextTick(callback, [createError(AsarError.NOT_FOUND, { asarPath, filePath })]);
      return;
    }

    nextTick(callback, [null, asarStatsToFsStats(resolved.stats, options)]);
  };

  fs.promises.stat = util.promisify(fs.stat);

  const wrapRealpathSync = function (realpathSync: Function) {
    return function (this: any, pathArgument: string, options: any) {
      const pathInfo = splitPath(pathArgument);
      if (!pathInfo.isAsar) return realpathSync.apply(this, arguments);
      const { asarPath, filePath } = pathInfo;

      const archive = getOrCreateArchive(asarPath);
      if (!archive) {
        throw createError(AsarError.INVALID_ARCHIVE, { asarPath });
      }

      const fileRealPath = archive.realpath(filePath);
      if (fileRealPath === false) {
        throw createError(AsarError.NOT_FOUND, { asarPath, filePath });
      }

      return path.join(realpathSync(asarPath, options), fileRealPath);
    };
  };

  const { realpathSync } = fs;
  fs.realpathSync = wrapRealpathSync(realpathSync);
  fs.realpathSync.native = wrapRealpathSync(realpathSync.native);

  const wrapRealpath = function (realpath: Function) {
    return function (this: any, pathArgument: string, options: any, callback: any) {
      const pathInfo = splitPath(pathArgument);
      if (!pathInfo.isAsar) return realpath.apply(this, arguments);
      const { asarPath, filePath } = pathInfo;

      if (arguments.length < 3) {
        callback = options;
        options = {};
      }

      const archive = getOrCreateArchive(asarPath);
      if (!archive) {
        const error = createError(AsarError.INVALID_ARCHIVE, { asarPath });
        nextTick(callback, [error]);
        return;
      }

      const fileRealPath = archive.realpath(filePath);
      if (fileRealPath === false) {
        const error = createError(AsarError.NOT_FOUND, { asarPath, filePath });
        nextTick(callback, [error]);
        return;
      }

      realpath(asarPath, options, (error: Error | null, archiveRealPath: string) => {
        if (error === null) {
          const fullPath = path.join(archiveRealPath, fileRealPath);
          callback(null, fullPath);
        } else {
          callback(error);
        }
      });
    };
  };

  const { realpath } = fs;
  fs.realpath = wrapRealpath(realpath);
  fs.realpath.native = wrapRealpath(realpath.native);

  fs.promises.realpath = util.promisify(fs.realpath.native);

  const { exists: nativeExists } = fs;
  fs.exists = function exists(pathArgument: string, callback: any) {
    let pathInfo: ReturnType<typeof splitPath>;
    try {
      pathInfo = splitPath(pathArgument);
    } catch {
      nextTick(callback, [false]);
      return;
    }
    if (!pathInfo.isAsar) return nativeExists(pathArgument, callback);
    const { asarPath, filePath } = pathInfo;

    const archive = getOrCreateArchive(asarPath);
    if (!archive) {
      nextTick(callback, [false]);
      return;
    }

    const pathExists = archive.stat(filePath) !== false;
    nextTick(callback, [pathExists]);
  };

  fs.exists[util.promisify.custom] = function exists(pathArgument: string) {
    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) return nativeExists[util.promisify.custom](pathArgument);
    const { asarPath, filePath } = pathInfo;

    const archive = getOrCreateArchive(asarPath);
    if (!archive) {
      return Promise.resolve(false);
    }

    return Promise.resolve(archive.stat(filePath) !== false);
  };

  const { existsSync } = fs;
  fs.existsSync = (pathArgument: string) => {
    let pathInfo: ReturnType<typeof splitPath>;
    try {
      pathInfo = splitPath(pathArgument);
    } catch {
      return false;
    }
    if (!pathInfo.isAsar) return existsSync(pathArgument);
    const { asarPath, filePath } = pathInfo;

    const archive = getOrCreateArchive(asarPath);
    if (!archive) return false;

    return archive.stat(filePath) !== false;
  };

  const { access } = fs;
  fs.access = function (pathArgument: string, mode: number, callback: any) {
    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) return access.apply(this, arguments);
    const { asarPath, filePath } = pathInfo;

    if (typeof mode === 'function') {
      callback = mode;
      mode = fs.constants.F_OK;
    }

    const archive = getOrCreateArchive(asarPath);
    if (!archive) {
      const error = createError(AsarError.INVALID_ARCHIVE, { asarPath });
      nextTick(callback, [error]);
      return;
    }

    const info = archive.getFileInfo(filePath);
    if (!info) {
      const error = createError(AsarError.NOT_FOUND, { asarPath, filePath });
      nextTick(callback, [error]);
      return;
    }

    if (info.unpacked) {
      return fs.access(getUnpackedPath(asarPath, filePath), mode, callback);
    }

    const stats = archive.stat(filePath);
    if (!stats) {
      const error = createError(AsarError.NOT_FOUND, { asarPath, filePath });
      nextTick(callback, [error]);
      return;
    }

    if (mode & fs.constants.W_OK) {
      const error = createError(AsarError.NO_ACCESS, { asarPath, filePath });
      nextTick(callback, [error]);
      return;
    }

    nextTick(callback);
  };

  const { access: accessPromise } = fs.promises;
  const promisifiedAccess = util.promisify(fs.access);
  fs.promises.access = function (pathArgument: string, mode: number) {
    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) {
      return accessPromise.apply(this, arguments);
    }

    return promisifiedAccess(pathArgument, mode);
  };

  const { accessSync } = fs;
  fs.accessSync = function (pathArgument: string, mode: any) {
    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) return accessSync.apply(this, arguments);
    const { asarPath, filePath } = pathInfo;

    if (mode == null) mode = fs.constants.F_OK;

    const archive = getOrCreateArchive(asarPath);
    if (!archive) {
      throw createError(AsarError.INVALID_ARCHIVE, { asarPath });
    }

    const info = archive.getFileInfo(filePath);
    if (!info) {
      throw createError(AsarError.NOT_FOUND, { asarPath, filePath });
    }

    if (info.unpacked) {
      return fs.accessSync(getUnpackedPath(asarPath, filePath), mode);
    }

    const stats = archive.stat(filePath);
    if (!stats) {
      throw createError(AsarError.NOT_FOUND, { asarPath, filePath });
    }

    if (mode & fs.constants.W_OK) {
      throw createError(AsarError.NO_ACCESS, { asarPath, filePath });
    }
  };

  function normalizeReadFileOptions(options: any) {
    if (typeof options === 'string') {
      options = { encoding: options };
    } else if (options === null || options === undefined) {
      options = { encoding: null };
    } else if (typeof options !== 'object') {
      throw new TypeError('Bad arguments');
    }
    return options;
  }

  function fsReadFileAsar(pathArgument: string, options: any, callback: any) {
    const pathInfo = splitPath(pathArgument);
    if (pathInfo.isAsar) {
      const { asarPath, filePath } = pathInfo;

      if (typeof options === 'function') {
        callback = options;
        options = { encoding: null };
      } else {
        options = normalizeReadFileOptions(options);
      }

      const { encoding } = options;
      const archive = getOrCreateArchive(asarPath);
      if (!archive) {
        const error = createError(AsarError.INVALID_ARCHIVE, { asarPath });
        nextTick(callback, [error]);
        return;
      }

      const info = archive.getFileInfo(filePath);
      if (!info) {
        const error = createError(AsarError.NOT_FOUND, { asarPath, filePath });
        nextTick(callback, [error]);
        return;
      }

      if (info.size === 0) {
        nextTick(callback, [null, encoding ? '' : Buffer.alloc(0)]);
        return;
      }

      if (info.unpacked) {
        return fs.readFile(getUnpackedPath(asarPath, filePath), options, callback);
      }

      const fd = archive.getFdAndValidateIntegrityLater();
      if (!(fd >= 0)) {
        const error = createError(AsarError.NOT_FOUND, { asarPath, filePath });
        nextTick(callback, [error]);
        return;
      }

      logASARAccess(asarPath, filePath, info.offset);
      const buffer = Buffer.allocUnsafeSlow(info.size);
      readFully(fd, buffer, 0, info.size, info.offset)
        .then(async (bytesRead) => {
          // Only what was actually read gets validated and returned; a
          // truncated archive must not surface as a spurious hash mismatch.
          const data = bytesRead === info.size ? buffer : buffer.subarray(0, bytesRead);
          await validateBufferIntegrityAsync(data, info.integrity);
          return data;
        })
        .then(
          (data) => callback(null, encoding ? data.toString(encoding) : data),
          (error) => callback(error)
        );
    }
  }

  const { readFile } = fs;
  fs.readFile = function (pathArgument: string, options: any, callback: any) {
    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) {
      return readFile.apply(this, arguments);
    }

    return fsReadFileAsar(pathArgument, options, callback);
  };

  const { readFile: readFilePromise } = fs.promises;
  const promisifiedReadFileAsar = util.promisify(fsReadFileAsar);
  fs.promises.readFile = function (pathArgument: string, options: any) {
    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) {
      return readFilePromise.apply(this, arguments);
    }

    return promisifiedReadFileAsar(pathArgument, options);
  };

  function readFileFromArchiveSync(
    pathInfo: { asarPath: string; filePath: string },
    options: any
  ): ReturnType<typeof readFileSync> {
    const { asarPath, filePath } = pathInfo;

    const archive = getOrCreateArchive(asarPath);
    if (!archive) throw createError(AsarError.INVALID_ARCHIVE, { asarPath });

    const info = archive.getFileInfo(filePath);
    if (!info) throw createError(AsarError.NOT_FOUND, { asarPath, filePath });

    options = normalizeReadFileOptions(options);
    const { encoding } = options;

    if (info.size === 0) return encoding ? '' : Buffer.alloc(0);
    if (info.unpacked) {
      return fs.readFileSync(getUnpackedPath(asarPath, filePath), options);
    }

    const fd = archive.getFdAndValidateIntegrityLater();
    if (!(fd >= 0)) {
      throw createError(AsarError.NOT_FOUND, { asarPath, filePath });
    }

    logASARAccess(asarPath, filePath, info.offset);
    let buffer = Buffer.allocUnsafeSlow(info.size);
    const bytesRead = readFullySync(fd, buffer, 0, info.size, info.offset);
    if (bytesRead !== info.size) buffer = buffer.subarray(0, bytesRead);
    validateBufferIntegrity(buffer, info.integrity);
    return encoding ? buffer.toString(encoding) : buffer;
  }

  const { readFileSync } = fs;
  fs.readFileSync = function (pathArgument: string, options: any) {
    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) return readFileSync.apply(this, arguments);

    return readFileFromArchiveSync(pathInfo, options);
  };

  type ReaddirOptions =
    | { encoding: BufferEncoding | null; withFileTypes?: false; recursive?: false }
    | undefined
    | null;
  type ReaddirCallback = (err: NodeJS.ErrnoException | null, files?: string[]) => void;

  const processReaddirResult = (args: any) =>
    args.context.withFileTypes ? handleDirents(args) : handleFilePaths(args);

  function handleDirents({ result, currentPath, context }: { result: any[]; currentPath: string; context: any }) {
    const length = result[0].length;
    for (let i = 0; i < length; i++) {
      const resultPath = path.join(currentPath, result[0][i]);
      const info = splitPath(resultPath);

      let type = result[1][i];
      if (info.isAsar) {
        const archive = getOrCreateArchive(info.asarPath);
        if (!archive) continue;
        const stats = archive.stat(info.filePath);
        if (!stats) continue;
        type = stats.type;
      }

      const dirent = getDirent(currentPath, result[0][i], type);
      const stat = statTypeForReaddir(resultPath);

      context.readdirResults.push(dirent);
      if (dirent!.isDirectory() || stat === 1) {
        context.pathsQueue.push(path.join(dirent!.parentPath, dirent!.name));
      }
    }
  }

  function handleFilePaths({ result, currentPath, context }: { result: string[]; currentPath: string; context: any }) {
    for (let i = 0; i < result.length; i++) {
      const resultPath = path.join(currentPath, result[i]);
      const relativeResultPath = path.relative(context.basePath, resultPath);
      const stat = statTypeForReaddir(resultPath);
      context.readdirResults.push(relativeResultPath);

      if (stat === 1) {
        context.pathsQueue.push(resultPath);
      }
    }
  }

  function readdirRecursive(basePath: string, options: ReaddirOptions, callback: ReaddirCallback) {
    const context = {
      withFileTypes: Boolean(options!.withFileTypes),
      encoding: options!.encoding,
      basePath,
      readdirResults: [],
      pathsQueue: [basePath]
    };

    let i = 0;

    function read(pathArg: string) {
      const req = new binding.FSReqCallback();
      req.oncomplete = (err: any, result: string) => {
        if (err) {
          callback(err);
          return;
        }

        if (result === undefined) {
          callback(null, context.readdirResults);
          return;
        }

        processReaddirResult({
          result,
          currentPath: pathArg,
          context
        });

        if (i < context.pathsQueue.length) {
          read(context.pathsQueue[i++]);
        } else {
          callback(null, context.readdirResults);
        }
      };

      const pathInfo = splitPath(pathArg);
      if (pathInfo.isAsar) {
        let readdirResult;
        const { asarPath, filePath } = pathInfo;

        const archive = getOrCreateArchive(asarPath);
        if (!archive) {
          const error = createError(AsarError.INVALID_ARCHIVE, { asarPath });
          nextTick(callback, [error]);
          return;
        }

        readdirResult = archive.readdir(filePath);
        if (!readdirResult) {
          const error = createError(AsarError.NOT_FOUND, { asarPath, filePath });
          nextTick(callback, [error]);
          return;
        }

        // If we're in an asar dir, we need to ensure the result is in the same format as the
        // native call to readdir withFileTypes i.e. an array of arrays.
        if (context.withFileTypes) {
          readdirResult = [
            [...readdirResult],
            readdirResult.map((p: string) => {
              return statTypeForReaddir(path.join(pathArg, p));
            })
          ];
        }

        processReaddirResult({
          result: readdirResult,
          currentPath: pathArg,
          context
        });

        if (i < context.pathsQueue.length) {
          read(context.pathsQueue[i++]);
        } else {
          callback(null, context.readdirResults);
        }
      } else {
        binding.readdir(pathArg, context.encoding, context.withFileTypes, req);
      }
    }

    read(context.pathsQueue[i++]);
  }

  const { readdir } = fs;
  fs.readdir = function (pathArgument: string, options: ReaddirOptions, callback: ReaddirCallback) {
    callback = typeof options === 'function' ? options : callback;
    validateFunction(callback, 'callback')!;

    options = getOptions(options);
    pathArgument = getValidatedPath(pathArgument);

    if (options?.recursive != null) {
      validateBoolean(options?.recursive, 'options.recursive')!;
    }

    if (options?.recursive) {
      readdirRecursive(pathArgument, options, callback);
      return;
    }

    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) return readdir.apply(this, arguments);
    const { asarPath, filePath } = pathInfo;

    const archive = getOrCreateArchive(asarPath);
    if (!archive) {
      const error = createError(AsarError.INVALID_ARCHIVE, { asarPath });
      nextTick(callback!, [error]);
      return;
    }

    const files = archive.readdir(filePath);
    if (!files) {
      const error = createError(AsarError.NOT_FOUND, { asarPath, filePath });
      nextTick(callback!, [error]);
      return;
    }

    if (options?.withFileTypes) {
      const dirents = [];
      for (const file of files) {
        const childPath = path.join(filePath, file);
        const stats = archive.stat(childPath);
        if (!stats) {
          const error = createError(AsarError.NOT_FOUND, { asarPath, filePath: childPath });
          nextTick(callback!, [error]);
          return;
        }
        dirents.push(getDirent(pathArgument, file, stats.type));
      }
      nextTick(callback!, [null, dirents]);
      return;
    }

    nextTick(callback!, [null, files]);
  };

  const { readdir: readdirPromise } = fs.promises;
  fs.promises.readdir = async function (pathArgument: string, options: ReaddirOptions) {
    options = getOptions(options);
    pathArgument = getValidatedPath(pathArgument);

    if (options?.recursive != null) {
      validateBoolean(options?.recursive, 'options.recursive')!;
    }

    if (options?.recursive) {
      return readdirRecursivePromises(pathArgument, options);
    }

    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) return readdirPromise(pathArgument, options);
    const { asarPath, filePath } = pathInfo;

    const archive = getOrCreateArchive(asarPath);
    if (!archive) {
      return Promise.reject(createError(AsarError.INVALID_ARCHIVE, { asarPath }));
    }

    const files = archive.readdir(filePath);
    if (!files) {
      return Promise.reject(createError(AsarError.NOT_FOUND, { asarPath, filePath }));
    }

    if (options?.withFileTypes) {
      const dirents = [];
      for (const file of files) {
        const childPath = path.join(filePath, file);
        const stats = archive.stat(childPath);
        if (!stats) {
          throw createError(AsarError.NOT_FOUND, { asarPath, filePath: childPath });
        }
        dirents.push(getDirent(pathArgument, file, stats.type));
      }
      return Promise.resolve(dirents);
    }

    return Promise.resolve(files);
  };

  const { readdirSync } = fs;
  fs.readdirSync = function (pathArgument: string, options: ReaddirOptions) {
    options = getOptions(options);
    pathArgument = getValidatedPath(pathArgument);

    if (options?.recursive != null) {
      validateBoolean(options?.recursive, 'options.recursive')!;
    }

    if (options?.recursive) {
      return readdirSyncRecursive(pathArgument, options);
    }

    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) return readdirSync.apply(this, arguments);
    const { asarPath, filePath } = pathInfo;

    const archive = getOrCreateArchive(asarPath);
    if (!archive) {
      throw createError(AsarError.INVALID_ARCHIVE, { asarPath });
    }

    const files = archive.readdir(filePath);
    if (!files) {
      throw createError(AsarError.NOT_FOUND, { asarPath, filePath });
    }

    if (options?.withFileTypes) {
      const dirents = [];
      for (const file of files) {
        const childPath = path.join(filePath, file);
        const stats = archive.stat(childPath);
        if (!stats) {
          throw createError(AsarError.NOT_FOUND, { asarPath, filePath: childPath });
        }
        dirents.push(getDirent(pathArgument, file, stats.type));
      }
      return dirents;
    }

    return files;
  };

  const modBinding = internalBinding('modules');
  modBinding.overrideReadFileSync((jsonPath: string): Buffer | false | undefined => {
    const pathInfo = splitPath(jsonPath);

    // Fallback to Node.js internal implementation
    if (!pathInfo.isAsar) return undefined;

    try {
      return readFileFromArchiveSync(pathInfo, undefined);
    } catch {
      // Not found
      return false;
    }
  });

  // Module resolution repeats most archive probes and an open archive's header
  // never changes, so answers derived from it are kept (bounded).
  const moduleStatCache = new Map<string, number>();
  const kModuleStatCacheLimit = 16 * 1024;
  const { internalModuleStat } = binding;
  internalBinding('fs').internalModuleStat = (pathArgument: string) => {
    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) return internalModuleStat(pathArgument);
    const cached = moduleStatCache.get(pathArgument);
    if (cached !== undefined) return cached;
    const { asarPath, filePath } = pathInfo;

    // -ENOENT; not cached, the archive may appear later.
    const archive = getOrCreateArchive(asarPath);
    if (!archive) return -34;

    // Like a real stat, follow symbolic links inside the archive.
    const resolved = resolveAsarLinks(archive, filePath);
    const result = !resolved ? -34 : resolved.stats.type === AsarFileType.kDirectory ? 1 : 0;
    if (moduleStatCache.size >= kModuleStatCacheLimit) moduleStatCache.clear();
    moduleStatCache.set(pathArgument, result);
    return result;
  };

  async function readdirRecursivePromises(originalPath: string, options: ReaddirOptions) {
    const result: any[] = [];

    const pathInfo = splitPath(originalPath);
    let queue: [string, string[]][] = [];
    const withFileTypes = Boolean(options?.withFileTypes);

    let initialItem = [];
    if (pathInfo.isAsar) {
      const archive = getOrCreateArchive(pathInfo.asarPath);
      if (!archive) return result;
      const files = archive.readdir(pathInfo.filePath);
      if (!files) return result;

      // If we're in an asar dir, we need to ensure the result is in the same format as the
      // native call to readdir withFileTypes i.e. an array of arrays.
      initialItem = files;
      if (withFileTypes) {
        initialItem = [
          [...initialItem],
          initialItem.map((p: string) => {
            return statTypeForReaddir(path.join(originalPath, p));
          })
        ];
      }
    } else {
      initialItem = await binding.readdir(
        path.toNamespacedPath(originalPath),
        options!.encoding,
        withFileTypes,
        kUsePromises
      );
    }

    queue = [[originalPath, initialItem]];

    if (withFileTypes) {
      while (queue.length > 0) {
        // @ts-expect-error this is a valid array destructure assignment.
        const { 0: pathArg, 1: readDir } = queue.pop();
        for (const dirent of getDirents(pathArg, readDir)) {
          result.push(dirent);
          if (dirent.isDirectory()) {
            const direntPath = path.join(pathArg, dirent.name);
            const info = splitPath(direntPath);
            let readdirResult;
            if (info.isAsar) {
              const archive = getOrCreateArchive(info.asarPath);
              if (!archive) continue;
              const files = archive.readdir(info.filePath);
              if (!files) continue;

              readdirResult = [
                [...files],
                files.map((p: string) => {
                  return statTypeForReaddir(path.join(direntPath, p));
                })
              ];
            } else {
              readdirResult = await binding.readdir(direntPath, options!.encoding, true, kUsePromises);
            }
            queue.push([direntPath, readdirResult]);
          }
        }
      }
    } else {
      while (queue.length > 0) {
        // @ts-expect-error this is a valid array destructure assignment.
        const { 0: pathArg, 1: readDir } = queue.pop();
        for (const ent of readDir) {
          const direntPath = path.join(pathArg, ent);
          const stat = statTypeForReaddir(direntPath);
          result.push(path.relative(originalPath, direntPath));

          if (stat === 1) {
            const subPathInfo = splitPath(direntPath);
            let item = [];
            if (subPathInfo.isAsar) {
              const archive = getOrCreateArchive(subPathInfo.asarPath);
              if (!archive) return;
              const files = archive.readdir(subPathInfo.filePath);
              if (!files) return result;
              item = files;
            } else {
              item = await binding.readdir(path.toNamespacedPath(direntPath), options!.encoding, false, kUsePromises);
            }
            queue.push([direntPath, item]);
          }
        }
      }
    }

    return result;
  }

  function readdirSyncRecursive(basePath: string, options: ReaddirOptions) {
    const context = {
      withFileTypes: Boolean(options!.withFileTypes),
      encoding: options!.encoding,
      basePath,
      readdirResults: [] as any,
      pathsQueue: [basePath]
    };

    function read(pathArg: string) {
      let readdirResult;

      const pathInfo = splitPath(pathArg);
      if (pathInfo.isAsar) {
        const { asarPath, filePath } = pathInfo;
        const archive = getOrCreateArchive(asarPath);
        if (!archive) return;

        readdirResult = archive.readdir(filePath);
        if (!readdirResult) return;
        // If we're in an asar dir, we need to ensure the result is in the same format as the
        // native call to readdir withFileTypes i.e. an array of arrays.
        if (context.withFileTypes) {
          readdirResult = [
            [...readdirResult],
            readdirResult.map((p: string) => {
              return statTypeForReaddir(path.join(pathArg, p));
            })
          ];
        }
      } else {
        readdirResult = binding.readdir(path.toNamespacedPath(pathArg), context.encoding, context.withFileTypes);
      }

      if (readdirResult === undefined) {
        return;
      }

      processReaddirResult({
        result: readdirResult,
        currentPath: pathArg,
        context
      });
    }

    for (let i = 0; i < context.pathsQueue.length; i++) {
      read(context.pathsQueue[i]);
    }

    return context.readdirResults;
  }

  // Calling mkdir for directory inside asar archive should throw ENOTDIR
  // error, but on Windows it throws ENOENT.
  if (process.platform === 'win32') {
    const { mkdir } = fs;
    fs.mkdir = (pathArgument: string, options: any, callback: any) => {
      if (typeof options === 'function') {
        callback = options;
        options = {};
      }

      const pathInfo = splitPath(pathArgument);
      if (pathInfo.isAsar && pathInfo.filePath.length > 0) {
        const error = createError(AsarError.NOT_DIR);
        nextTick(callback, [error]);
        return;
      }

      mkdir(pathArgument, options, callback);
    };

    fs.promises.mkdir = util.promisify(fs.mkdir);

    const { mkdirSync } = fs;
    fs.mkdirSync = function (pathArgument: string, options: any) {
      const pathInfo = splitPath(pathArgument);
      if (pathInfo.isAsar && pathInfo.filePath.length) throw createError(AsarError.NOT_DIR);
      return mkdirSync(pathArgument, options);
    };
  }

  function invokeWithNoAsar(func: Function) {
    return function (this: any) {
      const processNoAsarOriginalValue = process.noAsar;
      process.noAsar = true;
      try {
        return func.apply(this, arguments);
      } finally {
        process.noAsar = processNoAsarOriginalValue;
      }
    };
  }

  //
  // fd-based access to packed entries: fs.open / fs.openSync /
  // fs.promises.open and everything built on them (fs.createReadStream,
  // fs.readFile(fd), FileHandle#read/readFile/stat/createReadStream, ...).
  //
  // These are hooked at the fs binding layer, below node's own argument
  // parsing, so that every JS entry point in node that funnels down to
  // binding.read / binding.fstat / binding.close for one of our fds is
  // covered without re-implementing node's overload handling.
  //

  // Node's native fs functions CHECK() their exact argument count to decide
  // between sync and async forms, so pass-through calls must forward the
  // original argument list untouched (in particular, never append a trailing
  // `undefined`).  Forwarding uses Function#apply and index access rather
  // than spread / array destructuring: those depend on Array.prototype's
  // iterator, which user code is allowed to delete (Node's own internals use
  // primordials for the same reason) and fs must keep working when it does.
  binding.open = function (...args: any[]) {
    const pathArgument = args[0];
    const flags = args[1];
    const req = args[3];
    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) return passThroughOpen(originalBinding.open, args, (fd) => fd);
    const { asarPath, filePath } = pathInfo;

    let opened;
    try {
      opened = openAsarEntry(asarPath, filePath, flags, 'open');
    } catch (error) {
      return completeRequest(req, error as Error);
    }
    if ('unpackedPath' in opened) {
      args[0] = opened.unpackedPath;
      return passThroughOpen(originalBinding.open, args, (fd) => fd);
    }

    const { reader } = opened;
    if (openAsarFiles.size > 256) sweepStaleAsarFds();
    // The number may have belonged to a handle that was closed behind our
    // back; make sure its entry (and retained block) is released first.
    forgetAsarFd(reader.fd);
    openAsarFiles.set(reader.fd, { reader });
    return completeRequest(req, null, reader.fd);
  };

  binding.openFileHandle = function (...args: any[]) {
    const pathArgument = args[0];
    const flags = args[1];
    const req = args[3];
    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) return passThroughOpen(originalBinding.openFileHandle, args, (handle) => handle?.fd);
    const { asarPath, filePath } = pathInfo;

    let opened;
    try {
      opened = openAsarEntry(asarPath, filePath, flags, 'open');
    } catch (error) {
      return completeRequest(req, error as Error);
    }
    if ('unpackedPath' in opened) {
      args[0] = opened.unpackedPath;
      return passThroughOpen(originalBinding.openFileHandle, args, (handle) => handle?.fd);
    }

    const { reader } = opened;
    // Node's C++ FileHandle takes ownership of the fd (and closes it on
    // close() or garbage collection); the JS FileHandle wraps this object.
    const handle = new binding.FileHandle(reader.fd);
    // close() runs the actual close(2) on the threadpool and only resets the
    // handle's fd to -1 afterwards, on the loop thread; forget the entry as
    // soon as close is requested so that a real file opened in between and
    // handed the same descriptor number can never be routed through this
    // reader.
    const { close } = handle;
    handle.close = function (this: any, ...closeArgs: any[]) {
      forgetAsarFd(reader.fd);
      return close.apply(this, closeArgs);
    };
    if (openAsarFiles.size > 256) sweepStaleAsarFds();
    forgetAsarFd(reader.fd);
    openAsarFiles.set(reader.fd, { reader, handle: new WeakRef(handle) });
    return completeRequest(req, null, handle);
  };

  binding.read = function (...args: any[]) {
    const fd = args[0];
    const buffer = args[1];
    const offset = args[2];
    const length = args[3];
    const position = args[4];
    const req = args[5];
    const reader = lookupAsarFd(fd);
    if (reader === undefined) return originalBinding.read.apply(undefined, args);

    const pos = normalizePosition(position);
    if (reader.integrity === null) {
      // Fast path: without integrity information the read only needs its
      // position translated and clamped to the entry, so hand it straight to
      // the native binding with the caller's own request object.
      const { start, count } = reader.resolveRange(length, pos);
      if (count === 0) return completeRequest(req, null, 0);
      args[0] = reader.archiveFd;
      args[3] = count;
      args[4] = reader.offset + start;
      return originalBinding.read.apply(undefined, args);
    }
    if (req === undefined) return reader.readSync(buffer, offset, length, pos);
    return completeRequestWithPromise(req, reader.read(buffer, offset, length, pos));
  };

  binding.readBuffers = function (...args: any[]) {
    const fd: number = args[0];
    const buffers: Uint8Array[] = args[1];
    const position: number | bigint | null = args[2];
    const req = args[3];
    const reader = lookupAsarFd(fd);
    if (reader === undefined) return originalBinding.readBuffers.apply(undefined, args);

    const pos = normalizePosition(position);
    if (reader.integrity === null) {
      // Fast path (see binding.read): when every buffer fits inside the
      // entry the native readv can be used as-is with a translated position.
      let wanted = 0;
      for (let i = 0; i < buffers.length; i++) wanted += buffers[i].byteLength;
      const { start, count } = reader.resolveRange(wanted, pos);
      if (count === 0) return completeRequest(req, null, 0);
      if (count === wanted) {
        args[0] = reader.archiveFd;
        args[2] = reader.offset + start;
        return originalBinding.readBuffers.apply(undefined, args);
      }
      // Otherwise the tail must be clamped; the read position was already
      // advanced by resolveRange, so serve the buffers positionally from
      // `start`.
      const filled: number[] = [];
      let remaining = count;
      for (let i = 0; i < buffers.length; i++) {
        const take = Math.min(remaining, buffers[i].byteLength);
        filled.push(take);
        remaining -= take;
      }
      // (Zero-length buffers must not end the loop: like preadv, they are
      // skipped and later buffers are still filled.)
      const readAllClamped = async () => {
        let total = 0;
        for (let i = 0; i < buffers.length; i++) {
          total += await readFully(reader.archiveFd, buffers[i], 0, filled[i], reader.offset + start + total);
        }
        return total;
      };
      if (req === undefined) {
        let total = 0;
        for (let i = 0; i < buffers.length; i++) {
          total += readFullySync(reader.archiveFd, buffers[i], 0, filled[i], reader.offset + start + total);
        }
        return total;
      }
      return completeRequestWithPromise(req, readAllClamped());
    }
    if (req === undefined) {
      let total = 0;
      for (let i = 0; i < buffers.length; i++) {
        const buffer = buffers[i];
        const n = reader.readSync(buffer, 0, buffer.byteLength, pos < 0 ? -1 : pos + total);
        total += n;
        if (n < buffer.byteLength) break;
      }
      return total;
    }
    const readAll = async () => {
      let total = 0;
      for (let i = 0; i < buffers.length; i++) {
        const buffer = buffers[i];
        const n = await reader.read(buffer, 0, buffer.byteLength, pos < 0 ? -1 : pos + total);
        total += n;
        if (n < buffer.byteLength) break;
      }
      return total;
    };
    return completeRequestWithPromise(req, readAll());
  };

  binding.fstat = function (...args: any[]) {
    const fd = args[0];
    const useBigint = args[1];
    const req = args[2];
    const reader = lookupAsarFd(fd);
    if (reader === undefined) return originalBinding.fstat.apply(undefined, args);
    return completeRequest(req, null, reader.statArray(Boolean(useBigint)));
  };

  binding.close = function (...args: any[]) {
    const fd = args[0];
    if (lookupAsarFd(fd) !== undefined) forgetAsarFd(fd);
    return originalBinding.close.apply(undefined, args);
  };

  binding.readFileUtf8 = function (...args: any[]) {
    const reader = lookupAsarFd(args[0]);
    if (reader === undefined) return originalBinding.readFileUtf8.apply(undefined, args);
    return reader.readToEndSync().toString('utf8');
  };

  // fs.writeFileSync's utf8 fast path opens the file natively by path; route
  // it through the same checks as open() so writes into archives fail with
  // EACCES (or land in the real file for unpacked entries) instead of
  // whatever the OS says about a path that goes through a file.
  binding.writeFileUtf8 = function (...args: any[]) {
    const pathOrFd = args[0];
    const flags = args[2];
    const pathInfo = splitPath(pathOrFd);
    if (!pathInfo.isAsar) return originalBinding.writeFileUtf8.apply(undefined, args);
    const { asarPath, filePath } = pathInfo;

    const opened = openAsarEntry(asarPath, filePath, flags, 'open', false);
    if ('unpackedPath' in opened) {
      args[0] = opened.unpackedPath;
      return originalBinding.writeFileUtf8.apply(undefined, args);
    }
    // Only reachable with read-only flags, which writeFile never uses.
    opened.reader.dispose();
    throw createError(AsarError.NO_ACCESS, { asarPath, filePath, syscall: 'open' });
  };

  // The sentinel descriptor is a write-only null device: reads through
  // anything but fs fail (EBADF), but writes, truncation and metadata changes
  // would quietly succeed against the null device.  Archives are read-only,
  // so refuse all of those explicitly with EACCES.  The sync write bindings
  // still use the legacy (..., undefined, ctx) convention where errors are
  // reported through `ctx` instead of being thrown.
  const refuseMutation = (name: keyof typeof originalBinding) => {
    const original = originalBinding[name] as Function;
    binding[name] = function (...args: any[]) {
      const fd = args[0];
      if (lookupAsarFd(fd) === undefined) return original.apply(undefined, args);
      const error = createError(AsarError.NO_ACCESS, { filePath: `fd ${fd}`, syscall: name });
      const last = args[args.length - 1];
      const isObject = last !== null && typeof last === 'object';
      if (last === kUsePromises || (isObject && 'oncomplete' in last)) {
        return completeRequest(last, error);
      }
      if (args.length >= 2 && args[args.length - 2] === undefined && isObject) {
        // (…, undefined, ctx): report through ctx like the native sync path.
        last.errno = error.errno;
        last.code = error.code;
        last.syscall = name;
        return;
      }
      throw error;
    };
  };
  for (const name of [
    'fchmod',
    'fchown',
    'futimes',
    'ftruncate',
    'writeBuffer',
    'writeBuffers',
    'writeString'
  ] as const) {
    refuseMutation(name);
  }

  //
  // fs.copyFile / fs.copyFileSync / fs.promises.copyFile from a packed entry:
  // stream the (validated) bytes straight into the destination instead of
  // materialising a temporary copy first.
  //

  const kCopyChunkSize = 1024 * 1024;
  const kMaxCopyFileMode = constants.COPYFILE_EXCL | constants.COPYFILE_FICLONE | constants.COPYFILE_FICLONE_FORCE;

  function copyFileFlagsToOpenFlags(mode: number) {
    return (
      constants.O_WRONLY |
      constants.O_CREAT |
      ((mode & constants.COPYFILE_EXCL) !== 0 ? constants.O_EXCL : constants.O_TRUNC)
    );
  }

  function copyPackedFileSync(reader: AsarEntryReader, dest: string, mode: number) {
    const destMode = reader.executable ? 0o755 : 0o644;
    const destFd = originalBinding.open(dest, copyFileFlagsToOpenFlags(mode), destMode);
    try {
      const chunk = Buffer.allocUnsafeSlow(Math.min(kCopyChunkSize, Math.max(reader.size, 1)));
      let position = 0;
      while (position < reader.size) {
        const n = reader.readSync(chunk, 0, Math.min(chunk.length, reader.size - position), position);
        if (n === 0) break;
        let written = 0;
        while (written < n) {
          // The synchronous writeBuffer binding still uses the legacy ctx
          // calling convention, so go through fs.writeSync (not overridden here).
          written += fs.writeSync(destFd, chunk, written, n - written);
        }
        position += n;
      }
    } finally {
      originalBinding.close(destFd);
      reader.dispose();
    }
  }

  async function copyPackedFile(reader: AsarEntryReader, dest: string, mode: number) {
    const destMode = reader.executable ? 0o755 : 0o644;
    const destFd = await originalBinding.open(dest, copyFileFlagsToOpenFlags(mode), destMode, kUsePromises);
    try {
      const chunk = Buffer.allocUnsafeSlow(Math.min(kCopyChunkSize, Math.max(reader.size, 1)));
      let position = 0;
      while (position < reader.size) {
        const n = await reader.read(chunk, 0, Math.min(chunk.length, reader.size - position), position);
        if (n === 0) break;
        let written = 0;
        while (written < n) {
          written += (await originalBinding.writeBuffer(destFd, chunk, written, n - written, null, kUsePromises)) || 0;
        }
        position += n;
      }
    } finally {
      await originalBinding.close(destFd, kUsePromises);
      reader.dispose();
    }
  }

  binding.copyFile = function (...args: any[]) {
    const src = args[0];
    const dest = args[1];
    const mode = args[2];
    const req = args[3];
    const pathInfo = splitPath(src);
    if (!pathInfo.isAsar) return originalBinding.copyFile.apply(undefined, args);
    const { asarPath, filePath } = pathInfo;

    let opened;
    const modeFlags = mode ?? 0;
    try {
      // Same validation as node's native CopyFile (GetValidFileMode).
      if (mode !== null && mode !== undefined && (typeof mode !== 'number' || (mode | 0) !== mode)) {
        throw new (errorCodes as any).ERR_INVALID_ARG_TYPE('mode', 'int32', mode);
      }
      if (modeFlags < 0 || modeFlags > kMaxCopyFileMode) {
        throw new (errorCodes as any).ERR_OUT_OF_RANGE('mode', `>= 0 && <= ${kMaxCopyFileMode}`, mode);
      }
      opened = openAsarEntry(asarPath, filePath, constants.O_RDONLY, 'copyfile', false);
    } catch (error) {
      return completeRequest(req, error as Error);
    }
    if ('unpackedPath' in opened) {
      // Unpacked entries are real files: let native fs handle them, including
      // copy-on-write clones.
      args[0] = opened.unpackedPath;
      return originalBinding.copyFile.apply(undefined, args);
    }

    const { reader } = opened;
    if ((modeFlags & constants.COPYFILE_FICLONE_FORCE) !== 0) {
      // A packed entry can never be reflinked.
      reader.dispose();
      return completeRequest(req, createError(AsarError.NOT_SUPPORTED, { asarPath, filePath, syscall: 'copyfile' }));
    }
    if (req === undefined) {
      copyPackedFileSync(reader, dest, mode ?? 0);
      return;
    }
    return completeRequestWithPromise(req, copyPackedFile(reader, dest, mode ?? 0));
  };

  //
  // fs.readlink / fs.readlinkSync / fs.promises.readlink for symbolic links
  // stored in an archive.  Archives store link targets relative to the
  // archive root; what a real filesystem would report is the target relative
  // to the link's own directory, so that is what gets returned (which also
  // makes trees copied out with fs.cp({ verbatimSymlinks: true }) work).
  //

  binding.readlink = function (...args: any[]) {
    const pathArgument = args[0];
    const encoding = args[1];
    const req = args[2];
    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) return originalBinding.readlink.apply(undefined, args);
    const { asarPath, filePath } = pathInfo;

    const archive = getOrCreateArchive(asarPath);
    if (!archive) {
      return completeRequest(req, createError(AsarError.INVALID_ARCHIVE, { asarPath, syscall: 'readlink' }));
    }

    const stats = archive.stat(filePath);
    if (!stats) {
      return completeRequest(req, createError(AsarError.NOT_FOUND, { asarPath, filePath, syscall: 'readlink' }));
    }
    if (stats.type !== AsarFileType.kLink) {
      return completeRequest(req, createError(AsarError.NOT_LINK, { asarPath, filePath, syscall: 'readlink' }));
    }

    const linkTarget = archive.realpath(filePath);
    if (linkTarget === false) {
      return completeRequest(req, createError(AsarError.NOT_FOUND, { asarPath, filePath, syscall: 'readlink' }));
    }
    // Both sides are archive-relative; anchor them at a root so that the
    // computation does not depend on the process's working directory.
    const target = path.relative(path.join(path.sep, path.dirname(filePath)), path.join(path.sep, linkTarget)) || '.';
    return completeRequest(req, null, encodeName(target, encoding));
  };

  //
  // fs.opendir / fs.opendirSync / fs.promises.opendir on archive directories.
  // Node's Dir class drives a native DirHandle through a small protocol
  // (read(encoding, bufferSize[, req]) -> [name, type, name, type, ...] |
  // null; close([req])); provide the same over the archive header.  Because
  // Dir also opens sub-directories through this binding when `recursive` is
  // set, recursive iteration works too.
  //

  class AsarDirHandle {
    private entries: [string, number][] | null;

    constructor(entries: [string, number][]) {
      this.entries = entries;
    }

    read(encoding: any, bufferSize: number, req?: any) {
      let result: any[] | null = null;
      if (this.entries !== null && this.entries.length > 0) {
        const count = Math.max(1, bufferSize | 0);
        const taken = this.entries.splice(0, count);
        result = [];
        for (const [name, type] of taken) {
          result.push(encodeName(name, encoding), type);
        }
      }
      return completeRequest(req, null, result);
    }

    close(req?: any) {
      this.entries = null;
      return completeRequest(req, null, undefined);
    }
  }

  function openAsarDir(pathInfo: { asarPath: string; filePath: string }, req: any) {
    const { asarPath, filePath } = pathInfo;

    const archive = getOrCreateArchive(asarPath);
    if (!archive) return completeRequest(req, createError(AsarError.INVALID_ARCHIVE, { asarPath, syscall: 'opendir' }));

    // opendir follows symbolic links.
    const resolved = resolveAsarLinks(archive, filePath);
    if (!resolved) {
      return completeRequest(req, createError(AsarError.NOT_FOUND, { asarPath, filePath, syscall: 'opendir' }));
    }
    if (resolved.stats.type !== AsarFileType.kDirectory) {
      return completeRequest(req, createError(AsarError.NOT_DIR, { asarPath, filePath, syscall: 'opendir' }));
    }

    const names = archive.readdir(filePath);
    if (!names) {
      return completeRequest(req, createError(AsarError.NOT_FOUND, { asarPath, filePath, syscall: 'opendir' }));
    }

    const entries: [string, number][] = [];
    for (const name of names) {
      const childStats = archive.stat(path.join(filePath, name));
      if (!childStats) continue;
      entries.push([name, childStats.type]);
    }
    return completeRequest(req, null, new AsarDirHandle(entries));
  }

  dirBinding.opendir = function (...args: any[]) {
    const pathArgument = args[0];
    const req = args[2];
    const pathInfo = splitPath(pathArgument);
    if (!pathInfo.isAsar) return originalDirBinding.opendir.apply(undefined, args);
    return openAsarDir(pathInfo, req);
  };

  dirBinding.opendirSync = function (...args: any[]) {
    const pathInfo = splitPath(args[0]);
    if (!pathInfo.isAsar) return originalDirBinding.opendirSync.apply(undefined, args);
    return openAsarDir(pathInfo, undefined);
  };

  //
  // fs.cpSync from an archive.  Node's cp() (async) is written on top of
  // fs.promises and works against the overrides above; cpSync() calls into
  // native helpers that resolve paths themselves, so those are given
  // archive-aware equivalents.
  //

  const { ERR_FS_CP_EINVAL, ERR_FS_CP_DIR_TO_NON_DIR, ERR_FS_CP_NON_DIR_TO_DIR, ERR_FS_EISDIR, ERR_FS_CP_EEXIST } =
    errorCodes as any;

  const isSubdirectory = (parent: string, child: string) => {
    const relative = path.relative(parent, child);
    return relative !== '' && !relative.startsWith('..') && !path.isAbsolute(relative);
  };

  binding.cpSyncCheckPaths = function (...args: any[]) {
    const src = args[0];
    const dest = args[1];
    const dereference = args[2];
    const recursive = args[3];
    if (!splitPath(src).isAsar) return originalBinding.cpSyncCheckPaths.apply(undefined, args);

    const statFn = dereference ? fs.statSync : fs.lstatSync;
    const srcStat = statFn(src);
    const destStat = statFn(dest, { throwIfNoEntry: false });
    const srcIsDir = srcStat.isDirectory();
    if (destStat) {
      const destIsDir = destStat.isDirectory();
      if (srcIsDir && !destIsDir) {
        throw new ERR_FS_CP_DIR_TO_NON_DIR({
          message: `Cannot overwrite non-directory ${dest} with directory ${src}`,
          path: dest,
          syscall: 'cp',
          errno: -22,
          code: 'EINVAL'
        });
      }
      if (!srcIsDir && destIsDir) {
        throw new ERR_FS_CP_NON_DIR_TO_DIR({
          message: `Cannot overwrite directory ${dest} with non-directory ${src}`,
          path: dest,
          syscall: 'cp',
          errno: -22,
          code: 'EINVAL'
        });
      }
    }
    if (srcIsDir && isSubdirectory(path.resolve(src), path.resolve(dest))) {
      throw new ERR_FS_CP_EINVAL({
        message: `Cannot copy ${src} to a subdirectory of self ${dest}`,
        path: dest,
        syscall: 'cp',
        errno: -22,
        code: 'EINVAL'
      });
    }
    if (srcIsDir && !recursive) {
      throw new ERR_FS_EISDIR({
        message: `Recursive option not enabled, cannot copy a directory: ${src}`,
        path: src,
        syscall: 'cp',
        errno: -21,
        code: 'EISDIR'
      });
    }
  };

  binding.cpSyncOverrideFile = function (...args: any[]) {
    const src = args[0];
    const dest = args[1];
    const mode = args[2];
    const preserveTimestamps = args[3];
    if (!splitPath(src).isAsar) return originalBinding.cpSyncOverrideFile.apply(undefined, args);
    fs.unlinkSync(dest);
    fs.copyFileSync(src, dest, mode);
    if (preserveTimestamps) {
      const srcStat = fs.statSync(src);
      fs.utimesSync(dest, srcStat.atime, srcStat.mtime);
    }
  };

  binding.cpSyncCopyDir = function (...args: any[]) {
    const src = args[0];
    const dest = args[1];
    const force = args[2];
    const dereference = args[3];
    const errorOnExist = args[4];
    const verbatimSymlinks = args[5];
    const preserveTimestamps = args[6];
    if (!splitPath(src).isAsar) return originalBinding.cpSyncCopyDir.apply(undefined, args);

    const copyDir = (srcDir: string, destDir: string) => {
      fs.mkdirSync(destDir, { recursive: true });
      for (const dirent of fs.readdirSync(srcDir, { withFileTypes: true }) as Dirent[]) {
        const srcItem = path.join(srcDir, dirent.name);
        const destItem = path.join(destDir, dirent.name);
        let isDirectory = dirent.isDirectory();
        if (dirent.isSymbolicLink()) {
          if (!dereference) {
            let target = fs.readlinkSync(srcItem);
            if (!verbatimSymlinks && !path.isAbsolute(target)) target = path.resolve(srcDir, target);
            // Like node's native implementation: an existing symlink at the
            // destination is replaced, anything else is an error.
            const existing = fs.lstatSync(destItem, { throwIfNoEntry: false });
            if (existing) {
              if (!existing.isSymbolicLink()) {
                throw createError(AsarError.EXISTS, { asarPath: dest, filePath: dirent.name, syscall: 'cp' });
              }
              fs.unlinkSync(destItem);
            }
            fs.symlinkSync(target, destItem);
            continue;
          }
          isDirectory = fs.statSync(srcItem).isDirectory();
        }
        if (isDirectory) {
          copyDir(srcItem, destItem);
          continue;
        }
        const destStat = fs.lstatSync(destItem, { throwIfNoEntry: false });
        if (destStat) {
          if (!force) {
            if (errorOnExist) {
              throw new ERR_FS_CP_EEXIST({
                message: `${destItem} already exists`,
                path: destItem,
                syscall: 'cp',
                errno: -17,
                code: 'EEXIST'
              });
            }
            continue;
          }
          fs.unlinkSync(destItem);
        }
        fs.copyFileSync(srcItem, destItem);
        if (preserveTimestamps) {
          const srcStat = fs.statSync(srcItem);
          fs.utimesSync(destItem, srcStat.atime, srcStat.mtime);
        }
      }
    };
    copyDir(src, dest);
  };

  // Native modules and child processes need a real file on disk, so these
  // are the only remaining consumers of Archive::CopyFileOut.
  overrideAPISync(process, 'dlopen', 1);
  overrideAPISync(Module._extensions, '.node', 1);

  const overrideChildProcess = (childProcess: Record<string, any>) => {
    // Executing a command string containing a path to an asar archive
    // confuses `childProcess.execFile`, which is internally called by
    // `childProcess.{exec,execSync}`, causing Electron to consider the full
    // command as a single path to an archive.
    const { exec, execSync } = childProcess;
    childProcess.exec = invokeWithNoAsar(exec);
    childProcess.exec[util.promisify.custom] = assignFunctionName(
      'exec',
      invokeWithNoAsar(exec[util.promisify.custom])
    );
    childProcess.execSync = invokeWithNoAsar(execSync);

    overrideAPI(childProcess, 'execFile');
    overrideAPISync(childProcess, 'execFileSync');
  };

  const asarReady = new WeakSet();

  // Lazily override the child_process APIs only when child_process is
  // fetched the first time.  We will eagerly override the child_process APIs
  // when this env var is set so that stack traces generated inside node unit
  // tests will match. This env var will only slow things down in users apps
  // and should not be used.
  if (process.env.ELECTRON_EAGER_ASAR_HOOK_FOR_TESTING) {
    overrideChildProcess(require('child_process'));
  } else {
    const originalModuleLoad = Module._load;
    Module._load = function (this: any, request: string) {
      const loadResult = originalModuleLoad.apply(this, arguments as any);
      if (request === 'child_process' || request === 'node:child_process') {
        if (!asarReady.has(loadResult)) {
          asarReady.add(loadResult);
          // Just to make it obvious what we are dealing with here
          const childProcess = loadResult;

          overrideChildProcess(childProcess);
        }
      }
      return loadResult;
    };
  }
};

function getDirents(p: string, { 0: names, 1: types }: any[][]): Dirent[] {
  for (let i = 0; i < names.length; i++) {
    let type = types[i];
    const info = splitPath(path.join(p, names[i]));
    if (info.isAsar) {
      const archive = getOrCreateArchive(info.asarPath);
      if (!archive) continue;
      const stats = archive.stat(info.filePath);
      if (!stats) continue;
      type = stats.type;
    }
    names[i] = getDirent(p, names[i], type);
  }

  return names;
}
