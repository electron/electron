// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_api_bridge.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/trace_event/trace_event.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/document_user_data.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/page.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "gin/arguments.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "mojo/public/cpp/bindings/message.h"
#include "mojo/public/cpp/bindings/self_owned_associated_receiver.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/api/electron_api_web_frame_main.h"
#include "shell/browser/child_web_contents_tracker.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/api/api.mojom.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/function_template_extensions.h"
#include "shell/common/gin_helper/wrappable_pointer_tags.h"
#include "shell/common/node_includes.h"
#include "shell/common/serialized_value.h"
#include "shell/common/v8_util.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/cppgc/persistent.h"
#include "v8/include/v8-cppgc.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-microtask-queue.h"
#include "v8/include/v8-promise.h"
#include "v8/include/v8-traced-handle.h"

namespace electron::api {

namespace {

constexpr std::string_view kSyncPrivateKey = "electron_api_bridge_sync";
constexpr std::string_view kWithCallerPrivateKey =
    "electron_api_bridge_with_caller";

// Deliberately the same for every reason a call is refused, so a renderer
// cannot probe which grants exist.
constexpr std::string_view kNotAvailableError =
    "This API is not available to this frame";

using ReplyCallback = base::OnceCallback<void(mojom::ApiBridgeResultPtr)>;

// Unique across the process, so a grant id alone never matches a grant of
// another frame or session.
uint64_t g_next_grant_id = 0;
// Bumped on every store update; lets a navigation tell which stores changed
// after it pushed their values.
uint64_t g_store_generation = 0;

v8::Local<v8::Private> PrivateKey(v8::Isolate* isolate, std::string_view key) {
  return v8::Private::ForApi(isolate, gin::StringToV8(isolate, key));
}

electron::SerializedValue Copy(base::span<const uint8_t> bytes) {
  return electron::SerializedValue(mojo_base::BigBuffer(bytes), bytes.size());
}

mojom::ApiBridgeResultPtr ErrorResult(std::string_view name,
                                      std::string_view message) {
  return mojom::ApiBridgeResult::NewError(
      mojom::ApiBridgeError::New(std::string(name), std::string(message)));
}

mojom::ApiBridgeResultPtr RefusalResult() {
  return ErrorResult("Error", kNotAvailableError);
}

// The name and message of |exception|, as the page's error gets them. The
// stack and any other properties stay in the main process.
mojom::ApiBridgeResultPtr ExceptionResult(v8::Isolate* isolate,
                                          v8::Local<v8::Value> exception) {
  constexpr std::string_view kUnknown = "An unknown error occurred";
  std::string name = "Error";
  if (exception.IsEmpty())
    return ErrorResult(name, kUnknown);
  // Reading `name` or `message`, or stringifying, can run getters that throw.
  v8::TryCatch try_catch(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  if (exception->IsNativeError()) {
    v8::Local<v8::Object> error = exception.As<v8::Object>();
    v8::Local<v8::Value> value;
    if (error->Get(context, gin::StringToV8(isolate, "name")).ToLocal(&value) &&
        value->IsString()) {
      std::string read = gin::V8ToString(isolate, value);
      if (!read.empty())
        name = std::move(read);
    }
    if (error->Get(context, gin::StringToV8(isolate, "message"))
            .ToLocal(&value) &&
        value->IsString()) {
      return ErrorResult(name, gin::V8ToString(isolate, value));
    }
  }
  v8::String::Utf8Value utf8(isolate, exception);
  return ErrorResult(
      name, *utf8 ? std::string(*utf8, utf8.length()) : std::string(kUnknown));
}

constexpr std::string_view kOriginError =
    "The origin option must be an origin such as 'https://example.com', with "
    "no path, or an array of them";
constexpr std::string_view kFileOriginError =
    "file:// can't be an API's origin: every local file has that origin. Load "
    "your pages from a custom protocol instead";

// Accepts "scheme://host[:port]" with an optional trailing slash. Anything
// that names more than an origin is rejected rather than trimmed, so an
// app never locks an API to a different origin than the one it wrote. On
// failure returns why.
std::optional<std::string_view> ParseOrigin(std::string_view value,
                                            url::Origin* out) {
  GURL url(value);
  if (url.SchemeIsFile())
    return kFileOriginError;
  if (!url.is_valid() || url.has_username() || url.has_password() ||
      url.has_query() || url.has_ref() || (url.has_path() && url.path() != "/"))
    return kOriginError;
  url::Origin origin = url::Origin::Create(url);
  if (origin.opaque())
    return kOriginError;
  *out = std::move(origin);
  return std::nullopt;
}

// An origin string or a non-empty array of them. On failure returns why.
std::optional<std::string_view> ParseOrigins(v8::Isolate* isolate,
                                             v8::Local<v8::Value> value,
                                             std::vector<url::Origin>* out) {
  std::vector<std::string> strings;
  std::string single;
  if (gin::ConvertFromV8(isolate, value, &single))
    strings.push_back(std::move(single));
  else if (!value->IsArray() || !gin::ConvertFromV8(isolate, value, &strings))
    return kOriginError;
  if (strings.empty())
    return kOriginError;
  for (const auto& string : strings) {
    url::Origin origin;
    if (std::optional<std::string_view> error = ParseOrigin(string, &origin))
      return error;
    out->push_back(std::move(origin));
  }
  return std::nullopt;
}

// Aborts |controller| in a task of its own, so abort listeners never run in
// the middle of a navigation or of a pass() or revoke() call.
void PostAbort(v8::Global<v8::Object> controller) {
  if (controller.IsEmpty())
    return;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](v8::Global<v8::Object> controller) {
            v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
            if (!isolate)
              return;
            v8::HandleScope handle_scope(isolate);
            v8::Local<v8::Object> object = controller.Get(isolate);
            v8::Local<v8::Context> context =
                object->GetCreationContextChecked(isolate);
            v8::Context::Scope context_scope(context);
            node::CallbackScope callback_scope(isolate, object,
                                               node::async_context{0, 0});
            v8::Local<v8::Value> abort;
            if (!object->Get(context, gin::StringToV8(isolate, "abort"))
                     .ToLocal(&abort) ||
                !abort->IsFunction()) {
              return;
            }
            std::ignore =
                abort.As<v8::Function>()->Call(context, object, 0, nullptr);
          },
          std::move(controller)));
}

}  // namespace

namespace api_bridge {

// Where an event or store is delivered: one member of one grant.
struct Subscriber {
  uint64_t grant_id = 0;
  uint32_t member = 0;
  // The frame of a frame grant; null for a session grant.
  content::FrameTreeNodeId frame;
  // The session of a session grant; empty for a frame grant.
  std::string session;
};

}  // namespace api_bridge

// The object apiBridgeMain.event() returns.
class ApiBridgeEvent final : public gin::Wrappable<ApiBridgeEvent> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static ApiBridgeEvent* Create(v8::Isolate* isolate) {
    return cppgc::MakeGarbageCollected<ApiBridgeEvent>(
        isolate->GetCppHeap()->GetAllocationHandle());
  }

  // Make public for cppgc::MakeGarbageCollected.
  ApiBridgeEvent() = default;
  ~ApiBridgeEvent() override = default;

  // disable copy
  ApiBridgeEvent(const ApiBridgeEvent&) = delete;
  ApiBridgeEvent& operator=(const ApiBridgeEvent&) = delete;

  void AddSubscriber(api_bridge::Subscriber subscriber) {
    subscribers_.push_back(std::move(subscriber));
  }

  // gin::Wrappable
  const gin::WrapperInfo* wrapper_info() const override {
    return &kWrapperInfo;
  }
  const char* GetHumanReadableName() const override {
    return "Electron / ApiBridgeEvent";
  }
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override {
    return gin::Wrappable<ApiBridgeEvent>::GetObjectTemplateBuilder(isolate)
        .SetMethod("emit", &ApiBridgeEvent::Emit);
  }

 private:
  void Emit(gin::Arguments* args);

  std::vector<api_bridge::Subscriber> subscribers_;
};

