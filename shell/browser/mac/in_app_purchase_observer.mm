// Copyright (c) 2017 Amaplex Software, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/mac/in_app_purchase_observer.h"

#include <vector>

#include "base/bind.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

#import <CommonCrypto/CommonCrypto.h>
#import <StoreKit/StoreKit.h>

// ============================================================================
//                             InAppTransactionObserver
// ============================================================================

namespace {

using InAppTransactionCallback = base::RepeatingCallback<void(
    const std::vector<in_app_purchase::Transaction>&)>;

}  // namespace

@interface InAppTransactionObserver : NSObject <SKPaymentTransactionObserver> {
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
 * Cleanup.
 */
- (void)dealloc {
  [[SKPaymentQueue defaultQueue] removeTransactionObserver:self];
  [super dealloc];
}

/**
 * Run the callback in the browser thread.
 *
 * @param transaction - The transaction to pass to the callback.
 */
- (void)runCallback:(NSArray*)transactions {
  // Convert the transaction.
  std::vector<in_app_purchase::Transaction> converted;
  converted.reserve([transactions count]);
  for (SKPaymentTransaction* transaction in transactions) {
    converted.push_back([self skPaymentTransactionToStruct:transaction]);
  }

  // Send the callback to the browser thread.
  base::PostTask(FROM_HERE, {content::BrowserThread::UI},
                 base::BindOnce(callback_, converted));
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
 * Convert a SKPaymentDiscount object to a PaymentDiscount structure.
 *
 * @param paymentDiscount - The SKPaymentDiscount object to convert.
 */
- (in_app_purchase::PaymentDiscount)skPaymentDiscountToStruct:
    (SKPaymentDiscount*)paymentDiscount API_AVAILABLE(macosx(10.14.4)) {
  in_app_purchase::PaymentDiscount paymentDiscountStruct;

  paymentDiscountStruct.identifier = [paymentDiscount.identifier UTF8String];
  paymentDiscountStruct.keyIdentifier =
      [paymentDiscount.keyIdentifier UTF8String];
  // paymentDiscountStruct.nonce = [paymentDiscount.nonce uuidString];
  paymentDiscountStruct.signature = [paymentDiscount.signature UTF8String];
  paymentDiscountStruct.timestamp = [paymentDiscount.timestamp intValue];

  return paymentDiscountStruct;
}

/**
 * Convert a SKPayment object to a Payment structure.
 *
 * @param payment - The SKPayment object to convert.
 */
- (in_app_purchase::Payment)skPaymentToStruct:(SKPayment*)payment {
  in_app_purchase::Payment paymentStruct;

  if (payment.productIdentifier != nil) {
    paymentStruct.productIdentifier = [payment.productIdentifier UTF8String];
  }

  if (payment.quantity >= 1) {
    paymentStruct.quantity = (int)payment.quantity;
  }

  if (payment.applicationUsername != nil) {
    paymentStruct.applicationUsername =
        [payment.applicationUsername UTF8String];
  }

  if (@available(macOS 10.14.4, *)) {
    if (payment.paymentDiscount != nil) {
      paymentStruct.paymentDiscount =
          [self skPaymentDiscountToStruct:payment.paymentDiscount];
    }
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
    transactionStruct.transactionState =
        [[@[ @"purchasing", @"purchased", @"failed", @"restored", @"deferred" ]
            objectAtIndex:transaction.transactionState] UTF8String];
  }

  if (transaction.payment != nil) {
    transactionStruct.payment = [self skPaymentToStruct:transaction.payment];
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
  [self runCallback:transactions];
}

@end

// ============================================================================
//                             C++ in_app_purchase
// ============================================================================

namespace in_app_purchase {

PaymentDiscount::PaymentDiscount() = default;
PaymentDiscount::PaymentDiscount(const PaymentDiscount&) = default;
PaymentDiscount::~PaymentDiscount() = default;

Payment::Payment() = default;
Payment::Payment(const Payment&) = default;
Payment::~Payment() = default;

Transaction::Transaction() = default;
Transaction::Transaction(const Transaction&) = default;
Transaction::~Transaction() = default;

TransactionObserver::TransactionObserver() {
  observer_ = [[InAppTransactionObserver alloc]
      initWithCallback:base::BindRepeating(
                           &TransactionObserver::OnTransactionsUpdated,
                           weak_ptr_factory_.GetWeakPtr())];
}

TransactionObserver::~TransactionObserver() {
  [observer_ release];
}

}  // namespace in_app_purchase
