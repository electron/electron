// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/pepper/pepper_flash_renderer_host.h"

#include <map>
#include <vector>

#include "base/lazy_instance.h"
#include "base/metrics/histogram.h"
#include "base/strings/string_util.h"
#include "content/public/renderer/pepper_plugin_instance.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/renderer_ppapi_host.h"
#include "ipc/ipc_message_macros.h"
#include "net/http/http_util.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/trusted/ppb_browser_font_trusted.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/proxy/host_dispatcher.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/resource_message_params.h"
#include "ppapi/proxy/serialized_structs.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_image_data_api.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPoint.h"
#include "third_party/skia/include/core/SkTypeface.h"
#include "ui/gfx/geometry/rect.h"
#include "url/gurl.h"

using ppapi::thunk::EnterResourceNoLock;
using ppapi::thunk::PPB_ImageData_API;

namespace {

// Some non-simple HTTP request headers that Flash may set.
// (Please see http://www.w3.org/TR/cors/#simple-header for the definition of
// simple headers.)
//
// The list and the enum defined below are used to collect data about request
// headers used in PPB_Flash.Navigate() calls, in order to understand the impact
// of rejecting PPB_Flash.Navigate() requests with non-simple headers.
//
// TODO(yzshen): We should be able to remove the histogram recording code once
// we get the answer.
const char* const kRejectedHttpRequestHeaders[] = {
    "authorization",     //
    "cache-control",     //
    "content-encoding",  //
    "content-md5",       //
    "content-type",      // If the media type is not one of those covered by the
                         // simple header definition.
    "expires",           //
    "from",              //
    "if-match",          //
    "if-none-match",     //
    "if-range",          //
    "if-unmodified-since",  //
    "pragma",               //
    "referer"               //
};

// Please note that new entries should be added right above
// FLASH_NAVIGATE_USAGE_ENUM_COUNT, and existing entries shouldn't be re-ordered
// or removed, since this ordering is used in a histogram.
enum FlashNavigateUsage {
  // This section must be in the same order as kRejectedHttpRequestHeaders.
  REJECT_AUTHORIZATION = 0,
  REJECT_CACHE_CONTROL,
  REJECT_CONTENT_ENCODING,
  REJECT_CONTENT_MD5,
  REJECT_CONTENT_TYPE,
  REJECT_EXPIRES,
  REJECT_FROM,
  REJECT_IF_MATCH,
  REJECT_IF_NONE_MATCH,
  REJECT_IF_RANGE,
  REJECT_IF_UNMODIFIED_SINCE,
  REJECT_PRAGMA,
  REJECT_REFERER,

  // The navigate request is rejected because of headers not listed above
  // (e.g., custom headers).
  REJECT_OTHER_HEADERS,

  // Total number of rejected navigate requests.
  TOTAL_REJECTED_NAVIGATE_REQUESTS,

  // Total number of navigate requests.
  TOTAL_NAVIGATE_REQUESTS,
  FLASH_NAVIGATE_USAGE_ENUM_COUNT
};

static base::LazyInstance<std::map<std::string, FlashNavigateUsage> >
    g_rejected_headers = LAZY_INSTANCE_INITIALIZER;

bool IsSimpleHeader(const std::string& lower_case_header_name,
                    const std::string& header_value) {
  if (lower_case_header_name == "accept" ||
      lower_case_header_name == "accept-language" ||
      lower_case_header_name == "content-language") {
    return true;
  }

  if (lower_case_header_name == "content-type") {
    std::string lower_case_mime_type;
    std::string lower_case_charset;
    bool had_charset = false;
    net::HttpUtil::ParseContentType(header_value,
                                    &lower_case_mime_type,
                                    &lower_case_charset,
                                    &had_charset,
                                    NULL);
    return lower_case_mime_type == "application/x-www-form-urlencoded" ||
           lower_case_mime_type == "multipart/form-data" ||
           lower_case_mime_type == "text/plain";
  }

  return false;
}

void RecordFlashNavigateUsage(FlashNavigateUsage usage) {
  DCHECK_NE(FLASH_NAVIGATE_USAGE_ENUM_COUNT, usage);
  UMA_HISTOGRAM_ENUMERATION(
      "Plugin.FlashNavigateUsage", usage, FLASH_NAVIGATE_USAGE_ENUM_COUNT);
}

}  // namespace

PepperFlashRendererHost::PepperFlashRendererHost(
    content::RendererPpapiHost* host,
    PP_Instance instance,
    PP_Resource resource)
    : ResourceHost(host->GetPpapiHost(), instance, resource),
      host_(host),
      weak_factory_(this) {}

PepperFlashRendererHost::~PepperFlashRendererHost() {
  // This object may be destroyed in the middle of a sync message. If that is
  // the case, make sure we respond to all the pending navigate calls.
  std::vector<ppapi::host::ReplyMessageContext>::reverse_iterator it;
  for (it = navigate_replies_.rbegin(); it != navigate_replies_.rend(); ++it)
    SendReply(*it, IPC::Message());
}

