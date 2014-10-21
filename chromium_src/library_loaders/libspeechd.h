// This is generated file. Do not modify directly.
// Path to the code generator: tools/generate_library_loader/generate_library_loader.py .

#ifndef LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H
#define LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H

#include "third_party/speech-dispatcher/libspeechd.h"
#define LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN


#include <string>

class LibSpeechdLoader {
 public:
  LibSpeechdLoader();
  ~LibSpeechdLoader();

  bool Load(const std::string& library_name)
      __attribute__((warn_unused_result));

  bool loaded() const { return loaded_; }

  typeof(&::spd_open) spd_open;
  typeof(&::spd_say) spd_say;
  typeof(&::spd_stop) spd_stop;
  typeof(&::spd_close) spd_close;
  typeof(&::spd_pause) spd_pause;
  typeof(&::spd_resume) spd_resume;
  typeof(&::spd_set_notification_on) spd_set_notification_on;
  typeof(&::spd_set_voice_rate) spd_set_voice_rate;
  typeof(&::spd_set_voice_pitch) spd_set_voice_pitch;
  typeof(&::spd_list_synthesis_voices) spd_list_synthesis_voices;
  typeof(&::spd_set_synthesis_voice) spd_set_synthesis_voice;
  typeof(&::spd_list_modules) spd_list_modules;
  typeof(&::spd_set_output_module) spd_set_output_module;


 private:
  void CleanUp(bool unload);

#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN)
  void* library_;
#endif

  bool loaded_;

  // Disallow copy constructor and assignment operator.
  LibSpeechdLoader(const LibSpeechdLoader&);
  void operator=(const LibSpeechdLoader&);
};

#endif  // LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H
