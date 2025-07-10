// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_REQUEST_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_REQUEST_H_

#include <map>
#include <set>
#include <string>

#include "base/types/pass_key.h"
#include "gin/weak_cell.h"
#include "gin/wrappable.h"
#include "net/base/completion_once_callback.h"
#include "services/network/public/cpp/resource_request.h"

class URLPattern;

namespace content {
class BrowserContext;
}

namespace extensions {
struct WebRequestInfo;
enum class WebRequestResourceType : uint8_t;
}  // namespace extensions

namespace gin {
class Arguments;
}  // namespace gin

namespace gin_helper {
template <typename T>
class Handle;
}  // namespace gin_helper

namespace electron::api {

class Session;

class WebRequest final : public gin::Wrappable<WebRequest> {
 public:
  using BeforeSendHeadersCallback =
      base::OnceCallback<void(const std::set<std::string>& removed_headers,
                              const std::set<std::string>& set_headers,
                              int error_code)>;

  // AuthRequiredResponse indicates how an OnAuthRequired call is handled.
  enum class AuthRequiredResponse {
    // No credentials were provided.
    AUTH_REQUIRED_RESPONSE_NO_ACTION,
    // AuthCredentials is filled in with a username and password, which should
    // be used in a response to the provided auth challenge.
    AUTH_REQUIRED_RESPONSE_SET_AUTH,
    // The request should be canceled.
    AUTH_REQUIRED_RESPONSE_CANCEL_AUTH,
    // The action will be decided asynchronously. |callback| will be invoked
    // when the decision is made, and one of the other AuthRequiredResponse
    // values will be passed in with the same semantics as described above.
    AUTH_REQUIRED_RESPONSE_IO_PENDING,
  };

  using AuthCallback = base::OnceCallback<void(AuthRequiredResponse)>;

  // Convenience wrapper around api::Session::FromOrCreate()->WebRequest().
  // Creates the Session and WebRequest if they don't already exist.
  // Note that the WebRequest is owned by the session, not by the caller.
  static WebRequest* FromOrCreate(v8::Isolate* isolate,
                                  content::BrowserContext* browser_context);

  // Return a new WebRequest object. This can only be called by api::Session.
  static WebRequest* Create(v8::Isolate* isolate, base::PassKey<Session>);

  // Make public for cppgc::MakeGarbageCollected.
  explicit WebRequest(base::PassKey<Session>);
  ~WebRequest() override;

  // disable copy
  WebRequest(const WebRequest&) = delete;
  WebRequest& operator=(const WebRequest&) = delete;

  static const char* GetClassName() { return "WebRequest"; }

  // gin::Wrappable:
  static const gin::WrapperInfo kWrapperInfo;
  void Trace(cppgc::Visitor*) const override;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  bool HasListener() const;
  int OnBeforeRequest(extensions::WebRequestInfo* info,
                      const network::ResourceRequest& request,
                      net::CompletionOnceCallback callback,
                      GURL* new_url);
  int OnBeforeSendHeaders(extensions::WebRequestInfo* info,
                          const network::ResourceRequest& request,
                          BeforeSendHeadersCallback callback,
                          net::HttpRequestHeaders* headers);
  int OnHeadersReceived(
      extensions::WebRequestInfo* info,
      const network::ResourceRequest& request,
      net::CompletionOnceCallback callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url);
  void OnSendHeaders(extensions::WebRequestInfo* info,
                     const network::ResourceRequest& request,
                     const net::HttpRequestHeaders& headers);
  AuthRequiredResponse OnAuthRequired(const extensions::WebRequestInfo* info,
                                      const net::AuthChallengeInfo& auth_info,
                                      AuthCallback callback,
                                      net::AuthCredentials* credentials);
  void OnBeforeRedirect(extensions::WebRequestInfo* info,
                        const network::ResourceRequest& request,
                        const GURL& new_location);
  void OnResponseStarted(extensions::WebRequestInfo* info,
                         const network::ResourceRequest& request);
  void OnErrorOccurred(extensions::WebRequestInfo* info,
                       const network::ResourceRequest& request,
                       int net_error);
  void OnCompleted(extensions::WebRequestInfo* info,
                   const network::ResourceRequest& request,
                   int net_error);
  void OnRequestWillBeDestroyed(extensions::WebRequestInfo* info);