int32_t PepperFlashRendererHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperFlashRendererHost, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_Flash_GetProxyForURL,
                                      OnGetProxyForURL)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_Flash_SetInstanceAlwaysOnTop,
                                      OnSetInstanceAlwaysOnTop)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_Flash_DrawGlyphs,
                                      OnDrawGlyphs)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_Flash_Navigate, OnNavigate)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_Flash_IsRectTopmost,
                                      OnIsRectTopmost)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

int32_t PepperFlashRendererHost::OnGetProxyForURL(
    ppapi::host::HostMessageContext* host_context,
    const std::string& url) {
  GURL gurl(url);
  if (!gurl.is_valid())
    return PP_ERROR_FAILED;
  std::string proxy;
  bool result = content::RenderThread::Get()->ResolveProxy(gurl, &proxy);
  if (!result)
    return PP_ERROR_FAILED;
  host_context->reply_msg = PpapiPluginMsg_Flash_GetProxyForURLReply(proxy);
  return PP_OK;
}

int32_t PepperFlashRendererHost::OnSetInstanceAlwaysOnTop(
    ppapi::host::HostMessageContext* host_context,
    bool on_top) {
  content::PepperPluginInstance* plugin_instance =
      host_->GetPluginInstance(pp_instance());
  if (plugin_instance)
    plugin_instance->SetAlwaysOnTop(on_top);
  // Since no reply is sent for this message, it doesn't make sense to return an
  // error.
  return PP_OK;
}

int32_t PepperFlashRendererHost::OnDrawGlyphs(
    ppapi::host::HostMessageContext* host_context,
    ppapi::proxy::PPBFlash_DrawGlyphs_Params params) {
  if (params.glyph_indices.size() != params.glyph_advances.size() ||
      params.glyph_indices.empty())
    return PP_ERROR_FAILED;

  // Set up the typeface.
  int style = SkTypeface::kNormal;
  if (static_cast<PP_BrowserFont_Trusted_Weight>(params.font_desc.weight) >=
      PP_BROWSERFONT_TRUSTED_WEIGHT_BOLD)
    style |= SkTypeface::kBold;
  if (params.font_desc.italic)
    style |= SkTypeface::kItalic;
  sk_sp<SkTypeface> typeface(SkTypeface::CreateFromName(
      params.font_desc.face.c_str(), static_cast<SkTypeface::Style>(style)));
  if (!typeface)
    return PP_ERROR_FAILED;

  EnterResourceNoLock<PPB_ImageData_API> enter(
      params.image_data.host_resource(), true);
  if (enter.failed())
    return PP_ERROR_FAILED;

  // Set up the canvas.
  PPB_ImageData_API* image = static_cast<PPB_ImageData_API*>(enter.object());
  SkCanvas* canvas = image->GetCanvas();
  bool needs_unmapping = false;
  if (!canvas) {
    needs_unmapping = true;
    image->Map();
    canvas = image->GetCanvas();
    if (!canvas)
      return PP_ERROR_FAILED;  // Failure mapping.
  }

  SkAutoCanvasRestore acr(canvas, true);

  // Clip is applied in pixels before the transform.
  SkRect clip_rect = {
      SkIntToScalar(params.clip.point.x), SkIntToScalar(params.clip.point.y),
      SkIntToScalar(params.clip.point.x + params.clip.size.width),
      SkIntToScalar(params.clip.point.y + params.clip.size.height)};
  canvas->clipRect(clip_rect);

  // Convert & set the matrix.
  SkMatrix matrix;
  matrix.set(SkMatrix::kMScaleX, SkFloatToScalar(params.transformation[0][0]));
  matrix.set(SkMatrix::kMSkewX, SkFloatToScalar(params.transformation[0][1]));
  matrix.set(SkMatrix::kMTransX, SkFloatToScalar(params.transformation[0][2]));
  matrix.set(SkMatrix::kMSkewY, SkFloatToScalar(params.transformation[1][0]));
  matrix.set(SkMatrix::kMScaleY, SkFloatToScalar(params.transformation[1][1]));
  matrix.set(SkMatrix::kMTransY, SkFloatToScalar(params.transformation[1][2]));
  matrix.set(SkMatrix::kMPersp0, SkFloatToScalar(params.transformation[2][0]));
  matrix.set(SkMatrix::kMPersp1, SkFloatToScalar(params.transformation[2][1]));
  matrix.set(SkMatrix::kMPersp2, SkFloatToScalar(params.transformation[2][2]));
  canvas->concat(matrix);

  SkPaint paint;
  paint.setColor(params.color);
  paint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);
  paint.setAntiAlias(true);
  paint.setHinting(SkPaint::kFull_Hinting);
  paint.setTextSize(SkIntToScalar(params.font_desc.size));
  paint.setTypeface(std::move(typeface));
  if (params.allow_subpixel_aa) {
    paint.setSubpixelText(true);
    paint.setLCDRenderText(true);
  }

  SkScalar x = SkIntToScalar(params.position.x);
  SkScalar y = SkIntToScalar(params.position.y);

  // Build up the skia advances.
  size_t glyph_count = params.glyph_indices.size();
  if (glyph_count) {
    std::vector<SkPoint> storage;
    storage.resize(glyph_count);
    SkPoint* sk_positions = &storage[0];
    for (uint32_t i = 0; i < glyph_count; i++) {
      sk_positions[i].set(x, y);
      x += SkFloatToScalar(params.glyph_advances[i].x);
      y += SkFloatToScalar(params.glyph_advances[i].y);
    }

    canvas->drawPosText(
        &params.glyph_indices[0], glyph_count * 2, sk_positions, paint);
  }

  if (needs_unmapping)
    image->Unmap();

  return PP_OK;
}

