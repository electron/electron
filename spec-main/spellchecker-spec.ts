import { BrowserWindow, Session, session } from 'electron/main';

import { expect } from 'chai';
import * as path from 'path';
import { closeWindow } from './window-helpers';
import { emittedOnce } from './events-helpers';
import { ifit, ifdescribe, delay } from './spec-helpers';

const features = process._linkedBinding('electron_common_features');
const v8Util = process._linkedBinding('electron_common_v8_util');

ifdescribe(features.isBuiltinSpellCheckerEnabled())('spellchecker', function () {
  // TODO(zcbenz): Spellchecker loads really slow on ASan, we should provide
  // a small testing dictionary to make the tests load faster.
  this.timeout((process.env.IS_ASAN ? 700 : 20) * 1000);

  let w: BrowserWindow;

  async function rightClick () {
    const contextMenuPromise = emittedOnce(w.webContents, 'context-menu');
    w.webContents.sendInputEvent({
      type: 'mouseDown',
      button: 'right',
      x: 43,
      y: 42
    });
    return (await contextMenuPromise)[1] as Electron.ContextMenuParams;
  }

  // When the page is just loaded, the spellchecker might not be ready yet. Since
  // there is no event to know the state of spellchecker, the only reliable way
  // to detect spellchecker is to keep checking with a busy loop.
  async function rightClickUntil (fn: (params: Electron.ContextMenuParams) => boolean) {
    const now = Date.now();
    const timeout = (process.env.IS_ASAN ? 600 : 10) * 1000;
    let contextMenuParams = await rightClick();
    while (!fn(contextMenuParams) && (Date.now() - now < timeout)) {
      await delay(100);
      contextMenuParams = await rightClick();
    }
    return contextMenuParams;
  }

  beforeEach(async () => {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: true,
        partition: `unique-spell-${Date.now()}`,
        contextIsolation: false
      }
    });
    w.webContents.session.setSpellCheckerLanguages(['en-US']);
    await w.loadFile(path.resolve(__dirname, './fixtures/chromium/spellchecker.html'));
  });

  afterEach(async () => {
    await closeWindow(w);
  });

  // Context menu test can not run on Windows.
  const shouldRun = process.platform !== 'win32';

  ifit(shouldRun)('should detect correctly spelled words as correct', async () => {
    await w.webContents.executeJavaScript('document.body.querySelector("textarea").value = "Beautiful and lovely"');
    await w.webContents.executeJavaScript('document.body.querySelector("textarea").focus()');
    const contextMenuParams = await rightClickUntil((contextMenuParams) => contextMenuParams.selectionText.length > 0);
    expect(contextMenuParams.misspelledWord).to.eq('');
    expect(contextMenuParams.dictionarySuggestions).to.have.lengthOf(0);
  });

  ifit(shouldRun)('should detect incorrectly spelled words as incorrect', async () => {
    await w.webContents.executeJavaScript('document.body.querySelector("textarea").value = "Beautifulllll asd asd"');
    await w.webContents.executeJavaScript('document.body.querySelector("textarea").focus()');
    const contextMenuParams = await rightClickUntil((contextMenuParams) => contextMenuParams.misspelledWord.length > 0);
    expect(contextMenuParams.misspelledWord).to.eq('Beautifulllll');
    expect(contextMenuParams.dictionarySuggestions).to.have.length.of.at.least(1);
  });

  ifit(shouldRun)('should detect incorrectly spelled words as incorrect after disabling all languages and re-enabling', async () => {
    w.webContents.session.setSpellCheckerLanguages([]);
    await delay(500);
    w.webContents.session.setSpellCheckerLanguages(['en-US']);
    await w.webContents.executeJavaScript('document.body.querySelector("textarea").value = "Beautifulllll asd asd"');
    await w.webContents.executeJavaScript('document.body.querySelector("textarea").focus()');
    const contextMenuParams = await rightClickUntil((contextMenuParams) => contextMenuParams.misspelledWord.length > 0);
    expect(contextMenuParams.misspelledWord).to.eq('Beautifulllll');
    expect(contextMenuParams.dictionarySuggestions).to.have.length.of.at.least(1);
  });

  ifit(shouldRun)('should expose webFrame spellchecker correctly', async () => {
    await w.webContents.executeJavaScript('document.body.querySelector("textarea").value = "Beautifulllll asd asd"');
    await w.webContents.executeJavaScript('document.body.querySelector("textarea").focus()');
    await rightClickUntil((contextMenuParams) => contextMenuParams.misspelledWord.length > 0);

    const callWebFrameFn = (expr: string) => w.webContents.executeJavaScript('require("electron").webFrame.' + expr);

    expect(await callWebFrameFn('isWordMisspelled("test")')).to.equal(false);
    expect(await callWebFrameFn('isWordMisspelled("testt")')).to.equal(true);
    expect(await callWebFrameFn('getWordSuggestions("test")')).to.be.empty();
    expect(await callWebFrameFn('getWordSuggestions("testt")')).to.not.be.empty();
  });

  describe('spellCheckerEnabled', () => {
    it('is enabled by default', async () => {
      expect(w.webContents.session.spellCheckerEnabled).to.be.true();
    });

    ifit(shouldRun)('can be dynamically changed', async () => {
      await w.webContents.executeJavaScript('document.body.querySelector("textarea").value = "Beautifulllll asd asd"');
      await w.webContents.executeJavaScript('document.body.querySelector("textarea").focus()');
      await rightClickUntil((contextMenuParams) => contextMenuParams.misspelledWord.length > 0);

      const callWebFrameFn = (expr: string) => w.webContents.executeJavaScript('require("electron").webFrame.' + expr);

      w.webContents.session.spellCheckerEnabled = false;
      v8Util.runUntilIdle();
      expect(w.webContents.session.spellCheckerEnabled).to.be.false();
      expect(await callWebFrameFn('isWordMisspelled("testt")')).to.equal(false);

      w.webContents.session.spellCheckerEnabled = true;
      v8Util.runUntilIdle();
      expect(w.webContents.session.spellCheckerEnabled).to.be.true();
      expect(await callWebFrameFn('isWordMisspelled("testt")')).to.equal(true);
    });
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
