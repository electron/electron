import { BrowserWindow, Session, session } from 'electron/main';

import { expect } from 'chai';
import * as path from 'node:path';
import * as fs from 'node:fs/promises';
import * as http from 'node:http';
import { closeWindow } from './lib/window-helpers';
import { ifit, ifdescribe, listen } from './lib/spec-helpers';
import { once } from 'node:events';
import { setTimeout } from 'node:timers/promises';

const features = process._linkedBinding('electron_common_features');
const v8Util = process._linkedBinding('electron_common_v8_util');

ifdescribe(features.isBuiltinSpellCheckerEnabled())('spellchecker', function () {
  this.timeout((process.env.IS_ASAN ? 200 : 20) * 1000);

  let w: BrowserWindow;

  async function rightClick () {
    const contextMenuPromise = once(w.webContents, 'context-menu');
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
    const timeout = (process.env.IS_ASAN ? 180 : 10) * 1000;
    let contextMenuParams = await rightClick();
    while (!fn(contextMenuParams) && (Date.now() - now < timeout)) {
      await setTimeout(100);
      contextMenuParams = await rightClick();
    }
    return contextMenuParams;
  }

  // Setup a server to download hunspell dictionary.
  const server = http.createServer(async (req, res) => {
    // The provided is minimal dict for testing only, full list of words can
    // be found at src/third_party/hunspell_dictionaries/xx_XX.dic.
    try {
      const data = await fs.readFile(path.join(__dirname, '/../../third_party/hunspell_dictionaries/xx-XX-3-0.bdic'));
      res.writeHead(200);
      res.end(data);
    } catch (err) {
      console.error('Failed to read dictionary file');
      res.writeHead(404);
      res.end(JSON.stringify(err));
    }
  });
  let serverUrl: string;
  before(async () => {
    serverUrl = (await listen(server)).url;
  });
  after(() => server.close());

  const fixtures = path.resolve(__dirname, 'fixtures');
  const preload = path.join(fixtures, 'module', 'preload-electron.js');

  const generateSpecs = (description: string, sandbox: boolean) => {
    describe(description, () => {
      beforeEach(async () => {
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            partition: `unique-spell-${Date.now()}`,
            contextIsolation: false,
            preload,
            sandbox
          }
        });
        w.webContents.session.setSpellCheckerDictionaryDownloadURL(serverUrl);
        w.webContents.session.setSpellCheckerLanguages(['en-US']);
        await w.loadFile(path.resolve(__dirname, './fixtures/chromium/spellchecker.html'));
      });

      afterEach(async () => {
        await closeWindow(w);
      });

      // Context menu test can not run on Windows.
      const shouldRun = process.platform !== 'win32';

      ifit(shouldRun)('should detect correctly spelled words as correct', async () => {
        await w.webContents.executeJavaScript('document.body.querySelector("textarea").value = "typography"');
        await w.webContents.executeJavaScript('document.body.querySelector("textarea").focus()');
        const contextMenuParams = await rightClickUntil((contextMenuParams) => contextMenuParams.selectionText.length > 0);
        expect(contextMenuParams.misspelledWord).to.eq('');
        expect(contextMenuParams.dictionarySuggestions).to.have.lengthOf(0);
      });

      ifit(shouldRun)('should detect incorrectly spelled words as incorrect', async () => {
        await w.webContents.executeJavaScript('document.body.querySelector("textarea").value = "typograpy"');
        await w.webContents.executeJavaScript('document.body.querySelector("textarea").focus()');
        const contextMenuParams = await rightClickUntil((contextMenuParams) => contextMenuParams.misspelledWord.length > 0);
        expect(contextMenuParams.misspelledWord).to.eq('typograpy');
        expect(contextMenuParams.dictionarySuggestions).to.have.length.of.at.least(1);
      });

      ifit(shouldRun)('should detect incorrectly spelled words as incorrect after disabling all languages and re-enabling', async () => {
        w.webContents.session.setSpellCheckerLanguages([]);
        await setTimeout(500);
        w.webContents.session.setSpellCheckerLanguages(['en-US']);
        await w.webContents.executeJavaScript('document.body.querySelector("textarea").value = "typograpy"');
        await w.webContents.executeJavaScript('document.body.querySelector("textarea").focus()');
        const contextMenuParams = await rightClickUntil((contextMenuParams) => contextMenuParams.misspelledWord.length > 0);
        expect(contextMenuParams.misspelledWord).to.eq('typograpy');
        expect(contextMenuParams.dictionarySuggestions).to.have.length.of.at.least(1);
      });

      ifit(shouldRun)('should expose webFrame spellchecker correctly', async () => {
        await w.webContents.executeJavaScript('document.body.querySelector("textarea").value = "typograpy"');
        await w.webContents.executeJavaScript('document.body.querySelector("textarea").focus()');
        await rightClickUntil((contextMenuParams) => contextMenuParams.misspelledWord.length > 0);

        const callWebFrameFn = (expr: string) => w.webContents.executeJavaScript(`electron.webFrame.${expr}`);

        expect(await callWebFrameFn('isWordMisspelled("typography")')).to.equal(false);
        expect(await callWebFrameFn('isWordMisspelled("typograpy")')).to.equal(true);
        expect(await callWebFrameFn('getWordSuggestions("typography")')).to.be.empty();
        expect(await callWebFrameFn('getWordSuggestions("typograpy")')).to.not.be.empty();
      });

      describe('spellCheckerEnabled', () => {
        it('is enabled by default', async () => {
          expect(w.webContents.session.spellCheckerEnabled).to.be.true();
        });

        ifit(shouldRun)('can be dynamically changed', async () => {
          await w.webContents.executeJavaScript('document.body.querySelector("textarea").value = "typograpy"');
          await w.webContents.executeJavaScript('document.body.querySelector("textarea").focus()');
          await rightClickUntil((contextMenuParams) => contextMenuParams.misspelledWord.length > 0);

          const callWebFrameFn = (expr: string) => w.webContents.executeJavaScript(`electron.webFrame.${expr}`);

          w.webContents.session.spellCheckerEnabled = false;
          v8Util.runUntilIdle();
          expect(w.webContents.session.spellCheckerEnabled).to.be.false();
          // spellCheckerEnabled is sent to renderer asynchronously and there is
          // no event notifying when it is finished, so wait a little while to
          // ensure the setting has been changed in renderer.
          await setTimeout(500);
          expect(await callWebFrameFn('isWordMisspelled("typograpy")')).to.equal(false);

          w.webContents.session.spellCheckerEnabled = true;
          v8Util.runUntilIdle();
          expect(w.webContents.session.spellCheckerEnabled).to.be.true();
          await setTimeout(500);
          expect(await callWebFrameFn('isWordMisspelled("typograpy")')).to.equal(true);
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

        describe('ses.setSpellCheckerLanguages', () => {
          const isMac = process.platform === 'darwin';

          ifit(isMac)('should be a no-op when setSpellCheckerLanguages is called on macOS', () => {
            expect(() => {
              w.webContents.session.setSpellCheckerLanguages(['i-am-a-nonexistent-language']);
            }).to.not.throw();
          });

          ifit(!isMac)('should throw when a bad language is passed', () => {
            expect(() => {
              w.webContents.session.setSpellCheckerLanguages(['i-am-a-nonexistent-language']);
            }).to.throw(/Invalid language code provided: "i-am-a-nonexistent-language" is not a valid language code/);
          });

          ifit(!isMac)('should not throw when a recognized language is passed', () => {
            expect(() => {
              w.webContents.session.setSpellCheckerLanguages(['es']);
            }).to.not.throw();
          });
        });

        describe('SetSpellCheckerDictionaryDownloadURL', () => {
          const isMac = process.platform === 'darwin';

          ifit(isMac)('should be a no-op when a bad url is passed on macOS', () => {
            expect(() => {
              w.webContents.session.setSpellCheckerDictionaryDownloadURL('i-am-not-a-valid-url');
            }).to.not.throw();
          });

          ifit(!isMac)('should throw when a bad url is passed', () => {
            expect(() => {
              w.webContents.session.setSpellCheckerDictionaryDownloadURL('i-am-not-a-valid-url');
            }).to.throw(/The URL you provided to setSpellCheckerDictionaryDownloadURL is not a valid URL/);
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
  };

  generateSpecs('without sandbox', false);
  generateSpecs('with sandbox', true);
});
