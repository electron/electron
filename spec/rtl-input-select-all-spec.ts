import { BrowserWindow } from 'electron/main';

import { expect } from 'chai';

import * as path from 'node:path';
import { setTimeout } from 'node:timers/promises';

import { ifit } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

const fixturesPath = path.resolve(__dirname, 'fixtures');

// Regression test for: after pressing Cmd+A (select all) in a text input that
// contains RTL text, the *next* character typed should remain in the active RTL
// input language and must NOT be coerced to English/Latin.
//
// Repro steps (macOS):
//   1. Switch system input to an RTL language (e.g. Hebrew).
//   2. Focus a <input> and type several RTL characters.
//   3. Press Cmd+A to select all.
//   4. Type a new character.
//   5. Bug: the character appears as a Latin letter (e.g. 'a') instead of the
//      expected RTL glyph (e.g. 'ש').
//
// This does NOT reproduce in Chrome, so it is Electron-specific.

describe('RTL input: Cmd+A followed by RTL character', () => {
  afterEach(closeAllWindows);

  // Only macOS uses Cmd+A and exhibits this IME bug.
  ifit(process.platform === 'darwin')('typing after Cmd+A should not produce an English character when the input language is RTL', async () => {
    const w = new BrowserWindow({ show: true });
    await w.loadFile(path.join(fixturesPath, 'pages', 'rtl-input.html'));

    // Focus the input element.
    await w.webContents.executeJavaScript("document.getElementById('rtl-input').focus()");

    // Simulate typing a few Hebrew characters via char events.
    // 'ש', 'ל', 'ו' are Hebrew characters that map to the keys a, l, v on a
    // standard QWERTY keyboard when the system input is set to Hebrew.
    const hebrewChars = ['ש', 'ל', 'ו', 'ם'];
    for (const char of hebrewChars) {
      w.webContents.sendInputEvent({ type: 'char', keyCode: char });
      await setTimeout(30);
    }

    // Verify initial RTL text was entered correctly.
    const initialValue = await w.webContents.executeJavaScript(
      "document.getElementById('rtl-input').value"
    );
    expect(initialValue).to.equal(hebrewChars.join(''), 'initial RTL characters should be in the input');

    // Press Cmd+A (select all) — this is the trigger for the bug.
    w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'a', modifiers: ['meta'] });
    w.webContents.sendInputEvent({ type: 'keyUp', keyCode: 'a', modifiers: ['meta'] });
    await setTimeout(100);

    // Now type one more Hebrew character after the select-all.
    // 'ש' is the Hebrew letter Shin, typed with the 'a' key in Hebrew layout.
    const nextHebrewChar = 'ש';
    w.webContents.sendInputEvent({ type: 'char', keyCode: nextHebrewChar });
    await setTimeout(100);

    const valueAfterSelectAllAndType = await w.webContents.executeJavaScript(
      "document.getElementById('rtl-input').value"
    );

    // After Cmd+A, typing replaces the selection. The resulting value must be
    // the new RTL character, NOT a Latin character like 'a'.
    //
    // On the buggy Electron build this assertion FAILS because the value is
    // 'a' (the raw QWERTY key) instead of 'ש' (the Hebrew character).
    expect(valueAfterSelectAllAndType).to.equal(
      nextHebrewChar,
      `After Cmd+A, typing a Hebrew character should produce '${nextHebrewChar}', not a Latin letter. ` +
      `Got: '${valueAfterSelectAllAndType}'. This is an Electron-specific IME bug (does not reproduce in Chrome).`
    );

    // Extra guard: ensure no Latin ASCII letter snuck in (a-z / A-Z).
    expect(valueAfterSelectAllAndType).to.not.match(
      /^[a-zA-Z]$/,
      'The character typed after Cmd+A must not be an English/Latin letter'
    );
  });
});
