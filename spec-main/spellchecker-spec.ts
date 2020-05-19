import { BrowserWindow, Session, session } from 'electron/main';

import { expect } from 'chai';
import * as path from 'path';
import { closeWindow } from './window-helpers';
import { emittedOnce } from './events-helpers';
import { ifit, ifdescribe } from './spec-helpers';

const features = process.electronBinding('features');

ifdescribe(features.isBuiltinSpellCheckerEnabled())('spellchecker', () => {
  let w: BrowserWindow;

  beforeEach(async () => {
    w = new BrowserWindow({
      show: false
    });
    await w.loadFile(path.resolve(__dirname, './fixtures/chromium/spellchecker.html'));
  });

  afterEach(async () => {
    await closeWindow(w);
  });

  // Context menu test can not run on Windows, and it is not reliable on ARM
  // CI machines.
  const shouldRun = process.platform !== 'win32' &&
                    process.arch !== 'arm' &&
                    process.arch !== 'arm64';

  ifit(shouldRun)('should detect correctly spelled words as correct', async () => {
    await w.webContents.executeJavaScript('document.body.querySelector("textarea").value = "Beautiful and lovely"');
    await w.webContents.executeJavaScript('document.body.querySelector("textarea").focus()');
    const contextMenuPromise = emittedOnce(w.webContents, 'context-menu');
    // Wait for spellchecker to load
    await new Promise(resolve => setTimeout(resolve, 500));
    w.webContents.sendInputEvent({
      type: 'mouseDown',
      button: 'right',
      x: 43,
      y: 42
    });
    const contextMenuParams: Electron.ContextMenuParams = (await contextMenuPromise)[1];
    expect(contextMenuParams.misspelledWord).to.eq('');
    expect(contextMenuParams.dictionarySuggestions).to.have.lengthOf(0);
  });

  ifit(shouldRun)('should detect incorrectly spelled words as incorrect', async () => {
    await w.webContents.executeJavaScript('document.body.querySelector("textarea").value = "Beautifulllll asd asd"');
    await w.webContents.executeJavaScript('document.body.querySelector("textarea").focus()');
    const contextMenuPromise = emittedOnce(w.webContents, 'context-menu');
    // Wait for spellchecker to load
    await new Promise(resolve => setTimeout(resolve, 500));
    w.webContents.sendInputEvent({
      type: 'mouseDown',
      button: 'right',
      x: 43,
      y: 42
    });
    const contextMenuParams: Electron.ContextMenuParams = (await contextMenuPromise)[1];
    expect(contextMenuParams.misspelledWord).to.eq('Beautifulllll');
    expect(contextMenuParams.dictionarySuggestions).to.have.length.of.at.least(1);
  });

  describe('custom dictionary word list API', () => {
    let ses: Session;

    beforeEach(async () => {
      // ensure a new session runs on each test run
      ses = session.fromPartition(`persist:customdictionary-test-${Date.now()}`);
    });

    afterEach(async () => {
      if (ses) {
        await ses.clearStorageData();
        ses = null as any;
      }
    });

    describe('ses.listWordsFromSpellCheckerDictionary', () => {
      it('should successfully list words in custom dictionary', async () => {
        const words = ['foo', 'bar', 'baz'];
        const results = words.map(word => ses.addWordToSpellCheckerDictionary(word));
        expect(results).to.eql([true, true, true]);

        const wordList = await ses.listWordsInSpellCheckerDictionary();
        expect(wordList).to.have.deep.members(words);
      });

      it('should return an empty array if no words are added', async () => {
        const wordList = await ses.listWordsInSpellCheckerDictionary();
        expect(wordList).to.have.length(0);
      });
    });

    describe('ses.addWordToSpellCheckerDictionary', () => {
      it('should successfully add word to custom dictionary', async () => {
        const result = ses.addWordToSpellCheckerDictionary('foobar');
        expect(result).to.equal(true);
        const wordList = await ses.listWordsInSpellCheckerDictionary();
        expect(wordList).to.eql(['foobar']);
      });

      it('should fail for an empty string', async () => {
        const result = ses.addWordToSpellCheckerDictionary('');
        expect(result).to.equal(false);
        const wordList = await ses.listWordsInSpellCheckerDictionary;
        expect(wordList).to.have.length(0);
      });

      // remove API will always return false because we can't add words
      it('should fail for non-persistent sessions', async () => {
        const tempSes = session.fromPartition('temporary');
        const result = tempSes.addWordToSpellCheckerDictionary('foobar');
        expect(result).to.equal(false);
      });
    });

    describe('ses.removeWordFromSpellCheckerDictionary', () => {
      it('should successfully remove words to custom dictionary', async () => {
        const result1 = ses.addWordToSpellCheckerDictionary('foobar');
        expect(result1).to.equal(true);
        const wordList1 = await ses.listWordsInSpellCheckerDictionary();
        expect(wordList1).to.eql(['foobar']);
        const result2 = ses.removeWordFromSpellCheckerDictionary('foobar');
        expect(result2).to.equal(true);
        const wordList2 = await ses.listWordsInSpellCheckerDictionary();
        expect(wordList2).to.have.length(0);
      });

      it('should fail for words not in custom dictionary', () => {
        const result2 = ses.removeWordFromSpellCheckerDictionary('foobar');
        expect(result2).to.equal(false);
      });
    });
  });
});
