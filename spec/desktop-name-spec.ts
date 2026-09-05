import { expect } from 'chai';

import * as path from 'node:path';

import { defaultDesktopName } from '../lib/browser/desktop-name';
import { ifdescribe } from './lib/spec-helpers';

ifdescribe(process.platform === 'linux')('defaultDesktopName', () => {
  it("derives an appropriate .desktop name from the app's human readable name", () => {
    const fallback = `${path.basename(process.execPath)}.desktop`;
    const cases: Array<[string | undefined, string]> = [
      ['Word', 'word.desktop'],
      ['Two Words', 'two-words.desktop'],
      ['Three Word Phrase', 'three-word-phrase.desktop'],
      ['Version 7', 'version-7.desktop'],
      ['already-hyphenated', 'already-hyphenated.desktop'],
      ['@org/pkg-name', 'org-pkg-name.desktop'],
      ['Décor Naïveté', 'decor-naivete.desktop'],
      ['Mixed  Punct / And!', 'mixed-punct-and.desktop'],
      ['  -Trim-  ', 'trim.desktop'],
      ['日本語japanese', 'japanese.desktop'],
      [undefined, fallback],
      ['', fallback],
      ['   ', fallback],
      ['日本語', fallback],
      ['!!!', fallback]
    ];
    for (const [input, expected] of cases) {
      expect(defaultDesktopName(input)).to.equal(expected, `input: ${JSON.stringify(input)}`);
    }
  });
});
