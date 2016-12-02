// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "atom/browser/api/atom_api_app_mac.h"

#include "atom/browser/native_window.h"
#include "atom/browser/unresponsive_suppressor.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/sys_string_conversions.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "content/public/browser/web_contents.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

@interface ElectronMediaController : NSObject {
}
- (void)setApp:(atom::api::App*)atomApp;
- (void)setOptions:(mate::Dictionary)opts;
@end

@implementation ElectronMediaController
  atom::api::App* _app;

- (void)setApp:(atom::api::App*)atomApp {
  _app = atomApp;
  _app->Emit("foo");
}

- (void)setOptions:(mate::Dictionary)opts {
  bool skipB = false;
  opts.Get("skipBackward", &skipB);
  bool skipF = false;
  opts.Get("skipForward", &skipF);
  bool seekB = false;
  opts.Get("seekBackward", &seekB);
  bool seekF = false;
  opts.Get("seekForward", &seekF);
  bool togglePP = false;
  opts.Get("togglePlayPause", &togglePP);
  bool pause = false;
  opts.Get("pause", &pause);
  bool play = false;
  opts.Get("play", &play);
  bool stop = false;
  opts.Get("stop", &stop);
  bool changeRate = false;
  opts.Get("changeRate", &changeRate);
  bool nextTrack = false;
  opts.Get("nextTrack", &nextTrack);
  bool prevTrack = false;
  opts.Get("previousTrack", &prevTrack);
  bool changeRating = false;
  opts.Get("rating", &changeRating);
  bool changeLike = false;
  opts.Get("like", &changeLike);
  bool changeDislike = false;
  opts.Get("dislike", &changeDislike);

  MPRemoteCommandCenter *remoteCommandCenter = [MPRemoteCommandCenter sharedCommandCenter];
  if (play) [[remoteCommandCenter playCommand] addTarget:self action:@selector(remotePlay)];

  // [[remoteCommandCenter skipForwardCommand] addTarget:self action:@selector(remoteSkipForward)];
  // [[remoteCommandCenter skipForwardCommand] addTarget:self action:@selector(remoteSkipForward)];
  // [[remoteCommandCenter togglePlayPauseCommand] addTarget:self action:@selector(remoteTogglePlayPause)];
  // [[remoteCommandCenter pauseCommand] addTarget:self action:@selector(remotePause)];
  // [[remoteCommandCenter stopCommand] addTarget:self action:@selector(remoteStop)];
}

- (void)remotePlay { NSLog("@play"); }

@end

namespace atom {

namespace api {

  ElectronMediaController* controller;

  void App::InitializeAsMediaPlayer(mate::Arguments* args) {
    if (media_controller_initialized_) {
      args->ThrowError("You can only initialize the app as a media player once");
      return;
    }
    mate::Dictionary opts;
    if (!args->GetNext(&opts)) {
      args->ThrowError("The first argument must be an options object");
      return;
    }
    if (!controller) {
      controller = [[ElectronMediaController alloc] init];
      [controller setApp:this];
      [controller setOptions:opts];
    }
    media_controller_initialized_ = true;
  }

  void App::SetNowPlaying(mate::Arguments* args) {
    if (!media_controller_initialized_) {
      args->ThrowError("You must initialize the app as a media player before settings now playing");
      return;
    }
    mate::Dictionary opts;
    if (!args->GetNext(&opts)) {
      args->ThrowError("The first argument must be an options object");
      return;
    }
  }

}  // namespace api

}  // namespace atom
