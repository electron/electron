# CancellableNavigationEvent Object extends `Event`

* `returnValue` Object - Set this to cancel the navigation event and optionally return a custom error code or error page
  * `errorCode` Number - Can be any error code from the [Net Error List](https://source.chromium.org/chromium/chromium/src/+/master:net/base/net_error_list.h).  If you provide `errorPage` then this code can not be `-3`. We recommend using `-2` which is `net::Aborted`.
  * `errorPage` String (optional) - Custom HTML error page to display when the navigation is cancelled.