gin::WrapperInfo ApiBridgeEvent::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronApiBridgeEvent);

// The object apiBridgeMain.store() returns.
class ApiBridgeStore final : public gin::Wrappable<ApiBridgeStore> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  // Make public for cppgc::MakeGarbageCollected.
  ApiBridgeStore() = default;
  ~ApiBridgeStore() override = default;

  // disable copy
  ApiBridgeStore(const ApiBridgeStore&) = delete;
  ApiBridgeStore& operator=(const ApiBridgeStore&) = delete;

  // Returns false, with a DataCloneError pending, if |value| can't be cloned.
  bool SetValue(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    electron::SerializedValue serialized;
    if (!SerializeV8Value(isolate, value, &serialized))
      return false;
    auto bytes = serialized.bytes();
    bytes_.assign(bytes.begin(), bytes.end());
    value_.Reset(isolate, value);
    generation_ = ++g_store_generation;
    return true;
  }

  electron::SerializedValue CopyValue() const { return Copy(bytes_); }
  uint64_t generation() const { return generation_; }

  void AddSubscriber(api_bridge::Subscriber subscriber) {
    subscribers_.push_back(std::move(subscriber));
  }

  // gin::Wrappable
  const gin::WrapperInfo* wrapper_info() const override {
    return &kWrapperInfo;
  }
  const char* GetHumanReadableName() const override {
    return "Electron / ApiBridgeStore";
  }
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override {
    return gin::Wrappable<ApiBridgeStore>::GetObjectTemplateBuilder(isolate)
        .SetMethod("get", &ApiBridgeStore::Get)
        .SetMethod("set", &ApiBridgeStore::Set);
  }
  void Trace(cppgc::Visitor* visitor) const override {
    gin::Wrappable<ApiBridgeStore>::Trace(visitor);
    visitor->Trace(value_);
  }

 private:
  v8::Local<v8::Value> Get(v8::Isolate* isolate) {
    return value_.IsEmpty() ? v8::Undefined(isolate).As<v8::Value>()
                            : value_.Get(isolate);
  }
  void Set(gin::Arguments* args);

  v8::TracedReference<v8::Value> value_;
  // |value_| serialized once, then copied into every message that carries it.
  std::vector<uint8_t> bytes_;
  uint64_t generation_ = 0;
  std::vector<api_bridge::Subscriber> subscribers_;
};

gin::WrapperInfo ApiBridgeStore::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronApiBridgeStore);

namespace api_bridge {

namespace {

struct Member {
  std::string name;
  mojom::ApiBridgeMemberKind kind;
  // For methods wrapped with apiBridgeMain.withCaller().
  bool with_caller = false;
  // Set for methods.
  v8::Global<v8::Function> function;
  // Set for events and stores. Off-heap edges into the cppgc heap.
  cppgc::Persistent<ApiBridgeEvent> event;
  cppgc::Persistent<ApiBridgeStore> store;
};

struct Grant {
  bool AllowsOrigin(const url::Origin& origin) const {
    return std::ranges::any_of(origins, [&origin](const url::Origin& allowed) {
      return allowed.IsSameOriginWith(origin);
    });
  }

  uint64_t id = 0;
  std::string name;
  mojom::ApiBridgeWorld world = mojom::ApiBridgeWorld::kMain;
  std::vector<url::Origin> origins;
  // Session grants only: every frame of a page rather than only main frames,
  // windows opened by a page, and <webview> guests.
  bool all_frames = false;
  bool popups = false;
  bool guests = false;
  // The object passed to pass(); methods are called with it as `this`. The
  // grant keeps it alive until it is revoked or its frame goes away.
  v8::Global<v8::Object> api;
  std::vector<Member> members;
};

using Grants = std::vector<std::unique_ptr<Grant>>;

Grant* FindById(Grants& grants, uint64_t grant_id) {
  auto it = std::ranges::find(grants, grant_id,
                              [](const auto& grant) { return grant->id; });
  return it == grants.end() ? nullptr : it->get();
}

Grants::iterator FindByName(Grants& grants,
                            std::string_view name,
                            mojom::ApiBridgeWorld world) {
  return std::ranges::find_if(grants, [&](const auto& grant) {
    return grant->name == name && grant->world == world;
  });
}

// What ReadyToCommitNavigation pushed, so DidFinishNavigation can send only
// what changed in between.
struct Push {
  std::vector<uint64_t> grant_ids;
  uint64_t store_generation = 0;
};

// A FrameTreeNode that has been passed an API, or has received a session API.
struct FrameGrants {
  // Grants passed to this frame. Pointers stay valid while JS runs and adds
  // or revokes grants.
  Grants grants;
  // Keyed by navigation id.
  base::flat_map<int64_t, Push> pushes;
  // Bound to |client_frame|, the frame's current RenderFrameHost when it was
  // last used; rebound after the frame swaps hosts.
  mojo::AssociatedRemote<mojom::ElectronApiBridgeClient> client;
  content::GlobalRenderFrameHostId client_frame;
};

using FrameRegistry =
    absl::flat_hash_map<content::FrameTreeNodeId, std::unique_ptr<FrameGrants>>;

FrameRegistry& GetFrames() {
  static base::NoDestructor<FrameRegistry> frames;
  return *frames;
}

// Session grants, keyed by BrowserContext::UniqueId(). Unlike a pointer, the
// id of a destroyed session is never reused by a new one.
using SessionRegistry = absl::flat_hash_map<std::string, Grants>;

SessionRegistry& GetSessions() {
  static base::NoDestructor<SessionRegistry> sessions;
  return *sessions;
}

FrameGrants* FindFrame(content::FrameTreeNodeId frame) {
  auto it = GetFrames().find(frame);
  return it == GetFrames().end() ? nullptr : it->second.get();
}

FrameGrants& GetOrCreateFrame(content::FrameTreeNodeId frame) {
  auto& slot = GetFrames()[frame];
  if (!slot)
    slot = std::make_unique<FrameGrants>();
  return *slot;
}

Grants* FindSession(const std::string& session) {
  auto it = GetSessions().find(session);
  return it == GetSessions().end() || it->second.empty() ? nullptr
                                                         : &it->second;
}

const std::string& SessionOf(content::RenderFrameHost* host) {
  return host->GetBrowserContext()->UniqueId();
}

content::RenderFrameHost* CurrentHost(content::FrameTreeNodeId frame) {
  content::WebContents* web_contents =
      content::WebContents::FromFrameTreeNodeId(frame);
  return web_contents ? web_contents->UnsafeFindFrameByFrameTreeNodeId(frame)
                      : nullptr;
}

mojom::ElectronApiBridgeClient* GetClient(FrameGrants& frame,
                                          content::RenderFrameHost* host) {
  if (!frame.client.is_bound() || frame.client_frame != host->GetGlobalId()) {
    frame.client.reset();
    host->GetRemoteAssociatedInterfaces()->GetInterface(&frame.client);
    frame.client_frame = host->GetGlobalId();
  }
  return frame.client.get();
}

// A call whose method returned a promise that has not settled yet.
struct PendingCall {
  uint64_t grant_id = 0;
  // Empty once the call has been answered.
  ReplyCallback reply;
  // The AbortController behind caller.signal, for withCaller methods.
  v8::Global<v8::Object> abort_controller;
};

// The apiBridge state of one document: that it has been sent grants, and its
// calls that have not been answered yet. It goes away with the document.
class ApiBridgeDocument : public content::DocumentUserData<ApiBridgeDocument> {
 public:
  ~ApiBridgeDocument() override {
    // Nobody can receive the answers any more; caller.signal aborts.
    for (auto& weak_call : calls_) {
      if (std::shared_ptr<PendingCall> call = weak_call.lock()) {
        call->reply.Reset();
        PostAbort(std::move(call->abort_controller));
      }
    }
  }

