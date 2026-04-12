import { BrowserWindow, shell } from 'electron/main';

import { expect } from 'chai';
import { afterEach, describe } from 'vitest';

import { once } from 'node:events';

import { ifit } from '../lib/spec-helpers';
import { closeAllWindows } from '../lib/window-helpers';

describe('shell module (serial)', () => {
  afterEach(closeAllWindows);

  ifit(process.platform === 'darwin')(
    'removes focus from the electron window after opening an external link',
    async () => {
      const url = 'http://127.0.0.1';
      const w = new BrowserWindow({ show: true });

      await once(w, 'focus');
      expect(w.isFocused()).to.be.true();

      await Promise.all<void>([shell.openExternal(url), once(w, 'blur') as Promise<any>]);

      expect(w.isFocused()).to.be.false();
    }
  );
});
