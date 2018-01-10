// Copyright (c) 2017 Amaplex Software, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/mac/in_app_purchase_observer.h"

#include "base/bind.h"
#include "base/strings/sys_string_conversions.h"
#include "content/public/browser/browser_thread.h"

#import <CommonCrypto/CommonCrypto.h>
#import <StoreKit/StoreKit.h>

// ============================================================================
//                             InAppTransactionObserver
// ============================================================================

namespace {

using InAppTransactionCallback =
    base::RepeatingCallback<void(const in_app_purchase::Payment&,
                                 const in_app_purchase::Transaction&)>;

}  // namespace

@interface InAppTransactionObserver : NSObject<SKPaymentTransactionObserver> {
 @private
  InAppTransactionCallback callback_;
}

- (id)initWithCallback:(const InAppTransactionCallback&)callback;

@end

@implementation InAppTransactionObserver

/**
 * Init with a callback.
 *
 * @param callback - The callback that will be called for each transaction
 * update.
 */
- (id)initWithCallback:(const InAppTransactionCallback&)callback {
  if ((self = [super init])) {
    callback_ = callback;

    [[SKPaymentQueue defaultQueue] addTransactionObserver:self];
  }

  return self;
}

/**
 * Run the callback in the browser thread.
 *
 * @param transaction - The transaction to pass to the callback.
 */
- (void)runCallback:(SKPaymentTransaction*)transaction {
  if (transaction == nil) {
    return;
  }

  // Convert the payment.
  in_app_purchase::Payment paymentStruct;
  paymentStruct = [self skPaymentToStruct:transaction.payment];

  // Convert the transaction.
  in_app_purchase::Transaction transactionStruct;
  transactionStruct = [self skPaymentTransactionToStruct:transaction];

  // Send the callback to the browser thread.
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(callback_, paymentStruct, transactionStruct));
}

/**
 * Convert an NSDate to ISO String.
 *
 * @param date - The date to convert.
 */
- (NSString*)dateToISOString:(NSDate*)date {
  NSDateFormatter* dateFormatter = [[NSDateFormatter alloc] init];
  NSLocale* enUSPOSIXLocale =
      [NSLocale localeWithLocaleIdentifier:@"en_US_POSIX"];
  [dateFormatter setLocale:enUSPOSIXLocale];
  [dateFormatter setDateFormat:@"yyyy-MM-dd'T'HH:mm:ssZZZZZ"];

  return [dateFormatter stringFromDate:date];
}

/**
 * Convert a SKPayment object to a Payment structure.
 *
 * @param payment - The SKPayment object to convert.
 */
- (in_app_purchase::Payment)skPaymentToStruct:(SKPayment*)payment {
  in_app_purchase::Payment paymentStruct;

  if (payment == nil) {
    return paymentStruct;
  }

  if (payment.productIdentifier != nil) {
    paymentStruct.productIdentifier = [payment.productIdentifier UTF8String];
  }

  if (payment.quantity >= 1) {
    paymentStruct.quantity = (int)payment.quantity;
  }

  return paymentStruct;
}

/**
 * Convert a SKPaymentTransaction object to a Transaction structure.
 *
 * @param transaction - The SKPaymentTransaction object to convert.
 */
- (in_app_purchase::Transaction)skPaymentTransactionToStruct:
    (SKPaymentTransaction*)transaction {
  in_app_purchase::Transaction transactionStruct;

  if (transaction == nil) {
    return transactionStruct;
  }

  if (transaction.transactionIdentifier != nil) {
    transactionStruct.transactionIdentifier =
        [transaction.transactionIdentifier UTF8String];
  }

  if (transaction.transactionDate != nil) {
    transactionStruct.transactionDate =
        [[self dateToISOString:transaction.transactionDate] UTF8String];
  }

  if (transaction.originalTransaction != nil) {
    transactionStruct.originalTransactionIdentifier =
        [transaction.originalTransaction.transactionIdentifier UTF8String];
  }

  if (transaction.error != nil) {
    transactionStruct.errorCode = (int)transaction.error.code;
    transactionStruct.errorMessage =
        [[transaction.error localizedDescription] UTF8String];
  }

  if (transaction.transactionState < 5) {
    transactionStruct.transactionState = [[@[
      @"SKPaymentTransactionStatePurchasing",
      @"SKPaymentTransactionStatePurchased",
      @"SKPaymentTransactionStateFailed",
      @"SKPaymentTransactionStateRestored",
      @"SKPaymentTransactionStateDeferred"
    ] objectAtIndex:transaction.transactionState] UTF8String];
  }

  return transactionStruct;
}

#pragma mark -
#pragma mark SKPaymentTransactionObserver methods

/**
 * Executed when a transaction is updated.
 *
 * @param queue - The payment queue.
 * @param transactions - The list of transactions updated.
 */
- (void)paymentQueue:(SKPaymentQueue*)queue
    updatedTransactions:(NSArray*)transactions {
  for (SKPaymentTransaction* transaction in transactions) {
    [self runCallback:transaction];
  }
}

@end

// ============================================================================
//                             C++ in_app_purchase
// ============================================================================

namespace in_app_purchase {

TransactionObserver::TransactionObserver() : weak_ptr_factory_(this) {
  obeserver_ = [[InAppTransactionObserver alloc]
     initWithCallback:base::Bind(&TransactionObserver::OnTransactionUpdated,
                                 weak_ptr_factory_.GetWeakPtr())];
}

TransactionObserver::~TransactionObserver() {
  [obeserver_ release];
}

}  // namespace in_app_purchase