  // disable copy
  ApiBridgeDocument(const ApiBridgeDocument&) = delete;
  ApiBridgeDocument& operator=(const ApiBridgeDocument&) = delete;

  // Only a weak reference: a call whose promise is collected without
  // settling must still be answered, by its reply's default.
  void AddCall(const std::shared_ptr<PendingCall>& call) {
    std::erase_if(calls_, [](const auto& weak_call) {
      std::shared_ptr<PendingCall> other = weak_call.lock();
      return !other || !other->reply;
    });
    calls_.push_back(call);
  }

  // Answers the calls made through grants not in |grant_ids| with the
  // refusal, right away, and aborts their caller.signal. The methods keep
  // running; their results are dropped.
  void CancelCallsExcept(const std::vector<uint64_t>& grant_ids) {
    std::erase_if(calls_, [&grant_ids](const auto& weak_call) {
      std::shared_ptr<PendingCall> call = weak_call.lock();
      if (!call || !call->reply)
        return true;
      if (std::ranges::find(grant_ids, call->grant_id) != grant_ids.end())
        return false;
      std::move(call->reply).Run(RefusalResult());
      PostAbort(std::move(call->abort_controller));
      return true;
    });
  }

 private:
  explicit ApiBridgeDocument(content::RenderFrameHost* render_frame_host)
      : content::DocumentUserData<ApiBridgeDocument>(render_frame_host) {}
  friend class content::DocumentUserData<ApiBridgeDocument>;
  DOCUMENT_USER_DATA_KEY_DECL();

  std::vector<std::weak_ptr<PendingCall>> calls_;
};

DOCUMENT_USER_DATA_KEY_IMPL(ApiBridgeDocument);

bool IsGuest(content::WebContents* web_contents) {
  WebContents* api_web_contents = WebContents::From(web_contents);
  return api_web_contents && api_web_contents->is_guest();
}

// Whether a page created |web_contents| with window.open(), with or without
// an opener. Windows the app creates never count.
bool IsPopup(content::WebContents* web_contents) {
  return ChildWebContentsTracker::FromWebContents(web_contents) != nullptr;
}

bool SessionGrantReachesWebContents(const Grant& grant,
                                    content::WebContents* web_contents) {
  if (IsGuest(web_contents) && !grant.guests)
    return false;
  return grant.popups || !IsPopup(web_contents);
}

// Whether a session grant reaches the committed document of |host|: a frame of
// the primary page (not prerendered, cached or fenced), a main frame unless
// the grant is for all frames, and popups and <webview> guests only if the
// grant asked for them.
bool SessionGrantReaches(const Grant& grant, content::RenderFrameHost* host) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(host);
  if (!web_contents || !SessionGrantReachesWebContents(grant, web_contents) ||
      !host->GetPage().IsPrimary()) {
    return false;
  }
  return grant.all_frames || host->IsInPrimaryMainFrame();
}

// The same for the document |navigation_handle| is about to commit, whose
// RenderFrameHost may not be part of the primary page yet.
bool SessionGrantReaches(const Grant& grant,
                         content::NavigationHandle* navigation_handle) {
  if (!SessionGrantReachesWebContents(grant,
                                      navigation_handle->GetWebContents())) {
    return false;
  }
  if (navigation_handle->IsInPrimaryMainFrame())
    return true;
  // Prerendered pages and fenced frames are main frames of other frame trees.
  if (!grant.all_frames || navigation_handle->IsInMainFrame())
    return false;
  content::RenderFrameHost* parent = navigation_handle->GetParentFrame();
  return parent && parent->GetPage().IsPrimary();
}

// The grants a document of |origin| in |frame| gets: the frame's own, then its
// session's that reach it. A frame's grant hides a session grant of the same
// name in the same world.
template <typename Reaches>
std::vector<Grant*> GrantsFor(FrameGrants* frame,
                              const std::string& session,
                              const url::Origin& origin,
                              Reaches reaches) {
  std::vector<Grant*> grants;
  if (origin.opaque())
    return grants;
  if (frame) {
    for (const auto& grant : frame->grants) {
      if (grant->AllowsOrigin(origin))
        grants.push_back(grant.get());
    }
  }
  if (Grants* session_grants = FindSession(session)) {
    for (const auto& grant : *session_grants) {
      if (!grant->AllowsOrigin(origin) || !reaches(*grant) ||
          std::ranges::any_of(grants, [&grant](const Grant* taken) {
            return taken->name == grant->name && taken->world == grant->world;
          })) {
        continue;
      }
      grants.push_back(grant.get());
    }
  }
  return grants;
}

// The grants the committed document of |host| has.
std::vector<Grant*> GrantsFor(content::RenderFrameHost* host) {
  return GrantsFor(FindFrame(host->GetFrameTreeNodeId()), SessionOf(host),
                   host->GetLastCommittedOrigin(), [host](const Grant& grant) {
                     return SessionGrantReaches(grant, host);
                   });
}

Grant* FindUsableGrant(content::RenderFrameHost* host, uint64_t grant_id) {
  for (Grant* grant : GrantsFor(host)) {
    if (grant->id == grant_id)
      return grant;
  }
  return nullptr;
}

std::vector<uint64_t> IdsOf(const std::vector<Grant*>& grants) {
  std::vector<uint64_t> ids;
  ids.reserve(grants.size());
  for (const Grant* grant : grants)
    ids.push_back(grant->id);
  return ids;
}

mojom::ApiBridgeGrantPtr ToMojo(const Grant& grant) {
  std::vector<mojom::ApiBridgeMemberPtr> members;
  members.reserve(grant.members.size());
  for (const auto& member : grant.members) {
    std::optional<electron::SerializedValue> value;
    if (member.kind == mojom::ApiBridgeMemberKind::kStore)
      value = member.store->CopyValue();
    members.push_back(mojom::ApiBridgeMember::New(member.name, member.kind,
                                                  std::move(value)));
  }
  return mojom::ApiBridgeGrant::New(grant.id, grant.name, grant.world,
                                    std::move(members));
}

std::vector<mojom::ApiBridgeGrantPtr> ToMojo(
    const std::vector<Grant*>& grants) {
  std::vector<mojom::ApiBridgeGrantPtr> mojo_grants;
  mojo_grants.reserve(grants.size());
  for (const Grant* grant : grants)
    mojo_grants.push_back(ToMojo(*grant));
  return mojo_grants;
}

// Remembers which grants an open document has, so that after grants are
// added or revoked it can be sent exactly what changed. This one mechanism
// covers replacing an API, a frame API hiding a session API of the same name,
// and the session API showing through again when the frame API goes.
class DocumentUpdate {
 public:
  explicit DocumentUpdate(content::RenderFrameHost* host)
      : host_(host->GetGlobalId()), before_(IdsOf(GrantsFor(host))) {}

