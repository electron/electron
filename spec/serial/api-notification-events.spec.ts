import { Notification } from 'electron/main';

import { describe } from 'vitest';

import { once } from 'node:events';

import { ifit } from '../lib/spec-helpers';

describe('Notification module (serial)', () => {
  ifit(process.platform === 'darwin')('emits show and close events', async () => {
    const n = new Notification({
      title: 'test notification',
      body: 'test body',
      silent: true
    });
    {
      const e = once(n, 'show');
      n.show();
      await e;
    }
    {
      const e = once(n, 'close');
      n.close();
      await e;
    }
  });

  ifit(process.platform === 'darwin')('emits show and close events with custom id', async () => {
    const n = new Notification({
      id: 'test-custom-id',
      title: 'test notification',
      body: 'test body',
      silent: true
    });
    {
      const e = once(n, 'show');
      n.show();
      await e;
    }
    {
      const e = once(n, 'close');
      n.close();
      await e;
    }
  });

  ifit(process.platform === 'darwin')('emits show and close events with custom id and groupId', async () => {
    const n = new Notification({
      id: 'E017VKL2N8H|C07RBMNS9EK|1772656675.039',
      groupId: 'E017VKL2N8H|C07RBMNS9EK',
      title: 'test notification',
      body: 'test body',
      silent: true
    });
    {
      const e = once(n, 'show');
      n.show();
      await e;
    }
    {
      const e = once(n, 'close');
      n.close();
      await e;
    }
  });
});
