// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ATOM_COOKIE_DELEGATE_H_
#define ATOM_BROWSER_NET_ATOM_COOKIE_DELEGATE_H_

#include "base/observer_list.h"
#include "net/cookies/cookie_monster.h"

namespace atom {

class AtomCookieDelegate : public net::CookieMonsterDelegate {
 public:
  AtomCookieDelegate();
  ~AtomCookieDelegate() override;

  class Observer {
   public:
    virtual void OnCookieChanged(const net::CanonicalCookie& cookie,
                                 bool removed,
                                 ChangeCause cause) {}
   protected:
    virtual ~Observer() {}
  };

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // net::CookieMonsterDelegate:
  void OnCookieChanged(const net::CanonicalCookie& cookie,
                       bool removed,
                       ChangeCause cause) override;


 private:
  base::ObserverList<Observer> observers_;

  void NotifyObservers(const net::CanonicalCookie& cookie,
                       bool removed,
                       ChangeCause cause);

  DISALLOW_COPY_AND_ASSIGN(AtomCookieDelegate);
};

}   // namespace atom

#endif  // ATOM_BROWSER_NET_ATOM_COOKIE_DELEGATE_H_