  void Send() {
    content::RenderFrameHost* host = content::RenderFrameHost::FromID(host_);
    if (!host || !host->IsRenderFrameLive())
      return;
    const std::vector<Grant*> after = GrantsFor(host);
    const std::vector<uint64_t> after_ids = IdsOf(after);
    std::vector<uint64_t> revoked;
    for (uint64_t id : before_) {
      if (std::ranges::find(after_ids, id) == after_ids.end())
        revoked.push_back(id);
    }
    std::vector<mojom::ApiBridgeGrantPtr> added;
    for (const Grant* grant : after) {
      if (std::ranges::find(before_, grant->id) == before_.end())
        added.push_back(ToMojo(*grant));
    }
    if (revoked.empty() && added.empty())
      return;

    // Marks the document as one that has had grants, for when it comes back
    // from the back/forward cache.
    ApiBridgeDocument::GetOrCreateForCurrentDocument(host)->CancelCallsExcept(
        after_ids);
    // Tagged with the document's origin: if the renderer has committed the
    // next document of this frame in the meantime, it drops the change.
    GetClient(GetOrCreateFrame(host->GetFrameTreeNodeId()), host)
        ->UpdateGrants(host->GetLastCommittedOrigin().Serialize(),
                       std::move(revoked), std::move(added));
  }

 private:
  content::GlobalRenderFrameHostId host_;
  std::vector<uint64_t> before_;
};

// Every open document of |browser_context|.
std::vector<DocumentUpdate> OpenDocumentsOf(
    content::BrowserContext* browser_context) {
  std::vector<DocumentUpdate> documents;
  for (auto it = content::RenderProcessHost::AllHostsIterator(); !it.IsAtEnd();
       it.Advance()) {
    it.GetCurrentValue()->ForEachRenderFrameHost(
        [&](content::RenderFrameHost* host) {
          if (host->GetBrowserContext() == browser_context &&
              host->IsActive() && host->IsRenderFrameLive()) {
            documents.emplace_back(host);
          }
        });
  }
  return documents;
}

// Sends |host|'s document all of its grants, replacing whatever it holds. For
// a page that comes back from the back/forward cache or from prerendering,
// which may hold grants that were revoked or stores that changed meanwhile.
void ResetDocument(content::RenderFrameHost* host) {
  ApiBridgeDocument* document = ApiBridgeDocument::GetForCurrentDocument(host);
  const std::vector<Grant*> grants = GrantsFor(host);
  // Never had a grant and has none now.
  if (!document && grants.empty())
    return;
  ApiBridgeDocument::GetOrCreateForCurrentDocument(host)->CancelCallsExcept(
      IdsOf(grants));
  GetClient(GetOrCreateFrame(host->GetFrameTreeNodeId()), host)
      ->ResetGrants(ToMojo(grants));
}

// Sends through |send| to every document that has the subscriber's grant, and
// drops subscribers whose grant is gone.
template <typename Send>
void Deliver(std::vector<Subscriber>& subscribers, Send send) {
  auto send_if_usable = [&send](FrameGrants& frame,
                                content::FrameTreeNodeId frame_id,
                                const Subscriber& subscriber) {
    content::RenderFrameHost* host = CurrentHost(frame_id);
    if (host && host->IsRenderFrameLive() &&
        FindUsableGrant(host, subscriber.grant_id)) {
      send(GetClient(frame, host), subscriber.grant_id, subscriber.member);
    }
  };
  std::erase_if(subscribers, [&](const Subscriber& subscriber) {
    if (!subscriber.frame.is_null()) {
      FrameGrants* frame = FindFrame(subscriber.frame);
      if (!frame || !FindById(frame->grants, subscriber.grant_id))
        return true;
      send_if_usable(*frame, subscriber.frame, subscriber);
      return false;
    }
    Grants* session = FindSession(subscriber.session);
    if (!session || !FindById(*session, subscriber.grant_id))
      return true;
    // Every frame a session grant has reached has an entry here.
    for (auto& [frame_id, frame] : GetFrames())
      send_if_usable(*frame, frame_id, subscriber);
    return false;
  });
}

// Replies with |value| serialized, or with the serialization error.
void Reply(v8::Isolate* isolate,
           v8::Local<v8::Value> value,
           ReplyCallback reply) {
  electron::SerializedValue serialized;
  v8::TryCatch try_catch(isolate);
  if (!SerializeV8Value(isolate, value, &serialized)) {
    std::move(reply).Run(ExceptionResult(isolate, try_catch.Exception()));
    return;
  }
  std::move(reply).Run(mojom::ApiBridgeResult::NewValue(std::move(serialized)));
}

// The first argument of a method wrapped with apiBridgeMain.withCaller(). Sets
// |controller| to the AbortController behind caller.signal.
v8::MaybeLocal<v8::Object> MakeCaller(v8::Isolate* isolate,
                                      v8::Local<v8::Context> context,
                                      content::RenderFrameHost* host,
                                      v8::Local<v8::Object>* controller) {
  v8::Local<v8::Value> frame;
  if (!gin::TryConvertToV8(isolate, WebFrameMain::From(isolate, host),
                           &frame)) {
    return {};
  }
  v8::Local<v8::Value> constructor;
  v8::Local<v8::Object> abort_controller;
  v8::Local<v8::Value> signal;
  if (!context->Global()
           ->Get(context, gin::StringToV8(isolate, "AbortController"))
           .ToLocal(&constructor) ||
      !constructor->IsFunction() ||
      !constructor.As<v8::Function>()->NewInstance(context).ToLocal(
          &abort_controller) ||
      !abort_controller->Get(context, gin::StringToV8(isolate, "signal"))
           .ToLocal(&signal)) {
    return {};
  }
  v8::Local<v8::Object> caller = v8::Object::New(isolate);
  auto set = [&](std::string_view key, v8::Local<v8::Value> value) {
    return caller->CreateDataProperty(context, gin::StringToV8(isolate, key),
                                      value);
  };
  if (!set("frame", frame).FromMaybe(false) ||
      !set("origin",
           gin::StringToV8(isolate, host->GetLastCommittedOrigin().Serialize()))
           .FromMaybe(false) ||
      !set("signal", signal).FromMaybe(false) ||
      !caller->SetIntegrityLevel(context, v8::IntegrityLevel::kFrozen)
           .FromMaybe(false)) {
    return {};
  }
  *controller = abort_controller;
  return caller;
}

// Runs a method for the frame |frame_id| and replies with its (awaited)
// result. Everything the renderer sends is checked here, against state only
// the browser holds.
void Dispatch(content::GlobalRenderFrameHostId frame_id,
              uint64_t grant_id,
              uint32_t member_index,
              bool sync,
              electron::SerializedValue args,
              ReplyCallback reply) {
  content::RenderFrameHost* host = content::RenderFrameHost::FromID(frame_id);
  // Documents in the back/forward cache, prerendered or being unloaded can't
  // call.
  if (!host || !host->IsActive()) {
    std::move(reply).Run(RefusalResult());
    return;
  }
  // Only a grant this document has now: one passed to its frame, or one of
  // its session's that reaches it, for its committed origin.
  Grant* grant = FindUsableGrant(host, grant_id);
  if (!grant) {
    std::move(reply).Run(RefusalResult());
    return;
  }
  // A grant's members never change, and the renderer creates sync and async
  // functions from the kinds it was sent, so a mismatch here only comes from
  // a renderer that is not running our code.
  const auto expected = sync ? mojom::ApiBridgeMemberKind::kSyncMethod
                             : mojom::ApiBridgeMemberKind::kMethod;
  if (member_index >= grant->members.size() ||
      grant->members[member_index].kind != expected) {
    mojo::ReportBadMessage("apiBridge: call to a member that is not a method");
    std::move(reply).Run(RefusalResult());
    return;
  }
  const Member& member = grant->members[member_index];
  TRACE_EVENT2("electron", "ApiBridge::Call", "api", grant->name, "member",
               member.name);

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Function> function = member.function.Get(isolate);
  v8::Local<v8::Object> receiver = grant->api.Get(isolate);
  v8::Local<v8::Context> context = function->GetCreationContextChecked(isolate);
  v8::Context::Scope context_scope(context);

  // Like an ipcMain handler: process.nextTick and microtasks run once the
  // implementation returns.
  node::CallbackScope callback_scope(isolate, receiver,
                                     node::async_context{0, 0});
  v8::MicrotasksScope microtasks_scope(context,
                                       v8::MicrotasksScope::kRunMicrotasks);

  v8::LocalVector<v8::Value> argv(isolate);
  v8::Local<v8::Object> abort_controller;
  if (member.with_caller) {
    v8::Local<v8::Object> caller;
    if (!MakeCaller(isolate, context, host, &abort_controller)
             .ToLocal(&caller)) {
      std::move(reply).Run(RefusalResult());
      return;
    }
    argv.push_back(caller);
  }
  v8::Local<v8::Value> args_value = DeserializeV8Value(isolate, args);
  if (args_value.IsEmpty() || !args_value->IsArray()) {
    std::move(reply).Run(
        ErrorResult("Error", "The arguments could not be read"));
    return;
  }
  v8::Local<v8::Array> args_array = args_value.As<v8::Array>();
  for (uint32_t i = 0; i < args_array->Length(); ++i) {
    v8::Local<v8::Value> arg;
    if (!args_array->Get(context, i).ToLocal(&arg)) {
      std::move(reply).Run(
          ErrorResult("Error", "The arguments could not be read"));
      return;
    }
    argv.push_back(arg);
  }

  // The method can revoke grants and destroy frames: |grant|, |member| and
  // |host| are not used after this call.
  //
  // Our TryCatch sits inside the CallbackScope's verbose one, so a throwing
  // implementation is answered to the caller instead of being reported as an
  // uncaught exception in the main process.
  v8::TryCatch try_catch(isolate);
  v8::Local<v8::Value> result;
  if (!function->Call(context, receiver, argv.size(), argv.data())
           .ToLocal(&result)) {
    if (!try_catch.HasTerminated())
      std::move(reply).Run(ExceptionResult(isolate, try_catch.Exception()));
    return;
  }
  if (!result->IsPromise()) {
    Reply(isolate, result, std::move(reply));
    return;
  }

  auto call = std::make_shared<PendingCall>();
  call->grant_id = grant_id;
  // A promise that is collected without settling drops both callbacks and
  // with them the last reference to |call|; answer then instead of leaving
  // the call (or a blocked sync caller) hanging.
  call->reply = mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      std::move(reply),
      ErrorResult("Error",
                  "The main process dropped the call without answering"));
  if (!abort_controller.IsEmpty())
    call->abort_controller.Reset(isolate, abort_controller);

