#include "atom/browser/atom_network_delegate.h"

#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request.h"

namespace atom {

NetworkDelegate::NetworkDelegate() {
}

NetworkDelegate::~NetworkDelegate() {
}

int NetworkDelegate::OnBeforeURLRequest(net::URLRequest* request, const net::CompletionCallback& callback, GURL* new_url) {
  // Allow unlimited concurrent connections.
  request->SetPriority(net::MAXIMUM_PRIORITY);
  request->SetLoadFlags(request->load_flags() | net::LOAD_IGNORE_LIMITS);
  return net::OK;
}

}
