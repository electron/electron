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

#include "MediaPlayer/MPNowPlayingInfoCenter.h"
#include "MediaPlayer/MPRemoteCommandCenter.h"
#include "MediaPlayer/MPRemoteCommand.h"
#include "MediaPlayer/MPMediaItem.h"
#include "MediaPlayer/MPRemoteCommandEvent.h"

#include "atom/common/node_includes.h"

@interface ElectronMediaController : NSObject {
}
- (void)setApp:(atom::api::App*)atomApp;
- (void)setNowPlaying:(mate::Dictionary)dict withArgs:(mate::Arguments*)args;
- (void)initialize;
@end

@implementation ElectronMediaController
  atom::api::App* _app;

- (void)setApp:(atom::api::App*)atomApp {
  _app = atomApp;
}

- (void)initialize {
  MPRemoteCommandCenter *remoteCommandCenter = [MPRemoteCommandCenter sharedCommandCenter];
  [remoteCommandCenter playCommand].enabled = true;
  [remoteCommandCenter pauseCommand].enabled = true;
  [remoteCommandCenter togglePlayPauseCommand].enabled = true;
  [remoteCommandCenter changePlaybackPositionCommand].enabled = true;

  [[remoteCommandCenter playCommand] addTarget:self action:@selector(remotePlay)];
  [[remoteCommandCenter pauseCommand] addTarget:self action:@selector(remotePause)];
  [[remoteCommandCenter togglePlayPauseCommand] addTarget:self action:@selector(remoteTogglePlayPause)];
  [[remoteCommandCenter changePlaybackPositionCommand] addTarget:self action:@selector(remoteChangePlaybackPosition:)];
}

- (void)remotePlay { _app->Emit("playback-play"); }
- (void)remotePause { _app->Emit("playback-pause"); }
- (void)remoteTogglePlayPause { _app->Emit("playback-play-pause"); }

- (void)remoteChangePlaybackPosition:(MPChangePlaybackPositionCommandEvent*)event {
  _app->Emit("playback-change-position", event.positionTime);
}

- (MPRemoteCommandHandlerStatus)move:(MPChangePlaybackPositionCommandEvent*)event {
  return MPRemoteCommandHandlerStatusSuccess;
}

- (void)setNowPlaying:(mate::Dictionary)dict withArgs:(mate::Arguments*)args {
  NSMutableDictionary *songInfo = [[NSMutableDictionary alloc] init];

  int songID;
  int currentTime;
  int duration;
  std::string title;
  std::string artist;
  std::string album;
  std::string state;

  if (!dict.Get("currentTime", &currentTime) || !dict.Get("duration", &duration) ||
      !dict.Get("title", &title) || !dict.Get("artist", &artist) || !dict.Get("album", &album) ||
      !dict.Get("id", &songID) || !dict.Get("state", &state)) {
    args->ThrowError("Missing required property on the Nowplaying object");
    return;
  }

  [songInfo setObject:[NSString stringWithUTF8String:title.c_str()] forKey:MPMediaItemPropertyTitle];
  [songInfo setObject:[NSString stringWithUTF8String:artist.c_str()] forKey:MPMediaItemPropertyArtist];
  [songInfo setObject:[NSString stringWithUTF8String:album.c_str()] forKey:MPMediaItemPropertyAlbumTitle];
  [songInfo setObject:[NSNumber numberWithFloat:currentTime] forKey:MPNowPlayingInfoPropertyElapsedPlaybackTime];
  [songInfo setObject:[NSNumber numberWithFloat:duration] forKey:MPMediaItemPropertyPlaybackDuration];
  [songInfo setObject:[NSNumber numberWithFloat:songID] forKey:MPMediaItemPropertyPersistentID];

  if (state == "playing") {
    [MPNowPlayingInfoCenter defaultCenter].playbackState = MPNowPlayingPlaybackStatePlaying;
  } else if (state == "paused") {
    [MPNowPlayingInfoCenter defaultCenter].playbackState = MPNowPlayingPlaybackStatePaused;
  } else {
    [MPNowPlayingInfoCenter defaultCenter].playbackState = MPNowPlayingPlaybackStateStopped;
  }
  [[MPNowPlayingInfoCenter defaultCenter] setNowPlayingInfo:songInfo];
}

@end

namespace atom {

namespace api {

  ElectronMediaController* controller;

  void App::InitializeAsMediaPlayer(mate::Arguments* args) {
    if (media_controller_initialized_) {
      args->ThrowError("You can only initialize the app as a media player once");
      return;
    }

    if (!controller) {
      controller = [[ElectronMediaController alloc] init];
      [controller setApp:this];
      [controller initialize];
    }
    media_controller_initialized_ = true;
  }

  void App::SetNowPlaying(mate::Arguments* args) {
    if (!media_controller_initialized_) {
      args->ThrowError("You must initialize the app as a media player before setting now playing");
      return;
    }

    mate::Dictionary opts;
    if (!args->GetNext(&opts)) {
      args->ThrowError("The first argument must be an options object");
      return;
    }

    [controller setNowPlaying:opts withArgs:args];
  }

}  // namespace api

}  // namespace atom
