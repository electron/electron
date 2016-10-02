// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_cookie_delegate.h"

#include "content/public/browser/browser_thread.h"

namespace atom {

AtomCookieDelegate::AtomCookieDelegate() {
}

AtomCookieDelegate::~AtomCookieDelegate() {
}

void AtomCookieDelegate::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AtomCookieDelegate::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void AtomCookieDelegate::NotifyObservers(
  const net::CanonicalCookie& cookie, bool removed, ChangeCause cause) {
  FOR_EACH_OBSERVER(Observer,
                    observers_,
                    OnCookieChanged(cookie, removed, cause));
}

void AtomCookieDelegate::OnCookieChanged(
    const net::CanonicalCookie& cookie, bool removed, ChangeCause cause) {
  content::BrowserThread::PostTask(
      content::BrowserThread::UI,
      FROM_HERE,
      base::Bind(&AtomCookieDelegate::NotifyObservers,
                 this, cookie, removed, cause));
}

}  // namespace atom
