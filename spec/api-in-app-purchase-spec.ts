import { inAppPurchase } from 'electron/main';

import { expect } from 'chai';

import * as childProcess from 'node:child_process';

import { ifdescribe, waitUntil } from './lib/spec-helpers';

// pid -> executable for every process that owns an on-screen window. Owner
// pids, unlike window titles, need no screen-recording permission.
function windowOwners(): Map<number, string> {
  const script = `ObjC.import('CoreGraphics');
    const all = ObjC.castRefToObject($.CGWindowListCopyWindowInfo($.kCGWindowListOptionOnScreenOnly, $.kCGNullWindowID));
    const pids = [];
    for (let i = 0; i < all.count; i++) pids.push(ObjC.unwrap(all.objectAtIndex(i).objectForKey('kCGWindowOwnerPID')));
    JSON.stringify(pids);`;
  const pids: number[] = JSON.parse(
    childProcess.execFileSync('osascript', ['-l', 'JavaScript', '-e', script]).toString()
  );
  const ps = childProcess.execFileSync('ps', ['-o', 'pid=,comm=', '-p', [...new Set(pids)].join(',')]).toString();
  const owners = new Map<number, string>();
  for (const line of ps.split('\n')) {
    const match = line.trim().match(/^(\d+)\s+(.*)$/);
    if (match) owners.set(Number(match[1]), match[2]);
  }
  return owners;
}

describe('inAppPurchase module', function () {
  if (process.platform !== 'darwin') return;

  this.timeout(3 * 60 * 1000);

  // Without an App Store session StoreKit answers restoreCompletedTransactions()
  // with an Apple Account sign-in dialog, shown by a system agent, that nothing
  // dismisses; left up, it covers every later screen capture in the run. Close
  // whatever system UI this suite brought on screen.
  let ownersBefore: Set<number>;
  const summoned = () =>
    [...windowOwners()].filter(
      ([pid, executable]) => !ownersBefore.has(pid) && /^\/(System|usr\/libexec)\//.test(executable)
    );
  before(() => {
    ownersBefore = new Set(windowOwners().keys());
  });
  after(async () => {
    // The dialog trails the call that caused it, so give it a moment to show.
    await waitUntil(() => summoned().length > 0, { timeout: 3000 }).catch(() => {});
    await waitUntil(() => {
      const left = summoned();
      for (const [pid] of left) process.kill(pid);
      return left.length === 0;
    });
  });

  it('canMakePayments() returns a boolean', () => {
    const canMakePayments = inAppPurchase.canMakePayments();
    expect(canMakePayments).to.be.a('boolean');
  });

  it('restoreCompletedTransactions() does not throw', () => {
    expect(() => {
      inAppPurchase.restoreCompletedTransactions();
    }).to.not.throw();
  });

  it('finishAllTransactions() does not throw', () => {
    expect(() => {
      inAppPurchase.finishAllTransactions();
    }).to.not.throw();
  });

  it('finishTransactionByDate() does not throw', () => {
    expect(() => {
      inAppPurchase.finishTransactionByDate(new Date().toISOString());
    }).to.not.throw();
  });

  it('getReceiptURL() returns receipt URL', () => {
    expect(inAppPurchase.getReceiptURL()).to.match(/_MASReceipt\/receipt$/);
  });

  // This fails on x64 in CI - likely owing to some weirdness with the machines.
  // We should look into fixing it there but at least run it on arm6 machines.
  ifdescribe(process.arch !== 'x64')('handles product purchases', () => {
    it('purchaseProduct() fails when buying invalid product', async () => {
      const success = await inAppPurchase.purchaseProduct('non-exist');
      expect(success).to.be.false('failed to purchase non-existent product');
    });

    it('purchaseProduct() accepts optional (Integer) argument', async () => {
      const success = await inAppPurchase.purchaseProduct('non-exist', 1);
      expect(success).to.be.false('failed to purchase non-existent product');
    });

    it('purchaseProduct() accepts optional (Object) argument', async () => {
      const success = await inAppPurchase.purchaseProduct('non-exist', { quantity: 1, username: 'username' });
      expect(success).to.be.false('failed to purchase non-existent product');
    });

    it('getProducts() returns an empty list when getting invalid product', async () => {
      const products = await inAppPurchase.getProducts(['non-exist']);
      expect(products).to.be.an('array').of.length(0);
    });
  });
});