// CAUTION: This code is subtle because Navigate is a sync call which may
// cause re-entrancy or cause the instance to be destroyed. If the instance
// is destroyed we need to ensure that we respond to all outstanding sync
// messages so that the plugin process does not remain blocked.
int32_t PepperFlashRendererHost::OnNavigate(
    ppapi::host::HostMessageContext* host_context,
    const ppapi::URLRequestInfoData& data,
    const std::string& target,
    bool from_user_action) {
  // If our PepperPluginInstance is already destroyed, just return a failure.
  content::PepperPluginInstance* plugin_instance =
      host_->GetPluginInstance(pp_instance());
  if (!plugin_instance)
    return PP_ERROR_FAILED;

  std::map<std::string, FlashNavigateUsage>& rejected_headers =
      g_rejected_headers.Get();
  if (rejected_headers.empty()) {
    for (size_t i = 0; i < arraysize(kRejectedHttpRequestHeaders); ++i)
      rejected_headers[kRejectedHttpRequestHeaders[i]] =
          static_cast<FlashNavigateUsage>(i);
  }

  net::HttpUtil::HeadersIterator header_iter(
      data.headers.begin(), data.headers.end(), "\n\r");
  bool rejected = false;
  while (header_iter.GetNext()) {
    std::string lower_case_header_name =
        base::ToLowerASCII(header_iter.name());
    if (!IsSimpleHeader(lower_case_header_name, header_iter.values())) {
      rejected = true;

      std::map<std::string, FlashNavigateUsage>::const_iterator iter =
          rejected_headers.find(lower_case_header_name);
      FlashNavigateUsage usage =
          iter != rejected_headers.end() ? iter->second : REJECT_OTHER_HEADERS;
      RecordFlashNavigateUsage(usage);
    }
  }

  RecordFlashNavigateUsage(TOTAL_NAVIGATE_REQUESTS);
  if (rejected) {
    RecordFlashNavigateUsage(TOTAL_REJECTED_NAVIGATE_REQUESTS);
    return PP_ERROR_NOACCESS;
  }

  // Navigate may call into Javascript (e.g. with a "javascript:" URL),
  // or do things like navigate away from the page, either one of which will
  // need to re-enter into the plugin. It is safe, because it is essentially
  // equivalent to NPN_GetURL, where Flash would expect re-entrancy.
  ppapi::proxy::HostDispatcher* host_dispatcher =
      ppapi::proxy::HostDispatcher::GetForInstance(pp_instance());
  host_dispatcher->set_allow_plugin_reentrancy();

  // Grab a weak pointer to ourselves on the stack so we can check if we are
  // still alive.
  base::WeakPtr<PepperFlashRendererHost> weak_ptr = weak_factory_.GetWeakPtr();
  // Keep track of reply contexts in case we are destroyed during a Navigate
  // call. Even if we are destroyed, we still need to send these replies to
  // unblock the plugin process.
  navigate_replies_.push_back(host_context->MakeReplyMessageContext());
  plugin_instance->Navigate(data, target.c_str(), from_user_action);
  // This object might have been destroyed by this point. If it is destroyed
  // the reply will be sent in the destructor. Otherwise send the reply here.
  if (weak_ptr.get()) {
    SendReply(navigate_replies_.back(), IPC::Message());
    navigate_replies_.pop_back();
  }

  // Return PP_OK_COMPLETIONPENDING so that no reply is automatically sent.
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperFlashRendererHost::OnIsRectTopmost(
    ppapi::host::HostMessageContext* host_context,
    const PP_Rect& rect) {
  content::PepperPluginInstance* plugin_instance =
      host_->GetPluginInstance(pp_instance());
  if (plugin_instance &&
      plugin_instance->IsRectTopmost(gfx::Rect(
          rect.point.x, rect.point.y, rect.size.width, rect.size.height)))
    return PP_OK;
  return PP_ERROR_FAILED;
}
