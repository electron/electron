const { expect } = require('chai');
const ChildProcess = require('child_process');
const fs = require('fs');
const path = require('path');
const temp = require('temp').track();
const util = require('util');
const { emittedOnce } = require('./events-helpers');
const { ifit, ifdescribe } = require('./spec-helpers');
const nativeImage = require('electron').nativeImage;

const features = process._linkedBinding('electron_common_features');

async function expectToThrowErrorWithCode (func, code) {
  let error;
  try {
    await func();
  } catch (e) {
    error = e;
  }

  expect(error).is.an('Error');
  expect(error).to.have.property('code').which.equals(code);
}

describe('asar package', function () {
  const fixtures = path.join(__dirname, 'fixtures');
  const asarDir = path.join(fixtures, 'test.asar');

  describe('node api', function () {
    it('supports paths specified as a Buffer', function () {
      const file = Buffer.from(path.join(asarDir, 'a.asar', 'file1'));
      expect(fs.existsSync(file)).to.be.true();
    });

    describe('fs.readFileSync', function () {
      it('does not leak fd', function () {
        let readCalls = 1;
        while (readCalls <= 10000) {
          fs.readFileSync(path.join(process.resourcesPath, 'default_app.asar', 'main.js'));
          readCalls++;
        }
      });

      it('reads a normal file', function () {
        const file1 = path.join(asarDir, 'a.asar', 'file1');
        expect(fs.readFileSync(file1).toString().trim()).to.equal('file1');
        const file2 = path.join(asarDir, 'a.asar', 'file2');
        expect(fs.readFileSync(file2).toString().trim()).to.equal('file2');
        const file3 = path.join(asarDir, 'a.asar', 'file3');
        expect(fs.readFileSync(file3).toString().trim()).to.equal('file3');
      });

      it('reads from a empty file', function () {
        const file = path.join(asarDir, 'empty.asar', 'file1');
        const buffer = fs.readFileSync(file);
        expect(buffer).to.be.empty();
        expect(buffer.toString()).to.equal('');
      });

      it('reads a linked file', function () {
        const p = path.join(asarDir, 'a.asar', 'link1');
        expect(fs.readFileSync(p).toString().trim()).to.equal('file1');
      });

      it('reads a file from linked directory', function () {
        const p1 = path.join(asarDir, 'a.asar', 'link2', 'file1');
        expect(fs.readFileSync(p1).toString().trim()).to.equal('file1');
        const p2 = path.join(asarDir, 'a.asar', 'link2', 'link2', 'file1');
        expect(fs.readFileSync(p2).toString().trim()).to.equal('file1');
      });

      it('throws ENOENT error when can not find file', function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        expect(() => {
          fs.readFileSync(p);
        }).to.throw(/ENOENT/);
      });

      it('passes ENOENT error to callback when can not find file', function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        let async = false;
        fs.readFile(p, function (error) {
          expect(async).to.be.true();
          expect(error).to.match(/ENOENT/);
        });
        async = true;
      });

      it('reads a normal file with unpacked files', function () {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        expect(fs.readFileSync(p).toString().trim()).to.equal('a');
      });

      it('reads a file in filesystem', function () {
        const p = path.resolve(asarDir, 'file');
        expect(fs.readFileSync(p).toString().trim()).to.equal('file');
      });
    });

    describe('fs.readFile', function () {
      it('reads a normal file', function (done) {
        const p = path.join(asarDir, 'a.asar', 'file1');
        fs.readFile(p, function (err, content) {
          try {
            expect(err).to.be.null();
            expect(String(content).trim()).to.equal('file1');
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('reads from a empty file', function (done) {
        const p = path.join(asarDir, 'empty.asar', 'file1');
        fs.readFile(p, function (err, content) {
          try {
            expect(err).to.be.null();
            expect(String(content)).to.equal('');
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('reads from a empty file with encoding', function (done) {
        const p = path.join(asarDir, 'empty.asar', 'file1');
        fs.readFile(p, 'utf8', function (err, content) {
          try {
            expect(err).to.be.null();
            expect(content).to.equal('');
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('reads a linked file', function (done) {
        const p = path.join(asarDir, 'a.asar', 'link1');
        fs.readFile(p, function (err, content) {
          try {
            expect(err).to.be.null();
            expect(String(content).trim()).to.equal('file1');
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('reads a file from linked directory', function (done) {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2', 'file1');
        fs.readFile(p, function (err, content) {
          try {
            expect(err).to.be.null();
            expect(String(content).trim()).to.equal('file1');
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('throws ENOENT error when can not find file', function (done) {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        fs.readFile(p, function (err) {
          try {
            expect(err.code).to.equal('ENOENT');
            done();
          } catch (e) {
            done(e);
          }
        });
      });
    });

    describe('fs.promises.readFile', function () {
      it('reads a normal file', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const content = await fs.promises.readFile(p);
        expect(String(content).trim()).to.equal('file1');
      });

      it('reads from a empty file', async function () {
        const p = path.join(asarDir, 'empty.asar', 'file1');
        const content = await fs.promises.readFile(p);
        expect(String(content)).to.equal('');
      });

      it('reads from a empty file with encoding', async function () {
        const p = path.join(asarDir, 'empty.asar', 'file1');
        const content = await fs.promises.readFile(p, 'utf8');
        expect(content).to.equal('');
      });

      it('reads a linked file', async function () {
        const p = path.join(asarDir, 'a.asar', 'link1');
        const content = await fs.promises.readFile(p);
        expect(String(content).trim()).to.equal('file1');
      });

      it('reads a file from linked directory', async function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2', 'file1');
        const content = await fs.promises.readFile(p);
        expect(String(content).trim()).to.equal('file1');
      });

      it('throws ENOENT error when can not find file', async function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        await expectToThrowErrorWithCode(() => fs.promises.readFile(p), 'ENOENT');
      });
    });

    describe('fs.copyFile', function () {
      it('copies a normal file', function (done) {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const dest = temp.path();
        fs.copyFile(p, dest, function (err) {
          try {
            expect(err).to.be.null();
            expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).to.be.true();
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('copies a unpacked file', function (done) {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        const dest = temp.path();
        fs.copyFile(p, dest, function (err) {
          try {
            expect(err).to.be.null();
            expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).to.be.true();
            done();
          } catch (e) {
            done(e);
          }
        });
      });
    });

    describe('fs.promises.copyFile', function () {
      it('copies a normal file', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const dest = temp.path();
        await fs.promises.copyFile(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).to.be.true();
      });

      it('copies a unpacked file', async function () {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        const dest = temp.path();
        await fs.promises.copyFile(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).to.be.true();
      });
    });

    describe('fs.copyFileSync', function () {
      it('copies a normal file', function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const dest = temp.path();
        fs.copyFileSync(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).to.be.true();
      });

      it('copies a unpacked file', function () {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        const dest = temp.path();
        fs.copyFileSync(p, dest);
        expect(fs.readFileSync(p).equals(fs.readFileSync(dest))).to.be.true();
      });
    });

    describe('fs.lstatSync', function () {
      it('handles path with trailing slash correctly', function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2', 'file1');
        fs.lstatSync(p);
        fs.lstatSync(p + '/');
      });

      it('returns information of root', function () {
        const p = path.join(asarDir, 'a.asar');
        const stats = fs.lstatSync(p);
        expect(stats.isFile()).to.be.false();
        expect(stats.isDirectory()).to.be.true();
        expect(stats.isSymbolicLink()).to.be.false();
        expect(stats.size).to.equal(0);
      });

      it('returns information of root with stats as bigint', function () {
        const p = path.join(asarDir, 'a.asar');
        const stats = fs.lstatSync(p, { bigint: false });
        expect(stats.isFile()).to.be.false();
        expect(stats.isDirectory()).to.be.true();
        expect(stats.isSymbolicLink()).to.be.false();
        expect(stats.size).to.equal(0);
      });

      it('returns information of a normal file', function () {
        const ref2 = ['file1', 'file2', 'file3', path.join('dir1', 'file1'), path.join('link2', 'file1')];
        for (let j = 0, len = ref2.length; j < len; j++) {
          const file = ref2[j];
          const p = path.join(asarDir, 'a.asar', file);
          const stats = fs.lstatSync(p);
          expect(stats.isFile()).to.be.true();
          expect(stats.isDirectory()).to.be.false();
          expect(stats.isSymbolicLink()).to.be.false();
          expect(stats.size).to.equal(6);
        }
      });

      it('returns information of a normal directory', function () {
        const ref2 = ['dir1', 'dir2', 'dir3'];
        for (let j = 0, len = ref2.length; j < len; j++) {
          const file = ref2[j];
          const p = path.join(asarDir, 'a.asar', file);
          const stats = fs.lstatSync(p);
          expect(stats.isFile()).to.be.false();
          expect(stats.isDirectory()).to.be.true();
          expect(stats.isSymbolicLink()).to.be.false();
          expect(stats.size).to.equal(0);
        }
      });

      it('returns information of a linked file', function () {
        const ref2 = ['link1', path.join('dir1', 'link1'), path.join('link2', 'link2')];
        for (let j = 0, len = ref2.length; j < len; j++) {
          const file = ref2[j];
          const p = path.join(asarDir, 'a.asar', file);
          const stats = fs.lstatSync(p);
          expect(stats.isFile()).to.be.false();
          expect(stats.isDirectory()).to.be.false();
          expect(stats.isSymbolicLink()).to.be.true();
          expect(stats.size).to.equal(0);
        }
      });

      it('returns information of a linked directory', function () {
        const ref2 = ['link2', path.join('dir1', 'link2'), path.join('link2', 'link2')];
        for (let j = 0, len = ref2.length; j < len; j++) {
          const file = ref2[j];
          const p = path.join(asarDir, 'a.asar', file);
          const stats = fs.lstatSync(p);
          expect(stats.isFile()).to.be.false();
          expect(stats.isDirectory()).to.be.false();
          expect(stats.isSymbolicLink()).to.be.true();
          expect(stats.size).to.equal(0);
        }
      });

      it('throws ENOENT error when can not find file', function () {
        const ref2 = ['file4', 'file5', path.join('dir1', 'file4')];
        for (let j = 0, len = ref2.length; j < len; j++) {
          const file = ref2[j];
          const p = path.join(asarDir, 'a.asar', file);
          expect(() => {
            fs.lstatSync(p);
          }).to.throw(/ENOENT/);
        }
      });
    });

    describe('fs.lstat', function () {
      it('handles path with trailing slash correctly', function (done) {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2', 'file1');
        fs.lstat(p + '/', done);
      });

      it('returns information of root', function (done) {
        const p = path.join(asarDir, 'a.asar');
        fs.lstat(p, function (err, stats) {
          try {
            expect(err).to.be.null();
            expect(stats.isFile()).to.be.false();
            expect(stats.isDirectory()).to.be.true();
            expect(stats.isSymbolicLink()).to.be.false();
            expect(stats.size).to.equal(0);
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('returns information of root with stats as bigint', function (done) {
        const p = path.join(asarDir, 'a.asar');
        fs.lstat(p, { bigint: false }, function (err, stats) {
          try {
            expect(err).to.be.null();
            expect(stats.isFile()).to.be.false();
            expect(stats.isDirectory()).to.be.true();
            expect(stats.isSymbolicLink()).to.be.false();
            expect(stats.size).to.equal(0);
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('returns information of a normal file', function (done) {
        const p = path.join(asarDir, 'a.asar', 'link2', 'file1');
        fs.lstat(p, function (err, stats) {
          try {
            expect(err).to.be.null();
            expect(stats.isFile()).to.be.true();
            expect(stats.isDirectory()).to.be.false();
            expect(stats.isSymbolicLink()).to.be.false();
            expect(stats.size).to.equal(6);
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('returns information of a normal directory', function (done) {
        const p = path.join(asarDir, 'a.asar', 'dir1');
        fs.lstat(p, function (err, stats) {
          try {
            expect(err).to.be.null();
            expect(stats.isFile()).to.be.false();
            expect(stats.isDirectory()).to.be.true();
            expect(stats.isSymbolicLink()).to.be.false();
            expect(stats.size).to.equal(0);
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('returns information of a linked file', function (done) {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link1');
        fs.lstat(p, function (err, stats) {
          try {
            expect(err).to.be.null();
            expect(stats.isFile()).to.be.false();
            expect(stats.isDirectory()).to.be.false();
            expect(stats.isSymbolicLink()).to.be.true();
            expect(stats.size).to.equal(0);
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('returns information of a linked directory', function (done) {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2');
        fs.lstat(p, function (err, stats) {
          try {
            expect(err).to.be.null();
            expect(stats.isFile()).to.be.false();
            expect(stats.isDirectory()).to.be.false();
            expect(stats.isSymbolicLink()).to.be.true();
            expect(stats.size).to.equal(0);
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('throws ENOENT error when can not find file', function (done) {
        const p = path.join(asarDir, 'a.asar', 'file4');
        fs.lstat(p, function (err) {
          try {
            expect(err.code).to.equal('ENOENT');
            done();
          } catch (e) {
            done(e);
          }
        });
      });
    });

    describe('fs.promises.lstat', function () {
      it('handles path with trailing slash correctly', async function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2', 'file1');
        await fs.promises.lstat(p + '/');
      });

      it('returns information of root', async function () {
        const p = path.join(asarDir, 'a.asar');
        const stats = await fs.promises.lstat(p);
        expect(stats.isFile()).to.be.false();
        expect(stats.isDirectory()).to.be.true();
        expect(stats.isSymbolicLink()).to.be.false();
        expect(stats.size).to.equal(0);
      });

      it('returns information of root with stats as bigint', async function () {
        const p = path.join(asarDir, 'a.asar');
        const stats = await fs.promises.lstat(p, { bigint: false });
        expect(stats.isFile()).to.be.false();
        expect(stats.isDirectory()).to.be.true();
        expect(stats.isSymbolicLink()).to.be.false();
        expect(stats.size).to.equal(0);
      });

      it('returns information of a normal file', async function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'file1');
        const stats = await fs.promises.lstat(p);
        expect(stats.isFile()).to.be.true();
        expect(stats.isDirectory()).to.be.false();
        expect(stats.isSymbolicLink()).to.be.false();
        expect(stats.size).to.equal(6);
      });

      it('returns information of a normal directory', async function () {
        const p = path.join(asarDir, 'a.asar', 'dir1');
        const stats = await fs.promises.lstat(p);
        expect(stats.isFile()).to.be.false();
        expect(stats.isDirectory()).to.be.true();
        expect(stats.isSymbolicLink()).to.be.false();
        expect(stats.size).to.equal(0);
      });

      it('returns information of a linked file', async function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link1');
        const stats = await fs.promises.lstat(p);
        expect(stats.isFile()).to.be.false();
        expect(stats.isDirectory()).to.be.false();
        expect(stats.isSymbolicLink()).to.be.true();
        expect(stats.size).to.equal(0);
      });

      it('returns information of a linked directory', async function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2');
        const stats = await fs.promises.lstat(p);
        expect(stats.isFile()).to.be.false();
        expect(stats.isDirectory()).to.be.false();
        expect(stats.isSymbolicLink()).to.be.true();
        expect(stats.size).to.equal(0);
      });

      it('throws ENOENT error when can not find file', async function () {
        const p = path.join(asarDir, 'a.asar', 'file4');
        await expectToThrowErrorWithCode(() => fs.promises.lstat(p), 'ENOENT');
      });
    });

    describe('fs.realpathSync', () => {
      it('returns real path root', () => {
        const parent = fs.realpathSync(asarDir);
        const p = 'a.asar';
        const r = fs.realpathSync(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      it('returns real path of a normal file', () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'file1');
        const r = fs.realpathSync(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      it('returns real path of a normal directory', () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'dir1');
        const r = fs.realpathSync(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      it('returns real path of a linked file', () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'link2', 'link1');
        const r = fs.realpathSync(path.join(parent, p));
        expect(r).to.equal(path.join(parent, 'a.asar', 'file1'));
      });

      it('returns real path of a linked directory', () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'link2', 'link2');
        const r = fs.realpathSync(path.join(parent, p));
        expect(r).to.equal(path.join(parent, 'a.asar', 'dir1'));
      });

      it('returns real path of an unpacked file', () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('unpack.asar', 'a.txt');
        const r = fs.realpathSync(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      it('throws ENOENT error when can not find file', () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'not-exist');
        expect(() => {
          fs.realpathSync(path.join(parent, p));
        }).to.throw(/ENOENT/);
      });
    });

    describe('fs.realpathSync.native', () => {
      it('returns real path root', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = 'a.asar';
        const r = fs.realpathSync.native(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      it('returns real path of a normal file', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'file1');
        const r = fs.realpathSync.native(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      it('returns real path of a normal directory', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'dir1');
        const r = fs.realpathSync.native(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      it('returns real path of a linked file', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'link2', 'link1');
        const r = fs.realpathSync.native(path.join(parent, p));
        expect(r).to.equal(path.join(parent, 'a.asar', 'file1'));
      });

      it('returns real path of a linked directory', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'link2', 'link2');
        const r = fs.realpathSync.native(path.join(parent, p));
        expect(r).to.equal(path.join(parent, 'a.asar', 'dir1'));
      });

      it('returns real path of an unpacked file', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('unpack.asar', 'a.txt');
        const r = fs.realpathSync.native(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      it('throws ENOENT error when can not find file', () => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'not-exist');
        expect(() => {
          fs.realpathSync.native(path.join(parent, p));
        }).to.throw(/ENOENT/);
      });
    });

    describe('fs.realpath', () => {
      it('returns real path root', done => {
        const parent = fs.realpathSync(asarDir);
        const p = 'a.asar';
        fs.realpath(path.join(parent, p), (err, r) => {
          try {
            expect(err).to.be.null();
            expect(r).to.equal(path.join(parent, p));
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('returns real path of a normal file', done => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'file1');
        fs.realpath(path.join(parent, p), (err, r) => {
          try {
            expect(err).to.be.null();
            expect(r).to.equal(path.join(parent, p));
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('returns real path of a normal directory', done => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'dir1');
        fs.realpath(path.join(parent, p), (err, r) => {
          try {
            expect(err).to.be.null();
            expect(r).to.equal(path.join(parent, p));
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('returns real path of a linked file', done => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'link2', 'link1');
        fs.realpath(path.join(parent, p), (err, r) => {
          try {
            expect(err).to.be.null();
            expect(r).to.equal(path.join(parent, 'a.asar', 'file1'));
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('returns real path of a linked directory', done => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'link2', 'link2');
        fs.realpath(path.join(parent, p), (err, r) => {
          try {
            expect(err).to.be.null();
            expect(r).to.equal(path.join(parent, 'a.asar', 'dir1'));
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('returns real path of an unpacked file', done => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('unpack.asar', 'a.txt');
        fs.realpath(path.join(parent, p), (err, r) => {
          try {
            expect(err).to.be.null();
            expect(r).to.equal(path.join(parent, p));
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('throws ENOENT error when can not find file', done => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'not-exist');
        fs.realpath(path.join(parent, p), err => {
          try {
            expect(err.code).to.equal('ENOENT');
            done();
          } catch (e) {
            done(e);
          }
        });
      });
    });

    describe('fs.promises.realpath', () => {
      it('returns real path root', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = 'a.asar';
        const r = await fs.promises.realpath(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      it('returns real path of a normal file', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'file1');
        const r = await fs.promises.realpath(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      it('returns real path of a normal directory', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'dir1');
        const r = await fs.promises.realpath(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      it('returns real path of a linked file', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'link2', 'link1');
        const r = await fs.promises.realpath(path.join(parent, p));
        expect(r).to.equal(path.join(parent, 'a.asar', 'file1'));
      });

      it('returns real path of a linked directory', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'link2', 'link2');
        const r = await fs.promises.realpath(path.join(parent, p));
        expect(r).to.equal(path.join(parent, 'a.asar', 'dir1'));
      });

      it('returns real path of an unpacked file', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('unpack.asar', 'a.txt');
        const r = await fs.promises.realpath(path.join(parent, p));
        expect(r).to.equal(path.join(parent, p));
      });

      it('throws ENOENT error when can not find file', async () => {
        const parent = fs.realpathSync(asarDir);
        const p = path.join('a.asar', 'not-exist');
        await expectToThrowErrorWithCode(() => fs.promises.realpath(path.join(parent, p)), 'ENOENT');
      });
    });

    describe('fs.realpath.native', () => {
      it('returns real path root', done => {
        const parent = fs.realpathSync.native(asarDir);
        const p = 'a.asar';
        fs.realpath.native(path.join(parent, p), (err, r) => {
          try {
            expect(err).to.be.null();
            expect(r).to.equal(path.join(parent, p));
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('returns real path of a normal file', done => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'file1');
        fs.realpath.native(path.join(parent, p), (err, r) => {
          try {
            expect(err).to.be.null();
            expect(r).to.equal(path.join(parent, p));
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('returns real path of a normal directory', done => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'dir1');
        fs.realpath.native(path.join(parent, p), (err, r) => {
          try {
            expect(err).to.be.null();
            expect(r).to.equal(path.join(parent, p));
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('returns real path of a linked file', done => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'link2', 'link1');
        fs.realpath.native(path.join(parent, p), (err, r) => {
          try {
            expect(err).to.be.null();
            expect(r).to.equal(path.join(parent, 'a.asar', 'file1'));
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('returns real path of a linked directory', done => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'link2', 'link2');
        fs.realpath.native(path.join(parent, p), (err, r) => {
          try {
            expect(err).to.be.null();
            expect(r).to.equal(path.join(parent, 'a.asar', 'dir1'));
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('returns real path of an unpacked file', done => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('unpack.asar', 'a.txt');
        fs.realpath.native(path.join(parent, p), (err, r) => {
          try {
            expect(err).to.be.null();
            expect(r).to.equal(path.join(parent, p));
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('throws ENOENT error when can not find file', done => {
        const parent = fs.realpathSync.native(asarDir);
        const p = path.join('a.asar', 'not-exist');
        fs.realpath.native(path.join(parent, p), err => {
          try {
            expect(err.code).to.equal('ENOENT');
            done();
          } catch (e) {
            done(e);
          }
        });
      });
    });

    describe('fs.readdirSync', function () {
      it('reads dirs from root', function () {
        const p = path.join(asarDir, 'a.asar');
        const dirs = fs.readdirSync(p);
        expect(dirs).to.deep.equal(['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
      });

      it('reads dirs from a normal dir', function () {
        const p = path.join(asarDir, 'a.asar', 'dir1');
        const dirs = fs.readdirSync(p);
        expect(dirs).to.deep.equal(['file1', 'file2', 'file3', 'link1', 'link2']);
      });

      it('supports withFileTypes', function () {
        const p = path.join(asarDir, 'a.asar');
        const dirs = fs.readdirSync(p, { withFileTypes: true });
        for (const dir of dirs) {
          expect(dir instanceof fs.Dirent).to.be.true();
        }
        const names = dirs.map(a => a.name);
        expect(names).to.deep.equal(['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
      });

      it('supports withFileTypes for a deep directory', function () {
        const p = path.join(asarDir, 'a.asar', 'dir3');
        const dirs = fs.readdirSync(p, { withFileTypes: true });
        for (const dir of dirs) {
          expect(dir instanceof fs.Dirent).to.be.true();
        }
        const names = dirs.map(a => a.name);
        expect(names).to.deep.equal(['file1', 'file2', 'file3']);
      });

      it('reads dirs from a linked dir', function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2');
        const dirs = fs.readdirSync(p);
        expect(dirs).to.deep.equal(['file1', 'file2', 'file3', 'link1', 'link2']);
      });

      it('throws ENOENT error when can not find file', function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        expect(() => {
          fs.readdirSync(p);
        }).to.throw(/ENOENT/);
      });
    });

    describe('fs.readdir', function () {
      it('reads dirs from root', function (done) {
        const p = path.join(asarDir, 'a.asar');
        fs.readdir(p, function (err, dirs) {
          try {
            expect(err).to.be.null();
            expect(dirs).to.deep.equal(['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('supports withFileTypes', function (done) {
        const p = path.join(asarDir, 'a.asar');

        fs.readdir(p, { withFileTypes: true }, (err, dirs) => {
          try {
            expect(err).to.be.null();
            for (const dir of dirs) {
              expect(dir instanceof fs.Dirent).to.be.true();
            }

            const names = dirs.map(a => a.name);
            expect(names).to.deep.equal(['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('reads dirs from a normal dir', function (done) {
        const p = path.join(asarDir, 'a.asar', 'dir1');
        fs.readdir(p, function (err, dirs) {
          try {
            expect(err).to.be.null();
            expect(dirs).to.deep.equal(['file1', 'file2', 'file3', 'link1', 'link2']);
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('reads dirs from a linked dir', function (done) {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2');
        fs.readdir(p, function (err, dirs) {
          try {
            expect(err).to.be.null();
            expect(dirs).to.deep.equal(['file1', 'file2', 'file3', 'link1', 'link2']);
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('throws ENOENT error when can not find file', function (done) {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        fs.readdir(p, function (err) {
          try {
            expect(err.code).to.equal('ENOENT');
            done();
          } catch (e) {
            done(e);
          }
        });
      });
    });

    describe('fs.promises.readdir', function () {
      it('reads dirs from root', async function () {
        const p = path.join(asarDir, 'a.asar');
        const dirs = await fs.promises.readdir(p);
        expect(dirs).to.deep.equal(['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
      });

      it('supports withFileTypes', async function () {
        const p = path.join(asarDir, 'a.asar');
        const dirs = await fs.promises.readdir(p, { withFileTypes: true });
        for (const dir of dirs) {
          expect(dir instanceof fs.Dirent).to.be.true();
        }
        const names = dirs.map(a => a.name);
        expect(names).to.deep.equal(['dir1', 'dir2', 'dir3', 'file1', 'file2', 'file3', 'link1', 'link2', 'ping.js']);
      });

      it('reads dirs from a normal dir', async function () {
        const p = path.join(asarDir, 'a.asar', 'dir1');
        const dirs = await fs.promises.readdir(p);
        expect(dirs).to.deep.equal(['file1', 'file2', 'file3', 'link1', 'link2']);
      });

      it('reads dirs from a linked dir', async function () {
        const p = path.join(asarDir, 'a.asar', 'link2', 'link2');
        const dirs = await fs.promises.readdir(p);
        expect(dirs).to.deep.equal(['file1', 'file2', 'file3', 'link1', 'link2']);
      });

      it('throws ENOENT error when can not find file', async function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        await expectToThrowErrorWithCode(() => fs.promises.readdir(p), 'ENOENT');
      });
    });

    describe('fs.openSync', function () {
      it('opens a normal/linked/under-linked-directory file', function () {
        const ref2 = ['file1', 'link1', path.join('link2', 'file1')];
        for (let j = 0, len = ref2.length; j < len; j++) {
          const file = ref2[j];
          const p = path.join(asarDir, 'a.asar', file);
          const fd = fs.openSync(p, 'r');
          const buffer = Buffer.alloc(6);
          fs.readSync(fd, buffer, 0, 6, 0);
          expect(String(buffer).trim()).to.equal('file1');
          fs.closeSync(fd);
        }
      });

      it('throws ENOENT error when can not find file', function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        expect(() => {
          fs.openSync(p);
        }).to.throw(/ENOENT/);
      });
    });

    describe('fs.open', function () {
      it('opens a normal file', function (done) {
        const p = path.join(asarDir, 'a.asar', 'file1');
        fs.open(p, 'r', function (err, fd) {
          expect(err).to.be.null();
          const buffer = Buffer.alloc(6);
          fs.read(fd, buffer, 0, 6, 0, function (err) {
            expect(err).to.be.null();
            expect(String(buffer).trim()).to.equal('file1');
            fs.close(fd, done);
          });
        });
      });

      it('throws ENOENT error when can not find file', function (done) {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        fs.open(p, 'r', function (err) {
          try {
            expect(err.code).to.equal('ENOENT');
            done();
          } catch (e) {
            done(e);
          }
        });
      });
    });

    describe('fs.promises.open', function () {
      it('opens a normal file', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        const fh = await fs.promises.open(p, 'r');
        const buffer = Buffer.alloc(6);
        await fh.read(buffer, 0, 6, 0);
        expect(String(buffer).trim()).to.equal('file1');
        await fh.close();
      });

      it('throws ENOENT error when can not find file', async function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        await expectToThrowErrorWithCode(() => fs.promises.open(p, 'r'), 'ENOENT');
      });
    });

    describe('fs.mkdir', function () {
      it('throws error when calling inside asar archive', function (done) {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        fs.mkdir(p, function (err) {
          try {
            expect(err.code).to.equal('ENOTDIR');
            done();
          } catch (e) {
            done(e);
          }
        });
      });
    });

    describe('fs.promises.mkdir', function () {
      it('throws error when calling inside asar archive', async function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        await expectToThrowErrorWithCode(() => fs.promises.mkdir(p), 'ENOTDIR');
      });
    });

    describe('fs.mkdirSync', function () {
      it('throws error when calling inside asar archive', function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        expect(() => {
          fs.mkdirSync(p);
        }).to.throw(/ENOTDIR/);
      });
    });

    describe('fs.exists', function () {
      it('handles an existing file', function (done) {
        const p = path.join(asarDir, 'a.asar', 'file1');
        // eslint-disable-next-line
        fs.exists(p, function (exists) {
          try {
            expect(exists).to.be.true();
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('handles a non-existent file', function (done) {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        // eslint-disable-next-line
        fs.exists(p, function (exists) {
          try {
            expect(exists).to.be.false();
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('promisified version handles an existing file', (done) => {
        const p = path.join(asarDir, 'a.asar', 'file1');
        // eslint-disable-next-line
        util.promisify(fs.exists)(p).then(exists => {
          try {
            expect(exists).to.be.true();
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('promisified version handles a non-existent file', function (done) {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        // eslint-disable-next-line
        util.promisify(fs.exists)(p).then(exists => {
          try {
            expect(exists).to.be.false();
            done();
          } catch (e) {
            done(e);
          }
        });
      });
    });

    describe('fs.existsSync', function () {
      it('handles an existing file', function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        expect(fs.existsSync(p)).to.be.true();
      });

      it('handles a non-existent file', function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        expect(fs.existsSync(p)).to.be.false();
      });
    });

    describe('fs.access', function () {
      it('accesses a normal file', function (done) {
        const p = path.join(asarDir, 'a.asar', 'file1');
        fs.access(p, function (err) {
          try {
            expect(err).to.be.undefined();
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('throws an error when called with write mode', function (done) {
        const p = path.join(asarDir, 'a.asar', 'file1');
        fs.access(p, fs.constants.R_OK | fs.constants.W_OK, function (err) {
          try {
            expect(err.code).to.equal('EACCES');
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('throws an error when called on non-existent file', function (done) {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        fs.access(p, function (err) {
          try {
            expect(err.code).to.equal('ENOENT');
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('allows write mode for unpacked files', function (done) {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        fs.access(p, fs.constants.R_OK | fs.constants.W_OK, function (err) {
          try {
            expect(err).to.be.null();
            done();
          } catch (e) {
            done(e);
          }
        });
      });
    });

    describe('fs.promises.access', function () {
      it('accesses a normal file', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        await fs.promises.access(p);
      });

      it('throws an error when called with write mode', async function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        await expectToThrowErrorWithCode(() => fs.promises.access(p, fs.constants.R_OK | fs.constants.W_OK), 'EACCES');
      });

      it('throws an error when called on non-existent file', async function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        await expectToThrowErrorWithCode(() => fs.promises.access(p), 'ENOENT');
      });

      it('allows write mode for unpacked files', async function () {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        await fs.promises.access(p, fs.constants.R_OK | fs.constants.W_OK);
      });
    });

    describe('fs.accessSync', function () {
      it('accesses a normal file', function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        expect(() => {
          fs.accessSync(p);
        }).to.not.throw();
      });

      it('throws an error when called with write mode', function () {
        const p = path.join(asarDir, 'a.asar', 'file1');
        expect(() => {
          fs.accessSync(p, fs.constants.R_OK | fs.constants.W_OK);
        }).to.throw(/EACCES/);
      });

      it('throws an error when called on non-existent file', function () {
        const p = path.join(asarDir, 'a.asar', 'not-exist');
        expect(() => {
          fs.accessSync(p);
        }).to.throw(/ENOENT/);
      });

      it('allows write mode for unpacked files', function () {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        expect(() => {
          fs.accessSync(p, fs.constants.R_OK | fs.constants.W_OK);
        }).to.not.throw();
      });
    });

    describe('child_process.fork', function () {
      before(function () {
        if (!features.isRunAsNodeEnabled()) {
          this.skip();
        }
      });

      it('opens a normal js file', function (done) {
        const child = ChildProcess.fork(path.join(asarDir, 'a.asar', 'ping.js'));
        child.on('message', function (msg) {
          try {
            expect(msg).to.equal('message');
            done();
          } catch (e) {
            done(e);
          }
        });
        child.send('message');
      });

      it('supports asar in the forked js', function (done) {
        const file = path.join(asarDir, 'a.asar', 'file1');
        const child = ChildProcess.fork(path.join(fixtures, 'module', 'asar.js'));
        child.on('message', function (content) {
          try {
            expect(content).to.equal(fs.readFileSync(file).toString());
            done();
          } catch (e) {
            done(e);
          }
        });
        child.send(file);
      });
    });

    describe('child_process.exec', function () {
      const echo = path.join(asarDir, 'echo.asar', 'echo');

      it('should not try to extract the command if there is a reference to a file inside an .asar', function (done) {
        ChildProcess.exec('echo ' + echo + ' foo bar', function (error, stdout) {
          try {
            expect(error).to.be.null();
            expect(stdout.toString().replace(/\r/g, '')).to.equal(echo + ' foo bar\n');
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('can be promisified', () => {
        return util.promisify(ChildProcess.exec)('echo ' + echo + ' foo bar').then(({ stdout }) => {
          expect(stdout.toString().replace(/\r/g, '')).to.equal(echo + ' foo bar\n');
        });
      });
    });

    describe('child_process.execSync', function () {
      const echo = path.join(asarDir, 'echo.asar', 'echo');

      it('should not try to extract the command if there is a reference to a file inside an .asar', function (done) {
        try {
          const stdout = ChildProcess.execSync('echo ' + echo + ' foo bar');
          expect(stdout.toString().replace(/\r/g, '')).to.equal(echo + ' foo bar\n');
          done();
        } catch (e) {
          done(e);
        }
      });
    });

    ifdescribe(process.platform === 'darwin' && process.arch !== 'arm64')('child_process.execFile', function () {
      const execFile = ChildProcess.execFile;
      const execFileSync = ChildProcess.execFileSync;
      const echo = path.join(asarDir, 'echo.asar', 'echo');

      it('executes binaries', function (done) {
        execFile(echo, ['test'], function (error, stdout) {
          try {
            expect(error).to.be.null();
            expect(stdout).to.equal('test\n');
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('executes binaries without callback', function (done) {
        const process = execFile(echo, ['test']);
        process.on('close', function (code) {
          try {
            expect(code).to.equal(0);
            done();
          } catch (e) {
            done(e);
          }
        });
        process.on('error', function () {
          done('error');
        });
      });

      it('execFileSync executes binaries', function () {
        const output = execFileSync(echo, ['test']);
        expect(String(output)).to.equal('test\n');
      });

      it('can be promisified', () => {
        return util.promisify(ChildProcess.execFile)(echo, ['test']).then(({ stdout }) => {
          expect(stdout).to.equal('test\n');
        });
      });
    });

    describe('internalModuleReadJSON', function () {
      const { internalModuleReadJSON } = process.binding('fs');

      it('reads a normal file', function () {
        const file1 = path.join(asarDir, 'a.asar', 'file1');
        const [s1, c1] = internalModuleReadJSON(file1);
        expect([s1.toString().trim(), c1]).to.eql(['file1', true]);

        const file2 = path.join(asarDir, 'a.asar', 'file2');
        const [s2, c2] = internalModuleReadJSON(file2);
        expect([s2.toString().trim(), c2]).to.eql(['file2', true]);

        const file3 = path.join(asarDir, 'a.asar', 'file3');
        const [s3, c3] = internalModuleReadJSON(file3);
        expect([s3.toString().trim(), c3]).to.eql(['file3', true]);
      });

      it('reads a normal file with unpacked files', function () {
        const p = path.join(asarDir, 'unpack.asar', 'a.txt');
        const [s, c] = internalModuleReadJSON(p);
        expect([s.toString().trim(), c]).to.eql(['a', true]);
      });
    });

    describe('util.promisify', function () {
      it('can promisify all fs functions', function () {
        const originalFs = require('original-fs');
        const { hasOwnProperty } = Object.prototype;

        for (const [propertyName, originalValue] of Object.entries(originalFs)) {
          // Some properties exist but have a value of `undefined` on some platforms.
          // E.g. `fs.lchmod`, which in only available on MacOS, see
          // https://nodejs.org/docs/latest-v10.x/api/fs.html#fs_fs_lchmod_path_mode_callback
          // Also check for `null`s, `hasOwnProperty()` can't handle them.
          if (typeof originalValue === 'undefined' || originalValue === null) continue;

          if (hasOwnProperty.call(originalValue, util.promisify.custom)) {
            expect(fs).to.have.own.property(propertyName)
              .that.has.own.property(util.promisify.custom);
          }
        }
      });
    });

    describe('process.noAsar', function () {
      const errorName = process.platform === 'win32' ? 'ENOENT' : 'ENOTDIR';

      beforeEach(function () {
        process.noAsar = true;
      });

      afterEach(function () {
        process.noAsar = false;
      });

      it('disables asar support in sync API', function () {
        const file = path.join(asarDir, 'a.asar', 'file1');
        const dir = path.join(asarDir, 'a.asar', 'dir1');
        expect(() => {
          fs.readFileSync(file);
        }).to.throw(new RegExp(errorName));
        expect(() => {
          fs.lstatSync(file);
        }).to.throw(new RegExp(errorName));
        expect(() => {
          fs.realpathSync(file);
        }).to.throw(new RegExp(errorName));
        expect(() => {
          fs.readdirSync(dir);
        }).to.throw(new RegExp(errorName));
      });

      it('disables asar support in async API', function (done) {
        const file = path.join(asarDir, 'a.asar', 'file1');
        const dir = path.join(asarDir, 'a.asar', 'dir1');
        fs.readFile(file, function (error) {
          expect(error.code).to.equal(errorName);
          fs.lstat(file, function (error) {
            expect(error.code).to.equal(errorName);
            fs.realpath(file, function (error) {
              expect(error.code).to.equal(errorName);
              fs.readdir(dir, function (error) {
                expect(error.code).to.equal(errorName);
                done();
              });
            });
          });
        });
      });

      it('disables asar support in promises API', async function () {
        const file = path.join(asarDir, 'a.asar', 'file1');
        const dir = path.join(asarDir, 'a.asar', 'dir1');
        await expect(fs.promises.readFile(file)).to.be.eventually.rejectedWith(Error, new RegExp(errorName));
        await expect(fs.promises.lstat(file)).to.be.eventually.rejectedWith(Error, new RegExp(errorName));
        await expect(fs.promises.realpath(file)).to.be.eventually.rejectedWith(Error, new RegExp(errorName));
        await expect(fs.promises.readdir(dir)).to.be.eventually.rejectedWith(Error, new RegExp(errorName));
      });

      it('treats *.asar as normal file', function () {
        const originalFs = require('original-fs');
        const asar = path.join(asarDir, 'a.asar');
        const content1 = fs.readFileSync(asar);
        const content2 = originalFs.readFileSync(asar);
        expect(content1.compare(content2)).to.equal(0);
        expect(() => {
          fs.readdirSync(asar);
        }).to.throw(/ENOTDIR/);
      });

      it('is reset to its original value when execSync throws an error', function () {
        process.noAsar = false;
        expect(() => {
          ChildProcess.execSync(path.join(__dirname, 'does-not-exist.txt'));
        }).to.throw();
        expect(process.noAsar).to.be.false();
      });
    });

    describe('process.env.ELECTRON_NO_ASAR', function () {
      before(function () {
        if (!features.isRunAsNodeEnabled()) {
          this.skip();
        }
      });

      it('disables asar support in forked processes', function (done) {
        const forked = ChildProcess.fork(path.join(__dirname, 'fixtures', 'module', 'no-asar.js'), [], {
          env: {
            ELECTRON_NO_ASAR: true
          }
        });
        forked.on('message', function (stats) {
          try {
            expect(stats.isFile).to.be.true();
            expect(stats.size).to.equal(3458);
            done();
          } catch (e) {
            done(e);
          }
        });
      });

      it('disables asar support in spawned processes', function (done) {
        const spawned = ChildProcess.spawn(process.execPath, [path.join(__dirname, 'fixtures', 'module', 'no-asar.js')], {
          env: {
            ELECTRON_NO_ASAR: true,
            ELECTRON_RUN_AS_NODE: true
          }
        });

        let output = '';
        spawned.stdout.on('data', function (data) {
          output += data;
        });
        spawned.stdout.on('close', function () {
          try {
            const stats = JSON.parse(output);
            expect(stats.isFile).to.be.true();
            expect(stats.size).to.equal(3458);
            done();
          } catch (e) {
            done(e);
          }
        });
      });
    });
  });

  describe('asar protocol', function () {
    it('can request a file in package', function (done) {
      const p = path.resolve(asarDir, 'a.asar', 'file1');
      $.get('file://' + p, function (data) {
        try {
          expect(data.trim()).to.equal('file1');
          done();
        } catch (e) {
          done(e);
        }
      });
    });

    it('can request a file in package with unpacked files', function (done) {
      const p = path.resolve(asarDir, 'unpack.asar', 'a.txt');
      $.get('file://' + p, function (data) {
        try {
          expect(data.trim()).to.equal('a');
          done();
        } catch (e) {
          done(e);
        }
      });
    });

    it('can request a linked file in package', function (done) {
      const p = path.resolve(asarDir, 'a.asar', 'link2', 'link1');
      $.get('file://' + p, function (data) {
        try {
          expect(data.trim()).to.equal('file1');
          done();
        } catch (e) {
          done(e);
        }
      });
    });

    it('can request a file in filesystem', function (done) {
      const p = path.resolve(asarDir, 'file');
      $.get('file://' + p, function (data) {
        try {
          expect(data.trim()).to.equal('file');
          done();
        } catch (e) {
          done(e);
        }
      });
    });

    it('gets 404 when file is not found', function (done) {
      const p = path.resolve(asarDir, 'a.asar', 'no-exist');
      $.ajax({
        url: 'file://' + p,
        error: function (err) {
          try {
            expect(err.status).to.equal(404);
            done();
          } catch (e) {
            done(e);
          }
        }
      });
    });
  });

  describe('original-fs module', function () {
    const originalFs = require('original-fs');

    it('treats .asar as file', function () {
      const file = path.join(asarDir, 'a.asar');
      const stats = originalFs.statSync(file);
      expect(stats.isFile()).to.be.true();
    });

    ifit(features.isRunAsNodeEnabled())('is available in forked scripts', async function () {
      const child = ChildProcess.fork(path.join(fixtures, 'module', 'original-fs.js'));
      const message = emittedOnce(child, 'message');
      child.send('message');
      const [msg] = await message;
      expect(msg).to.equal('object');
    });

    it('can be used with streams', () => {
      originalFs.createReadStream(path.join(asarDir, 'a.asar'));
    });

    it('can recursively delete a directory with an asar file in it', () => {
      const deleteDir = path.join(asarDir, 'deleteme');
      fs.mkdirSync(deleteDir);

      originalFs.rmdirSync(deleteDir, { recursive: true });

      expect(fs.existsSync(deleteDir)).to.be.false();
    });

    it('has the same APIs as fs', function () {
      expect(Object.keys(require('fs'))).to.deep.equal(Object.keys(require('original-fs')));
      expect(Object.keys(require('fs').promises)).to.deep.equal(Object.keys(require('original-fs').promises));
    });
  });

  describe('graceful-fs module', function () {
    const gfs = require('graceful-fs');

    it('recognize asar archvies', function () {
      const p = path.join(asarDir, 'a.asar', 'link1');
      expect(gfs.readFileSync(p).toString().trim()).to.equal('file1');
    });
    it('does not touch global fs object', function () {
      expect(fs.readdir).to.not.equal(gfs.readdir);
    });
  });

  describe('mkdirp module', function () {
    const mkdirp = require('mkdirp');

    it('throws error when calling inside asar archive', function () {
      const p = path.join(asarDir, 'a.asar', 'not-exist');
      expect(() => {
        mkdirp.sync(p);
      }).to.throw(/ENOTDIR/);
    });
  });

  describe('native-image', function () {
    it('reads image from asar archive', function () {
      const p = path.join(asarDir, 'logo.asar', 'logo.png');
      const logo = nativeImage.createFromPath(p);
      expect(logo.getSize()).to.deep.equal({
        width: 55,
        height: 55
      });
    });

    it('reads image from asar archive with unpacked files', function () {
      const p = path.join(asarDir, 'unpack.asar', 'atom.png');
      const logo = nativeImage.createFromPath(p);
      expect(logo.getSize()).to.deep.equal({
        width: 1024,
        height: 1024
      });
    });
  });
});