  // If the method revoked its own API, or the document went away, the call
  // is cancelled like any other unfinished call.
  content::RenderFrameHost* caller_host =
      content::RenderFrameHost::FromID(frame_id);
  if (!caller_host || !caller_host->IsActive() ||
      !FindUsableGrant(caller_host, grant_id)) {
    std::move(call->reply).Run(RefusalResult());
    PostAbort(std::move(call->abort_controller));
    return;
  }
  ApiBridgeDocument::GetOrCreateForCurrentDocument(caller_host)->AddCall(call);

  auto on_fulfilled = base::BindOnce(
      [](std::shared_ptr<PendingCall> call, v8::Local<v8::Value> value) {
        call->abort_controller.Reset();
        if (call->reply) {
          Reply(JavascriptEnvironment::GetIsolate(), value,
                std::move(call->reply));
        }
      },
      call);
  auto on_rejected = base::BindOnce(
      [](std::shared_ptr<PendingCall> call, v8::Local<v8::Value> reason) {
        call->abort_controller.Reset();
        if (call->reply) {
          std::move(call->reply)
              .Run(
                  ExceptionResult(JavascriptEnvironment::GetIsolate(), reason));
        }
      },
      call);
  std::ignore = result.As<v8::Promise>()->Then(
      context,
      gin::ConvertToV8(isolate, std::move(on_fulfilled)).As<v8::Function>(),
      gin::ConvertToV8(isolate, std::move(on_rejected)).As<v8::Function>());
}

class ApiBridgeHost final : public mojom::ElectronApiBridgeHost {
 public:
  explicit ApiBridgeHost(content::GlobalRenderFrameHostId frame_id)
      : frame_id_(frame_id) {}

  // disable copy
  ApiBridgeHost(const ApiBridgeHost&) = delete;
  ApiBridgeHost& operator=(const ApiBridgeHost&) = delete;

  // mojom::ElectronApiBridgeHost
  void Call(uint64_t grant_id,
            uint32_t member,
            electron::SerializedValue args,
            CallCallback callback) override {
    Dispatch(frame_id_, grant_id, member, /*sync=*/false, std::move(args),
             std::move(callback));
  }
  void CallSync(uint64_t grant_id,
                uint32_t member,
                electron::SerializedValue args,
                CallSyncCallback callback) override {
    Dispatch(frame_id_, grant_id, member, /*sync=*/true, std::move(args),
             std::move(callback));
  }

 private:
  // The frame this receiver was bound for. Calls are attributed to it and
  // never to anything the renderer says.
  const content::GlobalRenderFrameHostId frame_id_;
};

void HandleReadyToCommit(content::NavigationHandle* navigation_handle) {
  // Same-document navigations keep the document and its grants. A page
  // restored from the back/forward cache or activated from prerendering
  // already has its document; DidFinishNavigation brings it up to date.
  if (navigation_handle->IsSameDocument() ||
      navigation_handle->IsPageActivation()) {
    return;
  }
  const content::FrameTreeNodeId frame_id =
      navigation_handle->GetFrameTreeNodeId();
  FrameGrants* frame = FindFrame(frame_id);
  content::RenderFrameHost* host = navigation_handle->GetRenderFrameHost();
  if (!host || !host->IsRenderFrameLive())
    return;
  const std::string& session = SessionOf(host);
  // Most navigations end here: no frame API and no session API.
  if (!frame && !FindSession(session))
    return;

  std::vector<Grant*> grants;
  if (std::optional<url::Origin> origin =
          navigation_handle->GetOriginToCommit()) {
    grants = GrantsFor(frame, session, *origin,
                       [navigation_handle](const Grant& grant) {
                         return SessionGrantReaches(grant, navigation_handle);
                       });
  }
  // A frame that never had anything needs no push: its document starts
  // empty anyway. Once it has had grants, always push, even an empty set.
  if (!frame && grants.empty())
    return;
  frame = &GetOrCreateFrame(frame_id);
  frame->pushes.insert_or_assign(navigation_handle->GetNavigationId(),
                                 Push{IdsOf(grants), g_store_generation});

  // |host| may be a speculative RenderFrameHost that isn't the frame's current
  // one yet, so this doesn't go through the cached client. Routed over the
  // navigation channel, it is ordered before CommitNavigation.
  mojo::AssociatedRemote<mojom::ElectronApiBridgeClient> client;
  host->GetRemoteAssociatedInterfaces()->GetInterface(&client);
  client->SetPendingGrants(ToMojo(grants));
}

