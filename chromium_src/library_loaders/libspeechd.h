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

  decltype(&::spd_open) spd_open;
  decltype(&::spd_say) spd_say;
  decltype(&::spd_stop) spd_stop;
  decltype(&::spd_close) spd_close;
  decltype(&::spd_pause) spd_pause;
  decltype(&::spd_resume) spd_resume;
  decltype(&::spd_set_notification_on) spd_set_notification_on;
  decltype(&::spd_set_voice_rate) spd_set_voice_rate;
  decltype(&::spd_set_voice_pitch) spd_set_voice_pitch;
  decltype(&::spd_list_synthesis_voices) spd_list_synthesis_voices;
  decltype(&::spd_set_synthesis_voice) spd_set_synthesis_voice;
  decltype(&::spd_list_modules) spd_list_modules;
  decltype(&::spd_set_output_module) spd_set_output_module;
  decltype(&::spd_set_language) spd_set_language;


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
