// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/cocoa/electron_render_widget_host_view_mac_delegate.h"

#include "base/strings/sys_string_conversions.h"
#include "components/spellcheck/browser/spellcheck_platform.h"
#include "components/spellcheck/common/spellcheck_panel.mojom.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/service_manager/public/cpp/interface_provider.h"

@implementation ElectronRenderWidgetHostViewMacDelegate

- (id)initWithRenderWidgetHost:(content::RenderWidgetHost*)renderWidgetHost {
  self = [super init];
  if (self) {
    _renderWidgetHost = renderWidgetHost;
  }
  return self;
}

- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item
                      isValidItem:(BOOL*)valid {
  SEL action = [item action];
  if (action == @selector(checkSpelling:)) {
    *valid = content::RenderViewHost::From(_renderWidgetHost) != nullptr;
    return YES;
  }
  if (action == @selector(showGuessPanel:)) {
    *valid = YES;
    return YES;
  }
  return NO;
}

// Spellchecking methods
// The implementations are copied from
// chrome/browser/renderer_host/chrome_render_widget_host_view_mac_delegate.mm

// This message is sent whenever the user specifies that a word should be
// changed from the spellChecker.
- (void)changeSpelling:(id)sender {
  // Grab the currently selected word from the spell panel, as this is the word
  // that we want to replace the selected word in the text with.
  NSString* newWord = [[sender selectedCell] stringValue];
  if (newWord != nil) {
    content::WebContents* webContents =
        content::WebContents::FromRenderViewHost(
            content::RenderViewHost::From(_renderWidgetHost));
    webContents->ReplaceMisspelling(base::SysNSStringToUTF16(newWord));
  }
}

// This message is sent by NSSpellChecker whenever the next word should be
// advanced to, either after a correction or clicking the "Find Next" button.
// This isn't documented anywhere useful, like in NSSpellProtocol.h with the
// other spelling panel methods. This is probably because Apple assumes that the
// the spelling panel will be used with an NSText, which will automatically
// catch this and advance to the next word for you. Thanks Apple.
// This is also called from the Edit -> Spelling -> Check Spelling menu item.
- (void)checkSpelling:(id)sender {
  content::WebContents* webContents = content::WebContents::FromRenderViewHost(
      content::RenderViewHost::From(_renderWidgetHost));
  if (webContents && webContents->GetFocusedFrame()) {
    mojo::Remote<spellcheck::mojom::SpellCheckPanel>
        focused_spell_check_panel_client;
    webContents->GetFocusedFrame()->GetRemoteInterfaces()->GetInterface(
        focused_spell_check_panel_client.BindNewPipeAndPassReceiver());
    focused_spell_check_panel_client->AdvanceToNextMisspelling();
  }
}

// This message is sent by the spelling panel whenever a word is ignored.
- (void)ignoreSpelling:(id)sender {
  // Ideally, we would ask the current RenderView for its tag, but that would
  // mean making a blocking IPC call from the browser. Instead,
  // spellcheck_platform::CheckSpelling remembers the last tag and
  // spellcheck_platform::IgnoreWord assumes that is the correct tag.
  NSString* wordToIgnore = [sender stringValue];
  if (wordToIgnore != nil)
    spellcheck_platform::IgnoreWord(nullptr,
                                    base::SysNSStringToUTF16(wordToIgnore));
}

- (void)showGuessPanel:(id)sender {
  const bool visible = spellcheck_platform::SpellingPanelVisible();

  content::WebContents* webContents = content::WebContents::FromRenderViewHost(
      content::RenderViewHost::From(_renderWidgetHost));
  DCHECK(webContents && webContents->GetFocusedFrame());

  mojo::Remote<spellcheck::mojom::SpellCheckPanel>
      focused_spell_check_panel_client;
  webContents->GetFocusedFrame()->GetRemoteInterfaces()->GetInterface(
      focused_spell_check_panel_client.BindNewPipeAndPassReceiver());
  focused_spell_check_panel_client->ToggleSpellPanel(visible);
}

// END Spellchecking methods

// RenderWidgetHostViewMacDelegate events

- (void)beginGestureWithEvent:(NSEvent*)event {
}

- (void)endGestureWithEvent:(NSEvent*)event {
}

- (void)touchesMovedWithEvent:(NSEvent*)event {
}

- (void)touchesBeganWithEvent:(NSEvent*)event {
}

- (void)touchesCancelledWithEvent:(NSEvent*)event {
}

- (void)touchesEndedWithEvent:(NSEvent*)event {
}

- (void)rendererHandledWheelEvent:(const blink::WebMouseWheelEvent&)event
                         consumed:(BOOL)consumed {
}

- (void)rendererHandledGestureScrollEvent:(const blink::WebGestureEvent&)event
                                 consumed:(BOOL)consumed {
}

- (void)rendererHandledOverscrollEvent:(const ui::DidOverscrollParams&)params {
}

@end
