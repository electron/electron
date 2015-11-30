// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/atom_renderer_client.h"

#include <string>

#include "atom/common/api/api_messages.h"
#include "atom/common/api/atom_bindings.h"
#include "atom/common/node_bindings.h"
#include "atom/common/node_includes.h"
#include "atom/common/options_switches.h"
#include "atom/renderer/atom_render_view_observer.h"
#include "atom/renderer/guest_view_container.h"
#include "atom/renderer/node_array_buffer_bridge.h"
#include "base/command_line.h"
#include "chrome/renderer/pepper/pepper_helper.h"
#include "chrome/renderer/printing/print_web_view_helper.h"
#include "chrome/renderer/tts_dispatcher.h"
#include "content/public/common/content_constants.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_thread.h"
#include "ipc/ipc_message_macros.h"
#include "third_party/WebKit/public/web/WebCustomElement.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebPluginParams.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebRuntimeFeatures.h"
#include "third_party/WebKit/public/web/WebView.h"

#if defined(OS_WIN)
#include <shlobj.h>
#endif

namespace atom {

namespace {

bool IsSwitchEnabled(base::CommandLine* command_line,
                     const char* switch_string) {
  return command_line->GetSwitchValueASCII(switch_string) == "true";
}

// Helper class to forward the messages to the client.
class AtomRenderFrameObserver : public content::RenderFrameObserver {
 public:
  AtomRenderFrameObserver(content::RenderFrame* frame,
                          AtomRendererClient* renderer_client)
      : content::RenderFrameObserver(frame),
        renderer_client_(renderer_client) {}

  // content::RenderFrameObserver:
  void DidCreateScriptContext(v8::Handle<v8::Context> context,
                              int extension_group,
                              int world_id) {
    renderer_client_->DidCreateScriptContext(
        render_frame()->GetWebFrame(), context);
  }

  bool OnMessageReceived(const IPC::Message& message) {
    bool handled = true;
    IPC_BEGIN_MESSAGE_MAP(AtomRenderFrameObserver, message)
      IPC_MESSAGE_HANDLER(AtomViewMsg_SetZoomLevel, OnSetZoomLevel)
      IPC_MESSAGE_UNHANDLED(handled = false)
    IPC_END_MESSAGE_MAP()

    return handled;
  }

  void OnSetZoomLevel(double level) {
    auto view = render_frame()->GetWebFrame()->view();
    if (view)
      view->setZoomLevel(level);
  }

 private:
  AtomRendererClient* renderer_client_;

  DISALLOW_COPY_AND_ASSIGN(AtomRenderFrameObserver);
};

}  // namespace

AtomRendererClient::AtomRendererClient()
    : node_bindings_(NodeBindings::Create(false)),
      atom_bindings_(new AtomBindings) {
}

AtomRendererClient::~AtomRendererClient() {
}

void AtomRendererClient::WebKitInitialized() {
  EnableWebRuntimeFeatures();

  blink::WebCustomElement::addEmbedderCustomElementName("webview");
  blink::WebCustomElement::addEmbedderCustomElementName("browserplugin");

  OverrideNodeArrayBuffer();

  node_bindings_->Initialize();
  node_bindings_->PrepareMessageLoop();

  DCHECK(!global_env);

  // Create a default empty environment which would be used when we need to
  // run V8 code out of a window context (like running a uv callback).
  v8::Isolate* isolate = blink::mainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = v8::Context::New(isolate);
  global_env = node::Environment::New(context, uv_default_loop());
}

void AtomRendererClient::RenderThreadStarted() {
  content::RenderThread::Get()->AddObserver(this);

#if defined(OS_WIN)
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  base::string16 app_id =
      command_line->GetSwitchValueNative(switches::kAppUserModelId);
  if (!app_id.empty()) {
    SetCurrentProcessExplicitAppUserModelID(app_id.c_str());
  }
#endif
}

void AtomRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  new PepperHelper(render_frame);
  new AtomRenderFrameObserver(render_frame, this);
}

void AtomRendererClient::RenderViewCreated(content::RenderView* render_view) {
  new printing::PrintWebViewHelper(render_view);
  new AtomRenderViewObserver(render_view, this);
}

blink::WebSpeechSynthesizer* AtomRendererClient::OverrideSpeechSynthesizer(
    blink::WebSpeechSynthesizerClient* client) {
  return new TtsDispatcher(client);
}

bool AtomRendererClient::OverrideCreatePlugin(
    content::RenderFrame* render_frame,
    blink::WebLocalFrame* frame,
    const blink::WebPluginParams& params,
    blink::WebPlugin** plugin) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (params.mimeType.utf8() == content::kBrowserPluginMimeType ||
      command_line->HasSwitch(switches::kEnablePlugins))
    return false;

  *plugin = nullptr;
  return true;
}

void AtomRendererClient::DidCreateScriptContext(
    blink::WebFrame* frame,
    v8::Handle<v8::Context> context) {
  // Only insert node integration for the main frame.
  if (frame->parent())
    return;

  // Give the node loop a run to make sure everything is ready.
  node_bindings_->RunMessageLoop();

  // Setup node environment for each window.
  node::Environment* env = node_bindings_->CreateEnvironment(context);

  // Add atom-shell extended APIs.
  atom_bindings_->BindTo(env->isolate(), env->process_object());

  // Make uv loop being wrapped by window context.
  if (node_bindings_->uv_env() == nullptr)
    node_bindings_->set_uv_env(env);

  // Load everything.
  node_bindings_->LoadEnvironment(env);
}

bool AtomRendererClient::ShouldFork(blink::WebLocalFrame* frame,
                                    const GURL& url,
                                    const std::string& http_method,
                                    bool is_initial_navigation,
                                    bool is_server_redirect,
                                    bool* send_referrer) {
  // Handle all the navigations and reloads in browser.
  // FIXME We only support GET here because http method will be ignored when
  // the OpenURLFromTab is triggered, which means form posting would not work,
  // we should solve this by patching Chromium in future.
  *send_referrer = true;
  return http_method == "GET";
}

content::BrowserPluginDelegate* AtomRendererClient::CreateBrowserPluginDelegate(
    content::RenderFrame* render_frame,
    const std::string& mime_type,
    const GURL& original_url) {
  if (mime_type == content::kBrowserPluginMimeType) {
    return new GuestViewContainer(render_frame);
  } else {
    return nullptr;
  }
}

bool AtomRendererClient::ShouldOverridePageVisibilityState(
    const content::RenderFrame* render_frame,
    blink::WebPageVisibilityState* override_state) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  if (IsSwitchEnabled(command_line, switches::kPageVisibility)) {
    *override_state = blink::WebPageVisibilityStateVisible;
    return true;
  }

  return false;
}

void AtomRendererClient::EnableWebRuntimeFeatures() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  if (IsSwitchEnabled(command_line, switches::kExperimentalFeatures))
    blink::WebRuntimeFeatures::enableExperimentalFeatures(true);
  if (IsSwitchEnabled(command_line, switches::kExperimentalCanvasFeatures))
    blink::WebRuntimeFeatures::enableExperimentalCanvasFeatures(true);
  if (IsSwitchEnabled(command_line, switches::kOverlayScrollbars))
    blink::WebRuntimeFeatures::enableOverlayScrollbars(true);
  if (IsSwitchEnabled(command_line, switches::kOverlayFullscreenVideo))
    blink::WebRuntimeFeatures::enableOverlayFullscreenVideo(true);
  if (IsSwitchEnabled(command_line, switches::kSharedWorker))
    blink::WebRuntimeFeatures::enableSharedWorker(true);
}

}  // namespace atom
