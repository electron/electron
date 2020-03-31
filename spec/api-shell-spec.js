const { expect } = require('chai');

const fs = require('fs');
const path = require('path');
const os = require('os');
const http = require('http');
const { shell } = require('electron');

describe('shell module', () => {
  const fixtures = path.resolve(__dirname, 'fixtures');
  const shortcutOptions = {
    target: 'C:\\target',
    description: 'description',
    cwd: 'cwd',
    args: 'args',
    appUserModelId: 'appUserModelId',
    icon: 'icon',
    iconIndex: 1
  };

  describe('shell.readShortcutLink(shortcutPath)', () => {
    beforeEach(function () {
      if (process.platform !== 'win32') this.skip();
    });

    it('throws when failed', () => {
      expect(() => {
        shell.readShortcutLink('not-exist');
      }).to.throw('Failed to read shortcut link');
    });

    it('reads all properties of a shortcut', () => {
      const shortcut = shell.readShortcutLink(path.join(fixtures, 'assets', 'shortcut.lnk'));
      expect(shortcut).to.deep.equal(shortcutOptions);
    });
  });

  describe('shell.writeShortcutLink(shortcutPath[, operation], options)', () => {
    beforeEach(function () {
      if (process.platform !== 'win32') this.skip();
    });

    const tmpShortcut = path.join(os.tmpdir(), `${Date.now()}.lnk`);

    afterEach(() => {
      fs.unlinkSync(tmpShortcut);
    });

    it('writes the shortcut', () => {
      expect(shell.writeShortcutLink(tmpShortcut, { target: 'C:\\' })).to.be.true();
      expect(fs.existsSync(tmpShortcut)).to.be.true();
    });

    it('correctly sets the fields', () => {
      expect(shell.writeShortcutLink(tmpShortcut, shortcutOptions)).to.be.true();
      expect(shell.readShortcutLink(tmpShortcut)).to.deep.equal(shortcutOptions);
    });

    it('updates the shortcut', () => {
      expect(shell.writeShortcutLink(tmpShortcut, 'update', shortcutOptions)).to.be.false();
      expect(shell.writeShortcutLink(tmpShortcut, 'create', shortcutOptions)).to.be.true();
      expect(shell.readShortcutLink(tmpShortcut)).to.deep.equal(shortcutOptions);
      const change = { target: 'D:\\' };
      expect(shell.writeShortcutLink(tmpShortcut, 'update', change)).to.be.true();
      expect(shell.readShortcutLink(tmpShortcut)).to.deep.equal(Object.assign(shortcutOptions, change));
    });

    it('replaces the shortcut', () => {
      expect(shell.writeShortcutLink(tmpShortcut, 'replace', shortcutOptions)).to.be.false();
      expect(shell.writeShortcutLink(tmpShortcut, 'create', shortcutOptions)).to.be.true();
      expect(shell.readShortcutLink(tmpShortcut)).to.deep.equal(shortcutOptions);
      const change = {
        target: 'D:\\',
        description: 'description2',
        cwd: 'cwd2',
        args: 'args2',
        appUserModelId: 'appUserModelId2',
        icon: 'icon2',
        iconIndex: 2
      };
      expect(shell.writeShortcutLink(tmpShortcut, 'replace', change)).to.be.true();
      expect(shell.readShortcutLink(tmpShortcut)).to.deep.equal(change);
    });
  });
});