void HandleDidFinishNavigation(content::NavigationHandle* navigation_handle) {
  const content::FrameTreeNodeId frame_id =
      navigation_handle->GetFrameTreeNodeId();
  FrameGrants* frame = FindFrame(frame_id);
  std::optional<Push> push;
  if (frame) {
    if (auto it = frame->pushes.find(navigation_handle->GetNavigationId());
        it != frame->pushes.end()) {
      push = std::move(it->second);
      frame->pushes.erase(it);
    }
  }
  if (!navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }
  content::RenderFrameHost* host = navigation_handle->GetRenderFrameHost();
  if (!host || !host->IsRenderFrameLive())
    return;
  if (navigation_handle->IsPageActivation()) {
    ResetDocument(host);
    return;
  }
  if (!frame && !FindSession(SessionOf(host)))
    return;

  // What the committed document should have, which can differ from what was
  // pushed: grants passed or revoked in between, or a different origin than
  // expected (an error page, a sandboxed response).
  const std::vector<Grant*> grants = GrantsFor(host);
  if (!frame && grants.empty())
    return;
  mojom::ElectronApiBridgeClient* client =
      GetClient(GetOrCreateFrame(frame_id), host);
  const std::vector<uint64_t> pushed =
      push ? push->grant_ids : std::vector<uint64_t>();
  std::vector<uint64_t> revoked;
  for (uint64_t grant_id : pushed) {
    auto it = std::ranges::find(grants, grant_id, &Grant::id);
    if (it == grants.end()) {
      revoked.push_back(grant_id);
      continue;
    }
    // Stores updated after the push went to the previous document.
    for (uint32_t i = 0; i < (*it)->members.size(); ++i) {
      const Member& member = (*it)->members[i];
      if (member.kind == mojom::ApiBridgeMemberKind::kStore &&
          member.store->generation() > push->store_generation) {
        client->UpdateStore(grant_id, i, member.store->CopyValue());
      }
    }
  }
  std::vector<mojom::ApiBridgeGrantPtr> added;
  for (const Grant* grant : grants) {
    if (std::ranges::find(pushed, grant->id) == pushed.end())
      added.push_back(ToMojo(*grant));
  }
  // Right after the commit, the renderer's current document is this one.
  if (!revoked.empty() || !added.empty())
    client->UpdateGrants(std::nullopt, std::move(revoked), std::move(added));
  if (!grants.empty())
    ApiBridgeDocument::GetOrCreateForCurrentDocument(host);
}

void HandleFrameDeleted(content::FrameTreeNodeId frame_tree_node_id) {
  GetFrames().erase(frame_tree_node_id);
}

void HandleRenderFrameDeleted(content::RenderFrameHost* render_frame_host) {
  // After a renderer crash the document's user data outlives the renderer
  // until the next navigation; its calls are over now.
  if (ApiBridgeDocument* document =
          ApiBridgeDocument::GetForCurrentDocument(render_frame_host)) {
    document->CancelCallsExcept({});
  }
}

void HandleWebContentsDestroyed(content::WebContents* web_contents) {
  absl::erase_if(GetFrames(), [web_contents](const auto& entry) {
    content::WebContents* owner =
        content::WebContents::FromFrameTreeNodeId(entry.first);
    return !owner || owner == web_contents;
  });
}

// Follows the navigations and frames of one WebContents. Separate from
// api::WebContents, which a popup only gets once its first navigation may
// already have committed.
class ApiBridgeObserver
    : public content::WebContentsObserver,
      public content::WebContentsUserData<ApiBridgeObserver> {
 public:
  ~ApiBridgeObserver() override = default;

  // disable copy
  ApiBridgeObserver(const ApiBridgeObserver&) = delete;
  ApiBridgeObserver& operator=(const ApiBridgeObserver&) = delete;

  // content::WebContentsObserver
  void ReadyToCommitNavigation(
      content::NavigationHandle* navigation_handle) override {
    HandleReadyToCommit(navigation_handle);
  }
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override {
    // Also for navigations that didn't commit, which drop what
    // ReadyToCommitNavigation recorded for them.
    HandleDidFinishNavigation(navigation_handle);
  }
  void FrameDeleted(content::FrameTreeNodeId frame_tree_node_id) override {
    HandleFrameDeleted(frame_tree_node_id);
  }
  void RenderFrameDeleted(
      content::RenderFrameHost* render_frame_host) override {
    HandleRenderFrameDeleted(render_frame_host);
  }
  void WebContentsDestroyed() override {
    HandleWebContentsDestroyed(web_contents());
  }

 private:
  explicit ApiBridgeObserver(content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents),
        content::WebContentsUserData<ApiBridgeObserver>(*web_contents) {}
  friend class content::WebContentsUserData<ApiBridgeObserver>;
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

WEB_CONTENTS_USER_DATA_KEY_IMPL(ApiBridgeObserver);

}  // namespace

void ObserveWebContents(content::WebContents* web_contents) {
  ApiBridgeObserver::CreateForWebContents(web_contents);
}

void BindHost(
    content::RenderFrameHost* render_frame_host,
    mojo::PendingAssociatedReceiver<mojom::ElectronApiBridgeHost> receiver) {
  mojo::MakeSelfOwnedAssociatedReceiver(
      std::make_unique<ApiBridgeHost>(render_frame_host->GetGlobalId()),
      std::move(receiver));
}

}  // namespace api_bridge

void ApiBridgeEvent::Emit(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  std::vector<v8::Local<v8::Value>> values;
  args->GetRemaining(&values);
  v8::Local<v8::Array> array =
      v8::Array::New(isolate, values.data(), values.size());
  electron::SerializedValue serialized;
  // Throws a DataCloneError if an argument can't be cloned.
  if (!SerializeV8Value(isolate, array, &serialized))
    return;
  api_bridge::Deliver(
      subscribers_, [&serialized](mojom::ElectronApiBridgeClient* client,
                                  uint64_t grant_id, uint32_t member) {
        client->EmitEvent(grant_id, member, Copy(serialized.bytes()));
      });
}

void ApiBridgeStore::Set(gin::Arguments* args) {
  v8::Local<v8::Value> value = v8::Undefined(args->isolate());
  args->GetNext(&value);
  if (!SetValue(args->isolate(), value))
    return;
  api_bridge::Deliver(subscribers_,
                      [this](mojom::ElectronApiBridgeClient* client,
                             uint64_t grant_id, uint32_t member) {
                        client->UpdateStore(grant_id, member, CopyValue());
                      });
}

}  // namespace electron::api