 private:
  // Contains info about requests that are blocked waiting for a response from
  // the user.
  struct BlockedRequest;

  enum class SimpleEvent {
    kOnSendHeaders,
    kOnBeforeRedirect,
    kOnResponseStarted,
    kOnCompleted,
    kOnErrorOccurred,
  };

  enum class ResponseEvent {
    kOnBeforeRequest,
    kOnBeforeSendHeaders,
    kOnHeadersReceived,
  };

  using SimpleListener = base::RepeatingCallback<void(v8::Local<v8::Value>)>;
  using ResponseCallback = base::OnceCallback<void(v8::Local<v8::Value>)>;
  using ResponseListener =
      base::RepeatingCallback<void(v8::Local<v8::Value>, ResponseCallback)>;

  template <SimpleEvent event>
  void SetSimpleListener(gin::Arguments* args);
  template <ResponseEvent event>
  void SetResponseListener(gin::Arguments* args);
  template <typename Listener, typename Listeners, typename Event>
  void SetListener(Event event, Listeners* listeners, gin::Arguments* args);

  template <typename... Args>
  void HandleSimpleEvent(SimpleEvent event,
                         extensions::WebRequestInfo* info,
                         Args... args);

  int HandleOnBeforeRequestResponseEvent(
      extensions::WebRequestInfo* info,
      const network::ResourceRequest& request,
      net::CompletionOnceCallback callback,
      GURL* redirect_url);
  int HandleOnBeforeSendHeadersResponseEvent(
      extensions::WebRequestInfo* info,
      const network::ResourceRequest& request,
      BeforeSendHeadersCallback callback,
      net::HttpRequestHeaders* headers);
  int HandleOnHeadersReceivedResponseEvent(
      extensions::WebRequestInfo* info,
      const network::ResourceRequest& request,
      net::CompletionOnceCallback callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers);

  void OnBeforeRequestListenerResult(uint64_t id,
                                     v8::Local<v8::Value> response);
  void OnBeforeSendHeadersListenerResult(uint64_t id,
                                         v8::Local<v8::Value> response);
  void OnHeadersReceivedListenerResult(uint64_t id,
                                       v8::Local<v8::Value> response);
  // Callback invoked by LoginHandler when auth credentials are supplied via
  // the unified 'login' event. Bridges back into WebRequest's AuthCallback.
  void OnLoginAuthResult(
      uint64_t id,
      net::AuthCredentials* credentials,
      const std::optional<net::AuthCredentials>& maybe_creds);

  class RequestFilter {
   public:
    RequestFilter(std::set<URLPattern>,
                  std::set<URLPattern>,
                  std::set<extensions::WebRequestResourceType>);
    RequestFilter(const RequestFilter&);
    RequestFilter();
    ~RequestFilter();

    void AddUrlPattern(URLPattern pattern, bool is_match_pattern);
    void AddUrlPatterns(const std::set<std::string>& filter_patterns,
                        RequestFilter* filter,
                        gin::Arguments* args,
                        bool is_match_pattern = true);
    void AddType(extensions::WebRequestResourceType type);

    bool MatchesRequest(const extensions::WebRequestInfo* info) const;

   private:
    bool MatchesURL(const GURL& url,
                    const std::set<URLPattern>& patterns) const;
    bool MatchesType(extensions::WebRequestResourceType type) const;

    std::set<URLPattern> include_url_patterns_;
    std::set<URLPattern> exclude_url_patterns_;
    std::set<extensions::WebRequestResourceType> types_;
  };

  struct SimpleListenerInfo {
    RequestFilter filter;
    SimpleListener listener;

    SimpleListenerInfo(RequestFilter, SimpleListener);
    SimpleListenerInfo();
    ~SimpleListenerInfo();
  };

  struct ResponseListenerInfo {
    RequestFilter filter;
    ResponseListener listener;

    ResponseListenerInfo(RequestFilter, ResponseListener);
    ResponseListenerInfo();
    ~ResponseListenerInfo();
  };

  std::map<SimpleEvent, SimpleListenerInfo> simple_listeners_;
  std::map<ResponseEvent, ResponseListenerInfo> response_listeners_;
  std::map<uint64_t, BlockedRequest> blocked_requests_;

  gin::WeakCellFactory<WebRequest> weak_factory_{this};
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_REQUEST_H_
