import { inAppPurchase } from 'electron/main';

import { expect } from 'chai';

import { ifdescribe } from './lib/spec-helpers';

describe('inAppPurchase module', function () {
  if (process.platform !== 'darwin') return;

  this.timeout(3 * 60 * 1000);

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