namespace {

using electron::api::ApiBridgeEvent;
using electron::api::ApiBridgeStore;
using electron::api::Session;
using electron::api::WebFrameMain;
using electron::mojom::ApiBridgeWorld;
namespace api_bridge = electron::api::api_bridge;

ApiBridgeEvent* CreateEvent(v8::Isolate* isolate) {
  return ApiBridgeEvent::Create(isolate);
}

v8::Local<v8::Value> CreateStore(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  v8::Local<v8::Value> initial = v8::Undefined(isolate);
  args->GetNext(&initial);
  auto* store = cppgc::MakeGarbageCollected<ApiBridgeStore>(
      isolate->GetCppHeap()->GetAllocationHandle());
  if (!store->SetValue(isolate, initial))
    return v8::Undefined(isolate);
  v8::Local<v8::Object> wrapper;
  if (!store->GetWrapper(isolate).ToLocal(&wrapper))
    return v8::Undefined(isolate);
  return wrapper;
}

// Marks the function passed in with a private symbol and returns it.
v8::Local<v8::Value> Mark(gin::Arguments* args,
                          std::string_view key,
                          std::string_view error) {
  v8::Isolate* isolate = args->isolate();
  v8::Local<v8::Function> function;
  if (!args->GetNext(&function)) {
    args->ThrowTypeError(std::string(error));
    return v8::Undefined(isolate);
  }
  function
      ->SetPrivate(isolate->GetCurrentContext(),
                   electron::api::PrivateKey(isolate, key), v8::True(isolate))
      .Check();
  return function;
}

v8::Local<v8::Value> MarkSync(gin::Arguments* args) {
  return Mark(args, electron::api::kSyncPrivateKey,
              "apiBridgeMain.sync() expects a function");
}

v8::Local<v8::Value> MarkWithCaller(gin::Arguments* args) {
  return Mark(args, electron::api::kWithCallerPrivateKey,
              "apiBridgeMain.withCaller() expects a function");
}

bool HasMark(v8::Isolate* isolate,
             v8::Local<v8::Context> context,
             v8::Local<v8::Function> function,
             std::string_view key) {
  v8::Local<v8::Value> value;
  return function->GetPrivate(context, electron::api::PrivateKey(isolate, key))
             .ToLocal(&value) &&
         value->IsTrue();
}

// Reads the name and API arguments shared by both pass() functions.
bool ReadNameAndApi(gin::Arguments* args,
                    std::string* name,
                    v8::Local<v8::Object>* api) {
  gin_helper::ErrorThrower thrower(args->isolate());
  // The name becomes a property of the page's navigator.electron.
  auto is_identifier = [](std::string_view value) {
    return !value.empty() && !base::IsAsciiDigit(value.front()) &&
           std::ranges::all_of(value, [](char c) {
             return base::IsAsciiAlphaNumeric(c) || c == '_' || c == '$';
           });
  };
  if (!args->GetNext(name) || !is_identifier(*name)) {
    thrower.ThrowTypeError(
        "The name must be a JavaScript identifier, such as 'notes'; the API "
        "gets that name on the page's navigator.electron");
    return false;
  }
  v8::Local<v8::Value> api_value;
  if (!args->GetNext(&api_value) || !api_value->IsObject() ||
      api_value->IsFunction()) {
    thrower.ThrowTypeError("The API must be an object");
    return false;
  }
  *api = api_value.As<v8::Object>();
  return true;
}

// Reads one property of the options argument, or an empty handle.
v8::Local<v8::Value> ReadOption(v8::Isolate* isolate,
                                v8::Local<v8::Value> options,
                                std::string_view key) {
  v8::Local<v8::Value> value;
  if (options.IsEmpty() || !options->IsObject())
    return value;
  gin_helper::Dictionary(isolate, options.As<v8::Object>()).Get(key, &value);
  return value.IsEmpty() || value->IsUndefined() ? v8::Local<v8::Value>()
                                                 : value;
}

// Reads a boolean option into |out|, or throws.
bool ReadBooleanOption(v8::Isolate* isolate,
                       v8::Local<v8::Value> options,
                       std::string_view key,
                       bool* out) {
  v8::Local<v8::Value> value = ReadOption(isolate, options, key);
  if (value.IsEmpty())
    return true;
  if (!value->IsBoolean()) {
    gin_helper::ErrorThrower(isolate).ThrowTypeError(
        "The " + std::string(key) + " option must be a boolean");
    return false;
  }
  *out = value->IsTrue();
  return true;
}

// Turns the API object into a grant's members, or throws.
std::unique_ptr<api_bridge::Grant> ReadMembers(v8::Isolate* isolate,
                                               v8::Local<v8::Object> api) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  // Only own enumerable string keys: nothing inherited from a prototype is
  // exposed by accident.
  v8::Local<v8::Array> keys;
  if (!api->GetOwnPropertyNames(context, v8::ONLY_ENUMERABLE,
                                v8::KeyConversionMode::kConvertToString)
           .ToLocal(&keys)) {
    return nullptr;
  }
  auto grant = std::make_unique<api_bridge::Grant>();
  grant->members.reserve(keys->Length());
  for (uint32_t i = 0; i < keys->Length(); ++i) {
    v8::Local<v8::Value> key;
    v8::Local<v8::Value> value;
    if (!keys->Get(context, i).ToLocal(&key) ||
        !api->Get(context, key).ToLocal(&value)) {
      return nullptr;
    }
    api_bridge::Member& member = grant->members.emplace_back();
    member.name = gin::V8ToString(isolate, key);
    ApiBridgeEvent* event = nullptr;
    ApiBridgeStore* store = nullptr;
    if (value->IsFunction()) {
      v8::Local<v8::Function> function = value.As<v8::Function>();
      member.kind =
          HasMark(isolate, context, function, electron::api::kSyncPrivateKey)
              ? electron::mojom::ApiBridgeMemberKind::kSyncMethod
              : electron::mojom::ApiBridgeMemberKind::kMethod;
      member.with_caller = HasMark(isolate, context, function,
                                   electron::api::kWithCallerPrivateKey);
      member.function.Reset(isolate, function);
    } else if (gin::ConvertFromV8(isolate, value, &event) && event) {
      member.kind = electron::mojom::ApiBridgeMemberKind::kEvent;
      member.event = event;
    } else if (gin::ConvertFromV8(isolate, value, &store) && store) {
      member.kind = electron::mojom::ApiBridgeMemberKind::kStore;
      member.store = store;
    } else {
      gin_helper::ErrorThrower(isolate).ThrowTypeError(
          "API member '" + member.name +
          "' must be a function, an apiBridgeMain.event() or an "
          "apiBridgeMain.store()");
      return nullptr;
    }
  }
  return grant;
}

// Gives |grant| its id, name, world and API object, and subscribes it to its
// events and stores.
void Finish(v8::Isolate* isolate,
            api_bridge::Grant& grant,
            std::string name,
            ApiBridgeWorld world,
            v8::Local<v8::Object> api,
            const api_bridge::Subscriber& subscriber_base) {
  grant.id = ++electron::api::g_next_grant_id;
  grant.name = std::move(name);
  grant.world = world;
  grant.api.Reset(isolate, api);
  for (uint32_t i = 0; i < grant.members.size(); ++i) {
    api_bridge::Subscriber subscriber = subscriber_base;
    subscriber.grant_id = grant.id;
    subscriber.member = i;
    if (grant.members[i].event)
      grant.members[i].event->AddSubscriber(subscriber);
    if (grant.members[i].store)
      grant.members[i].store->AddSubscriber(subscriber);
  }
}

ApiBridgeWorld ReadWorld(gin::Arguments* args) {
  bool isolated = false;
  args->GetNext(&isolated);
  return isolated ? ApiBridgeWorld::kIsolated : ApiBridgeWorld::kMain;
}

