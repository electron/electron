import { expect } from 'chai';
import { BrowserWindow, Session, session } from 'electron';
import { ifdescribe } from './spec-helpers';
import { closeWindow } from './window-helpers';

const features = process.electronBinding('features');

ifdescribe(features.isBuiltinSpellCheckerEnabled())('spellchecker', () => {
  let w: BrowserWindow;

  beforeEach(async () => {
    w = new BrowserWindow({
      show: false
    });
  });

  afterEach(async () => {
    await closeWindow(w);
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
        ses.destroy();
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
