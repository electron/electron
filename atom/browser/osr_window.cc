// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/osr_window.h"
#include "third_party/WebKit/public/platform/WebScreenInfo.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_delegate.h"
#include "content/public/browser/render_widget_host_view_frame_subscriber.h"
#include "ui/events/latency_info.h"
#include "content/common/view_messages.h"
#include "ui/gfx/geometry/dip_util.h"
#include "base/memory/ptr_util.h"
#include "content/public/browser/context_factory.h"
#include "base/single_thread_task_runner.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_type.h"
#include "base/location.h"
#include "ui/gfx/native_widget_types.h"
#include <iostream>
#include <chrono>
#include <thread>

#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/core/SkPixmap.h"
#include "cc/output/output_surface_client.h"

namespace atom {

OffScreenWebContentsView::OffScreenWebContentsView() {
  std::cout << "OffScreenWebContentsView" << std::endl;
  //std::this_thread::sleep_for(std::chrono::milliseconds(10000));
}
OffScreenWebContentsView::~OffScreenWebContentsView() {
  std::cout << "~OffScreenWebContentsView" << std::endl;
}

// Returns the native widget that contains the contents of the tab.
gfx::NativeView OffScreenWebContentsView::GetNativeView() const{
  std::cout << "GetNativeView" << std::endl;
  return gfx::NativeView();
}

// Returns the native widget with the main content of the tab (i.e. the main
// render view host, though there may be many popups in the tab as children of
// the container).
gfx::NativeView OffScreenWebContentsView::GetContentNativeView() const{
  std::cout << "GetContentNativeView" << std::endl;
  return gfx::NativeView();
}

// Returns the outermost native view. This will be used as the parent for
// dialog boxes.
gfx::NativeWindow OffScreenWebContentsView::GetTopLevelNativeWindow() const{
  std::cout << "GetTopLevelNativeWindow" << std::endl;
  return gfx::NativeWindow();
}

// Computes the rectangle for the native widget that contains the contents of
// the tab in the screen coordinate system.
void OffScreenWebContentsView::GetContainerBounds(gfx::Rect* out) const{
  std::cout << "GetContainerBounds" << std::endl;
  *out = GetViewBounds();
}

// TODO(brettw) this is a hack. It's used in two places at the time of this
// writing: (1) when render view hosts switch, we need to size the replaced
// one to be correct, since it wouldn't have known about sizes that happened
// while it was hidden; (2) in constrained windows.
//
// (1) will be fixed once interstitials are cleaned up. (2) seems like it
// should be cleaned up or done some other way, since this works for normal
// WebContents without the special code.
void OffScreenWebContentsView::SizeContents(const gfx::Size& size){
  std::cout << "SizeContents" << std::endl;
}

// Sets focus to the native widget for this tab.
void OffScreenWebContentsView::Focus(){
  std::cout << "OffScreenWebContentsView::Focus" << std::endl;
}

// Sets focus to the appropriate element when the WebContents is shown the
// first time.
void OffScreenWebContentsView::SetInitialFocus(){
  std::cout << "SetInitialFocus" << std::endl;
}

// Stores the currently focused view.
void OffScreenWebContentsView::StoreFocus(){
  std::cout << "StoreFocus" << std::endl;
}

// Restores focus to the last focus view. If StoreFocus has not yet been
// invoked, SetInitialFocus is invoked.
void OffScreenWebContentsView::RestoreFocus(){
  std::cout << "RestoreFocus" << std::endl;
}

// Returns the current drop data, if any.
content::DropData* OffScreenWebContentsView::GetDropData() const{
  std::cout << "GetDropData" << std::endl;
  return nullptr;
}

// Get the bounds of the View, relative to the parent.
gfx::Rect OffScreenWebContentsView::GetViewBounds() const{
  std::cout << "OffScreenWebContentsView::GetViewBounds" << std::endl;
  return view_ ? view_->GetViewBounds() : gfx::Rect();
}

void OffScreenWebContentsView::CreateView(
    const gfx::Size& initial_size, gfx::NativeView context){
  std::cout << "CreateView" << std::endl;
  std::cout << initial_size.width() << "x" << initial_size.height() << std::endl;
}

// Sets up the View that holds the rendered web page, receives messages for
// it and contains page plugins. The host view should be sized to the current
// size of the WebContents.
//
// |is_guest_view_hack| is temporary hack and will be removed once
// RenderWidgetHostViewGuest is not dependent on platform view.
// TODO(lazyboy): Remove |is_guest_view_hack| once http://crbug.com/330264 is
// fixed.
content::RenderWidgetHostViewBase*
  OffScreenWebContentsView::CreateViewForWidget(
    content::RenderWidgetHost* render_widget_host, bool is_guest_view_hack){
  std::cout << "CreateViewForWidget" << std::endl;
  view_ = new OffScreenWindow(render_widget_host);
  return view_;
}

// Creates a new View that holds a popup and receives messages for it.
content::RenderWidgetHostViewBase*
  OffScreenWebContentsView::CreateViewForPopupWidget(
    content::RenderWidgetHost* render_widget_host){
  std::cout << "CreateViewForPopupWidget" << std::endl;
  view_ = new OffScreenWindow(render_widget_host);
  return view_;
}

// Sets the page title for the native widgets corresponding to the view. This
// is not strictly necessary and isn't expected to be displayed anywhere, but
// can aid certain debugging tools such as Spy++ on Windows where you are
// trying to find a specific window.
void OffScreenWebContentsView::SetPageTitle(const base::string16& title){
  std::cout << "SetPageTitle" << std::endl;
  std::cout << title << std::endl;
}

// Invoked when the WebContents is notified that the RenderView has been
// fully created.
void OffScreenWebContentsView::RenderViewCreated(content::RenderViewHost* host){
  std::cout << "RenderViewCreated" << std::endl;
}

// Invoked when the WebContents is notified that the RenderView has been
// swapped in.
void OffScreenWebContentsView::RenderViewSwappedIn(content::RenderViewHost* host){
  std::cout << "RenderViewSwappedIn" << std::endl;
}

// Invoked to enable/disable overscroll gesture navigation.
void OffScreenWebContentsView::SetOverscrollControllerEnabled(bool enabled){
  std::cout << "SetOverscrollControllerEnabled" << std::endl;
}






OffScreenOutputDevice::OffScreenOutputDevice() {
  std::cout << "OffScreenOutputDevice" << std::endl;
}

OffScreenOutputDevice::~OffScreenOutputDevice() {
  std::cout << "~OffScreenOutputDevice" << std::endl;
}

void OffScreenOutputDevice::Resize(
  const gfx::Size& pixel_size, float scale_factor) {
  std::cout << "Resize" << std::endl;
  std::cout << pixel_size.width() << "x" << pixel_size.height() << std::endl;

  scale_factor_ = scale_factor;

  if (viewport_pixel_size_ == pixel_size) return;
  viewport_pixel_size_ = pixel_size;

  canvas_.reset(NULL);
  bitmap_.reset(new SkBitmap);
  bitmap_->allocN32Pixels(viewport_pixel_size_.width(),
                          viewport_pixel_size_.height(),
                          false);
  if (bitmap_->drawsNothing()) {
    std::cout << "drawsNothing" << std::endl;
    NOTREACHED();
    bitmap_.reset(NULL);
    return;
  }
  bitmap_->eraseARGB(0, 0, 0, 0);

  canvas_.reset(new SkCanvas(*bitmap_.get()));
}

SkCanvas* OffScreenOutputDevice::BeginPaint(const gfx::Rect& damage_rect) {
  std::cout << "BeginPaint" << std::endl;
  DCHECK(canvas_.get());
  DCHECK(bitmap_.get());

  damage_rect_ = damage_rect;

  return canvas_.get();
}

void OffScreenOutputDevice::saveSkBitmapToBMPFile(const SkBitmap& skBitmap, const char* path){
    typedef unsigned char UINT8;
    typedef signed char SINT8;
    typedef unsigned short UINT16;
    typedef signed short SINT16;
    typedef unsigned int UINT32;
    typedef signed int SINT32;

    struct BMP_FILEHDR // BMP file header
    {
        UINT32 bfSize; // size of file
        UINT16 bfReserved1;
        UINT16 bfReserved2;
        UINT32 bfOffBits; // pointer to the pixmap bits
    };

    struct BMP_INFOHDR // BMP information header
    {
        UINT32 biSize; // size of this struct
        UINT32 biWidth; // pixmap width
        UINT32 biHeight; // pixmap height
        UINT16 biPlanes; // should be 1
        UINT16 biBitCount; // number of bits per pixel
        UINT32 biCompression; // compression method
        UINT32 biSizeImage; // size of image
        UINT32 biXPelsPerMeter; // horizontal resolution
        UINT32 biYPelsPerMeter; // vertical resolution
        UINT32 biClrUsed; // number of colors used
        UINT32 biClrImportant; // number of important colors
    };
    #define BitmapColorGetA(color) (((color) >> 24) & 0xFF)
    #define BitmapColorGetR(color) (((color) >> 16) & 0xFF)
    #define BitmapColorGetG(color) (((color) >> 8) & 0xFF)
    #define BitmapColorGetB(color) (((color) >> 0) & 0xFF)

    int bmpWidth = skBitmap.width();
    int bmpHeight = skBitmap.height();
    int stride = skBitmap.rowBytes();
    char* m_pmap = (char*)skBitmap.getPixels();
    //virtual PixelFormat& GetPixelFormat() =0; //assume pf is ARGB;
    FILE* fp = fopen(path, "wb");
    if(!fp){
        printf("saveSkBitmapToBMPFile: fopen %s Error!\n", path);
    }
    SINT32 bpl=bmpWidth*4;
    // BMP file header.
    BMP_FILEHDR fhdr;
    fputc('B', fp);
    fputc('M', fp);
    fhdr.bfReserved1=fhdr.bfReserved2=0;
    fhdr.bfOffBits=14+40; // File header size + header size.
    fhdr.bfSize=fhdr.bfOffBits+bpl*bmpHeight;
    fwrite(&fhdr, 1, 12, fp);

    // BMP header.
    BMP_INFOHDR bhdr;
    bhdr.biSize=40;
    bhdr.biBitCount=32;
    bhdr.biCompression=0; // RGB Format.
    bhdr.biPlanes=1;
    bhdr.biWidth=bmpWidth;
    bhdr.biHeight=bmpHeight;
    bhdr.biClrImportant=0;
    bhdr.biClrUsed=0;
    bhdr.biXPelsPerMeter=2384;
    bhdr.biYPelsPerMeter=2384;
    bhdr.biSizeImage=bpl*bmpHeight;
    fwrite(&bhdr, 1, 40, fp);

    // BMP data.
    //for(UINT32 y=0; y<m_height; y++)
    for(SINT32 y=bmpHeight-1; y>=0; y--)
    {
        SINT32 base=y*stride;
        for(SINT32 x=0; x<(SINT32)bmpWidth; x++)
        {
            UINT32 i=base+x*4;
            UINT32 pixelData = *(UINT32*)(m_pmap+i);
            UINT8 b1=BitmapColorGetB(pixelData);
            UINT8 g1=BitmapColorGetG(pixelData);
            UINT8 r1=BitmapColorGetR(pixelData);
            UINT8 a1=BitmapColorGetA(pixelData);
            r1=r1*a1/255;
            g1=g1*a1/255;
            b1=b1*a1/255;
            UINT32 temp=(a1<<24)|(r1<<16)|(g1<<8)|b1;//a bmp pixel in little endian is B、G、R、A
            fwrite(&temp, 4, 1, fp);
        }
    }
    fflush(fp);
    fclose(fp);
}

void OffScreenOutputDevice::EndPaint() {
  std::cout << "EndPaint" << std::endl;

  DCHECK(canvas_.get());
  DCHECK(bitmap_.get());

  if (!bitmap_.get()) return;

  cc::SoftwareOutputDevice::EndPaint();

  SkAutoLockPixels bitmap_pixels_lock(*bitmap_.get());
  //saveSkBitmapToBMPFile(*(bitmap_.get()), "test.bmp");

  uint8_t* pixels = reinterpret_cast<uint8_t*>(bitmap_->getPixels());
  for (int i = 0; i<4; i++) {
    int x = static_cast<int>(pixels[i]);
    std::cout << std::hex << x << std::dec << std::endl;
  }
}

OffScreenWindow::OffScreenWindow(content::RenderWidgetHost* host)
  : render_widget_host_(content::RenderWidgetHostImpl::From(host)),
    delegated_frame_host_(new content::DelegatedFrameHost(this)),
    compositor_widget_(gfx::kNullAcceleratedWidget),
    scale_factor_(1.0f),
    is_showing_(!render_widget_host_->is_hidden()),
    size_(gfx::Size(800, 600)),
    weak_ptr_factory_(this) {
  DCHECK(render_widget_host_);
  std::cout << "OffScreenWindow" << std::endl;
  render_widget_host_->SetView(this);

  root_layer_.reset(new ui::Layer(ui::LAYER_SOLID_COLOR));

  CreatePlatformWidget();

  compositor_.reset(new ui::Compositor(content::GetContextFactory(),
    base::ThreadTaskRunnerHandle::Get()));
  compositor_->SetAcceleratedWidget(compositor_widget_);
  compositor_->SetDelegate(this);
  compositor_->SetRootLayer(root_layer_.get());
}

OffScreenWindow::~OffScreenWindow() {
  std::cout << "~OffScreenWindow" << std::endl;
  if (is_showing_) delegated_frame_host_->WasHidden();
  delegated_frame_host_->ResetCompositor();

  delegated_frame_host_.reset(NULL);
  compositor_.reset(NULL);
  root_layer_.reset(NULL);
}

bool OffScreenWindow::OnMessageReceived(const IPC::Message& message) {
  std::cout << "OnMessageReceived" << std::endl;

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(OffScreenWindow, message)
    IPC_MESSAGE_HANDLER(ViewHostMsg_SetNeedsBeginFrames,
                        OnSetNeedsBeginFrames)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  if (!handled)
    return content::RenderWidgetHostViewBase::OnMessageReceived(message);
  return handled;
}

void OffScreenWindow::InitAsChild(gfx::NativeView) {
  std::cout << "InitAsChild" << std::endl;
}

content::RenderWidgetHost* OffScreenWindow::GetRenderWidgetHost() const {
  std::cout << "GetRenderWidgetHost" << std::endl;
  return render_widget_host_;
}

void OffScreenWindow::SetSize(const gfx::Size& size) {
  std::cout << "SetSize" << std::endl;
  size_ = size;

  std::cout << size.width() << "x" << size.height() << std::endl;

  const gfx::Size& size_in_pixels =
    gfx::ConvertSizeToPixel(scale_factor_, size);

  root_layer_->SetBounds(gfx::Rect(size));
  compositor_->SetScaleAndSize(scale_factor_, size_in_pixels);
}

void OffScreenWindow::SetBounds(const gfx::Rect& new_bounds) {
  std::cout << "SetBounds" << std::endl;
}

gfx::Vector2dF OffScreenWindow::GetLastScrollOffset() const {
  std::cout << "GetLastScrollOffset" << std::endl;
  return last_scroll_offset_;
}

gfx::NativeView OffScreenWindow::GetNativeView() const {
  std::cout << "GetNativeView" << std::endl;
  return gfx::NativeView();
}

gfx::NativeViewId OffScreenWindow::GetNativeViewId() const {
  std::cout << "GetNativeViewId" << std::endl;
  return gfx::NativeViewId();
}

gfx::NativeViewAccessible OffScreenWindow::GetNativeViewAccessible() {
  std::cout << "GetNativeViewAccessible" << std::endl;
  return gfx::NativeViewAccessible();
}

ui::TextInputClient* OffScreenWindow::GetTextInputClient() {
  std::cout << "GetTextInputClient" << std::endl;
  return nullptr;
}

void OffScreenWindow::Focus() {
  std::cout << "Focus" << std::endl;
}

bool OffScreenWindow::HasFocus() const {
  std::cout << "HasFocus" << std::endl;
  return false;
}

bool OffScreenWindow::IsSurfaceAvailableForCopy() const {
  std::cout << "IsSurfaceAvailableForCopy" << std::endl;
  return delegated_frame_host_->CanCopyToBitmap();
}

void OffScreenWindow::Show() {
  std::cout << "Show" << std::endl;
  if (is_showing_)
    return;

  is_showing_ = true;
  if (render_widget_host_) render_widget_host_->WasShown(ui::LatencyInfo());
  delegated_frame_host_->SetCompositor(compositor_.get());
  delegated_frame_host_->WasShown(ui::LatencyInfo());
}

void OffScreenWindow::Hide() {
  std::cout << "Hide" << std::endl;
  if (!is_showing_)
    return;

  if (render_widget_host_) render_widget_host_->WasHidden();
  delegated_frame_host_->WasHidden();
  delegated_frame_host_->ResetCompositor();
  is_showing_ = false;
}

bool OffScreenWindow::IsShowing() {
  std::cout << "IsShowing" << std::endl;
  return is_showing_;
}

gfx::Rect OffScreenWindow::GetViewBounds() const {
  std::cout << "GetViewBounds" << std::endl;
  std::cout << size_.width() << "x" << size_.height() << std::endl;
  return gfx::Rect(size_);
}

gfx::Size OffScreenWindow::GetVisibleViewportSize() const {
  std::cout << "GetVisibleViewportSize" << std::endl;
  std::cout << size_.width() << "x" << size_.height() << std::endl;
  return size_;
}

void OffScreenWindow::SetInsets(const gfx::Insets& insets) {
  std::cout << "SetInsets" << std::endl;
}

bool OffScreenWindow::LockMouse() {
  std::cout << "LockMouse" << std::endl;
  return false;
}

void OffScreenWindow::UnlockMouse() {
  std::cout << "UnlockMouse" << std::endl;
}

bool OffScreenWindow::GetScreenColorProfile(std::vector<char>*) {
  std::cout << "GetScreenColorProfile" << std::endl;
  return false;
}

void OffScreenWindow::OnSwapCompositorFrame(
  uint32_t output_surface_id,
  std::unique_ptr<cc::CompositorFrame> frame) {
  std::cout << "OnSwapCompositorFrame" << std::endl;

  std::cout << output_surface_id << std::endl;

  if (frame->delegated_frame_data)
    std::cout << "delegated_frame_data" << std::endl;

  if (frame->metadata.root_scroll_offset != last_scroll_offset_) {
    last_scroll_offset_ = frame->metadata.root_scroll_offset;
  }

  if (frame->delegated_frame_data)
    delegated_frame_host_->SwapDelegatedFrame(
      output_surface_id, std::move(frame));
}

void OffScreenWindow::ClearCompositorFrame() {
  std::cout << "ClearCompositorFrame" << std::endl;
  delegated_frame_host_->ClearDelegatedFrame();
}

void OffScreenWindow::InitAsPopup(
  content::RenderWidgetHostView *, const gfx::Rect &) {
  std::cout << "InitAsPopup" << std::endl;
}

void OffScreenWindow::InitAsFullscreen(content::RenderWidgetHostView *) {
  std::cout << "InitAsFullscreen" << std::endl;
}

void OffScreenWindow::UpdateCursor(const content::WebCursor &) {
  std::cout << "UpdateCursor" << std::endl;
}

void OffScreenWindow::SetIsLoading(bool loading) {
  std::cout << "SetIsLoading" << std::endl;
}

void OffScreenWindow::TextInputStateChanged(
  const ViewHostMsg_TextInputState_Params &) {
  std::cout << "TextInputStateChanged" << std::endl;
}

void OffScreenWindow::ImeCancelComposition() {
  std::cout << "ImeCancelComposition" << std::endl;
}

void OffScreenWindow::RenderProcessGone(base::TerminationStatus,int) {
  std::cout << "RenderProcessGone" << std::endl;
  Destroy();
}

void OffScreenWindow::Destroy() {
  std::cout << "Destroy" << std::endl;
}

void OffScreenWindow::SetTooltipText(const base::string16 &) {
  std::cout << "SetTooltipText" << std::endl;
}

void OffScreenWindow::SelectionBoundsChanged(
  const ViewHostMsg_SelectionBounds_Params &) {
  std::cout << "SelectionBoundsChanged" << std::endl;
}

void OffScreenWindow::CopyFromCompositingSurface(const gfx::Rect& src_subrect,
  const gfx::Size& dst_size,
  const content::ReadbackRequestCallback& callback,
  const SkColorType preferred_color_type) {
  std::cout << "CopyFromCompositingSurface" << std::endl;
  delegated_frame_host_->CopyFromCompositingSurface(
    src_subrect, dst_size, callback, preferred_color_type);
}

void OffScreenWindow::CopyFromCompositingSurfaceToVideoFrame(
  const gfx::Rect& src_subrect,
  const scoped_refptr<media::VideoFrame>& target,
  const base::Callback<void (const gfx::Rect&, bool)>& callback) {
  std::cout << "CopyFromCompositingSurfaceToVideoFrame" << std::endl;
  delegated_frame_host_->CopyFromCompositingSurfaceToVideoFrame(
    src_subrect, target, callback);
}

bool OffScreenWindow::CanCopyToVideoFrame() const {
  std::cout << "CanCopyToVideoFrame" << std::endl;
  return delegated_frame_host_->CanCopyToVideoFrame();
}

void OffScreenWindow::BeginFrameSubscription(
  std::unique_ptr<content::RenderWidgetHostViewFrameSubscriber> subscriber) {
  std::cout << "BeginFrameSubscription" << std::endl;
  delegated_frame_host_->BeginFrameSubscription(std::move(subscriber));
}

void OffScreenWindow::EndFrameSubscription() {
  std::cout << "EndFrameSubscription" << std::endl;
  delegated_frame_host_->EndFrameSubscription();
}

bool OffScreenWindow::HasAcceleratedSurface(const gfx::Size &) {
  std::cout << "HasAcceleratedSurface" << std::endl;
  return false;
}

void OffScreenWindow::GetScreenInfo(blink::WebScreenInfo* results) {
  std::cout << "GetScreenInfo" << std::endl;
  std::cout << size_.width() << "x" << size_.height() << std::endl;
  results->rect = gfx::Rect(size_);
  results->availableRect = gfx::Rect(size_);
  results->depth = 24;
  results->depthPerComponent = 8;
  results->deviceScaleFactor = scale_factor_;
  results->orientationAngle = 0;
  results->orientationType = blink::WebScreenOrientationLandscapePrimary;
}

bool OffScreenWindow::GetScreenColorProfile(blink::WebVector<char>*) {
  std::cout << "GetScreenColorProfile" << std::endl;
  return false;
}

gfx::Rect OffScreenWindow::GetBoundsInRootWindow() {
  std::cout << "GetBoundsInRootWindow" << std::endl;
  std::cout << size_.width() << "x" << size_.height() << std::endl;
  return gfx::Rect(size_);
}

void OffScreenWindow::LockCompositingSurface() {
  std::cout << "LockCompositingSurface" << std::endl;
}

void OffScreenWindow::UnlockCompositingSurface() {
  std::cout << "UnlockCompositingSurface" << std::endl;
}

void OffScreenWindow::ImeCompositionRangeChanged(
  const gfx::Range &, const std::vector<gfx::Rect>&) {
  std::cout << "ImeCompositionRangeChanged" << std::endl;
}

gfx::Size OffScreenWindow::GetPhysicalBackingSize() const {
  std::cout << "GetPhysicalBackingSize" << std::endl;
  return size_;
}

gfx::Size OffScreenWindow::GetRequestedRendererSize() const {
  std::cout << "GetRequestedRendererSize" << std::endl;
  return size_;
}

int OffScreenWindow::DelegatedFrameHostGetGpuMemoryBufferClientId() const {
  std::cout << "DelegatedFrameHostGetGpuMemoryBufferClientId" << std::endl;
  return render_widget_host_->GetProcess()->GetID();
}

ui::Layer* OffScreenWindow::DelegatedFrameHostGetLayer() const {
  std::cout << "DelegatedFrameHostGetLayer" << std::endl;
  return const_cast<ui::Layer*>(root_layer_.get());
}

bool OffScreenWindow::DelegatedFrameHostIsVisible() const {
  std::cout << "DelegatedFrameHostIsVisible" << std::endl;
  std::cout << !render_widget_host_->is_hidden() << std::endl;
  return !render_widget_host_->is_hidden();
}

SkColor OffScreenWindow::DelegatedFrameHostGetGutterColor(SkColor color) const {
  std::cout << "DelegatedFrameHostGetGutterColor" << std::endl;
  return color;
}

gfx::Size OffScreenWindow::DelegatedFrameHostDesiredSizeInDIP() const {
  std::cout << "DelegatedFrameHostDesiredSizeInDIP" << std::endl;
  std::cout << size_.width() << "x" << size_.height() << std::endl;
  return size_;
}

bool OffScreenWindow::DelegatedFrameCanCreateResizeLock() const {
  std::cout << "DelegatedFrameCanCreateResizeLock" << std::endl;
  return false;
}

std::unique_ptr<content::ResizeLock>
  OffScreenWindow::DelegatedFrameHostCreateResizeLock(bool) {
  std::cout << "DelegatedFrameHostCreateResizeLock" << std::endl;
  return nullptr;
}

void OffScreenWindow::DelegatedFrameHostResizeLockWasReleased() {
  std::cout << "DelegatedFrameHostResizeLockWasReleased" << std::endl;
  return render_widget_host_->WasResized();
}

void OffScreenWindow::DelegatedFrameHostSendCompositorSwapAck(
  int output_surface_id, const cc::CompositorFrameAck& ack) {
  std::cout << "DelegatedFrameHostSendCompositorSwapAck" << std::endl;
  std::cout << output_surface_id << std::endl;
  render_widget_host_->Send(new ViewMsg_SwapCompositorFrameAck(
    render_widget_host_->GetRoutingID(),
    output_surface_id, ack));
}

void OffScreenWindow::DelegatedFrameHostSendReclaimCompositorResources(
  int output_surface_id, const cc::CompositorFrameAck& ack) {
  std::cout << "DelegatedFrameHostSendReclaimCompositorResources" << std::endl;
  std::cout << output_surface_id << std::endl;
  render_widget_host_->Send(new ViewMsg_ReclaimCompositorResources(
    render_widget_host_->GetRoutingID(),
    output_surface_id, ack));
}

void OffScreenWindow::DelegatedFrameHostOnLostCompositorResources() {
  std::cout << "DelegatedFrameHostOnLostCompositorResources" << std::endl;
  render_widget_host_->ScheduleComposite();
}

void OffScreenWindow::DelegatedFrameHostUpdateVSyncParameters(
  const base::TimeTicks& timebase, const base::TimeDelta& interval) {
  std::cout << "DelegatedFrameHostUpdateVSyncParameters" << std::endl;
  render_widget_host_->UpdateVSyncParameters(timebase, interval);
}

std::unique_ptr<cc::SoftwareOutputDevice>
  OffScreenWindow::CreateSoftwareOutputDevice(ui::Compositor* compositor) {
  std::cout << "CreateSoftwareOutputDevice" << std::endl;
  return std::unique_ptr<cc::SoftwareOutputDevice>(new OffScreenOutputDevice);
}

void OffScreenWindow::OnSetNeedsBeginFrames(bool enabled) {
  std::cout << "OnSetNeedsBeginFrames" << std::endl;
}

}  // namespace atom