// pass(frame, name, api, options, isolated)
void Pass(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::ErrorThrower thrower(isolate);

  WebFrameMain* web_frame = nullptr;
  std::string name;
  v8::Local<v8::Object> api;
  if (!args->GetNext(&web_frame) || !web_frame) {
    thrower.ThrowTypeError("Expected a WebFrameMain");
    return;
  }
  if (!ReadNameAndApi(args, &name, &api))
    return;
  v8::Local<v8::Value> options;
  args->GetNext(&options);
  const ApiBridgeWorld world = ReadWorld(args);
  TRACE_EVENT1("electron", "ApiBridge::Pass", "name", name);
  content::RenderFrameHost* host = web_frame->render_frame_host();
  if (!host) {
    thrower.ThrowError("The frame has been destroyed");
    return;
  }

  std::vector<url::Origin> origins;
  if (v8::Local<v8::Value> origin = ReadOption(isolate, options, "origin");
      !origin.IsEmpty()) {
    if (std::optional<std::string_view> error =
            electron::api::ParseOrigins(isolate, origin, &origins)) {
      thrower.ThrowTypeError(*error);
      return;
    }
  } else {
    const url::Origin& committed = host->GetLastCommittedOrigin();
    if (committed.scheme() == url::kFileScheme) {
      thrower.ThrowError(electron::api::kFileOriginError);
      return;
    }
    if (committed.opaque()) {
      thrower.ThrowError(
          "The frame has no origin to lock the API to yet; pass the origin "
          "option");
      return;
    }
    origins.push_back(committed);
  }

  std::unique_ptr<api_bridge::Grant> grant = ReadMembers(isolate, api);
  if (!grant)
    return;
  grant->origins = std::move(origins);

  const content::FrameTreeNodeId frame_id = host->GetFrameTreeNodeId();
  content::RenderFrameHost* current = api_bridge::CurrentHost(frame_id);
  std::optional<api_bridge::DocumentUpdate> update;
  if (current && current->IsRenderFrameLive())
    update.emplace(current);

  api_bridge::FrameGrants& frame = api_bridge::GetOrCreateFrame(frame_id);
  // Passing a name again replaces the API.
  if (auto it = api_bridge::FindByName(frame.grants, name, world);
      it != frame.grants.end()) {
    frame.grants.erase(it);
  }
  Finish(isolate, *grant, std::move(name), world, api, {.frame = frame_id});
  frame.grants.push_back(std::move(grant));

  // The current document gets it now if it has one of the origins; later
  // documents of those origins get it before they commit.
  if (update)
    update->Send();
}

// revoke(frame, name, isolated) -> boolean
bool Revoke(gin::Arguments* args) {
  WebFrameMain* web_frame = nullptr;
  std::string name;
  if (!args->GetNext(&web_frame) || !web_frame || !args->GetNext(&name)) {
    args->ThrowTypeError("Expected a WebFrameMain and a name");
    return false;
  }
  const ApiBridgeWorld world = ReadWorld(args);
  TRACE_EVENT1("electron", "ApiBridge::Revoke", "name", name);
  content::RenderFrameHost* host = web_frame->render_frame_host();
  if (!host)
    return false;
  const content::FrameTreeNodeId frame_id = host->GetFrameTreeNodeId();
  api_bridge::FrameGrants* frame = api_bridge::FindFrame(frame_id);
  if (!frame)
    return false;
  auto it = api_bridge::FindByName(frame->grants, name, world);
  if (it == frame->grants.end())
    return false;

  content::RenderFrameHost* current = api_bridge::CurrentHost(frame_id);
  std::optional<api_bridge::DocumentUpdate> update;
  if (current && current->IsRenderFrameLive())
    update.emplace(current);
  frame->grants.erase(it);
  // Also lets a session API of the same name take its place.
  if (update)
    update->Send();
  return true;
}

// passToSession(session, name, api, options, isolated)
void PassToSession(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::ErrorThrower thrower(isolate);

  Session* session = nullptr;
  std::string name;
  v8::Local<v8::Object> api;
  if (!args->GetNext(&session) || !session) {
    thrower.ThrowTypeError("Expected a Session");
    return;
  }
  if (!ReadNameAndApi(args, &name, &api))
    return;
  v8::Local<v8::Value> options;
  args->GetNext(&options);
  const ApiBridgeWorld world = ReadWorld(args);
  TRACE_EVENT1("electron", "ApiBridge::PassToSession", "name", name);

  std::vector<url::Origin> origins;
  v8::Local<v8::Value> origin = ReadOption(isolate, options, "origin");
  if (origin.IsEmpty()) {
    thrower.ThrowTypeError(
        "The origin option is required: an origin such as "
        "'https://example.com', with no path, or an array of them");
    return;
  }
  if (std::optional<std::string_view> error =
          electron::api::ParseOrigins(isolate, origin, &origins)) {
    thrower.ThrowTypeError(*error);
    return;
  }
  bool all_frames = false;
  if (v8::Local<v8::Value> frames = ReadOption(isolate, options, "frames");
      !frames.IsEmpty()) {
    std::string value;
    if (!gin::ConvertFromV8(isolate, frames, &value) ||
        (value != "main" && value != "all")) {
      thrower.ThrowTypeError("The frames option must be 'main' or 'all'");
      return;
    }
    all_frames = value == "all";
  }
  bool popups = false;
  bool guests = false;
  if (!ReadBooleanOption(isolate, options, "popups", &popups) ||
      !ReadBooleanOption(isolate, options, "guests", &guests)) {
    return;
  }

  std::unique_ptr<api_bridge::Grant> grant = ReadMembers(isolate, api);
  if (!grant)
    return;
  grant->origins = std::move(origins);
  grant->all_frames = all_frames;
  grant->popups = popups;
  grant->guests = guests;

  content::BrowserContext* browser_context = session->browser_context();
  const std::string& key = browser_context->UniqueId();
  std::vector<api_bridge::DocumentUpdate> updates =
      api_bridge::OpenDocumentsOf(browser_context);

  api_bridge::Grants& grants = api_bridge::GetSessions()[key];
  // Passing a name again replaces the API.
  if (auto it = api_bridge::FindByName(grants, name, world); it != grants.end())
    grants.erase(it);
  Finish(isolate, *grant, std::move(name), world, api, {.session = key});
  grants.push_back(std::move(grant));

  // Open documents get it now; later ones get it before they commit.
  for (auto& update : updates)
    update.Send();
}

// revokeFromSession(session, name, isolated) -> boolean
bool RevokeFromSession(gin::Arguments* args) {
  Session* session = nullptr;
  std::string name;
  if (!args->GetNext(&session) || !session || !args->GetNext(&name)) {
    args->ThrowTypeError("Expected a Session and a name");
    return false;
  }
  const ApiBridgeWorld world = ReadWorld(args);
  TRACE_EVENT1("electron", "ApiBridge::RevokeFromSession", "name", name);
  content::BrowserContext* browser_context = session->browser_context();
  auto sessions_it =
      api_bridge::GetSessions().find(browser_context->UniqueId());
  if (sessions_it == api_bridge::GetSessions().end())
    return false;
  api_bridge::Grants& grants = sessions_it->second;
  auto it = api_bridge::FindByName(grants, name, world);
  if (it == grants.end())
    return false;

  std::vector<api_bridge::DocumentUpdate> updates =
      api_bridge::OpenDocumentsOf(browser_context);
  grants.erase(it);
  for (auto& update : updates)
    update.Send();
  return true;
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict{isolate, exports};
  dict.SetMethod("createEvent", &CreateEvent);
  dict.SetMethod("createStore", &CreateStore);
  dict.SetMethod("markSync", &MarkSync);
  dict.SetMethod("markWithCaller", &MarkWithCaller);
  dict.SetMethod("pass", &Pass);
  dict.SetMethod("revoke", &Revoke);
  dict.SetMethod("passToSession", &PassToSession);
  dict.SetMethod("revokeFromSession", &RevokeFromSession);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_api_bridge, Initialize)
