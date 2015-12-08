// This is generated file. Do not modify directly.
// Path to the code generator: tools/generate_library_loader/generate_library_loader.py .

#include "library_loaders/libspeechd.h"

#include <dlfcn.h>

// Put these sanity checks here so that they fire at most once
// (to avoid cluttering the build output).
#if !defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN) && !defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DT_NEEDED)
#error neither LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN nor LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DT_NEEDED defined
#endif
#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN) && defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DT_NEEDED)
#error both LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN and LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DT_NEEDED defined
#endif

LibSpeechdLoader::LibSpeechdLoader() : loaded_(false) {
}

LibSpeechdLoader::~LibSpeechdLoader() {
  CleanUp(loaded_);
}

bool LibSpeechdLoader::Load(const std::string& library_name) {
  if (loaded_)
    return false;

#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN)
  library_ = dlopen(library_name.c_str(), RTLD_LAZY);
  if (!library_)
    return false;
#endif


#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN)
  spd_open =
      reinterpret_cast<decltype(this->spd_open)>(
          dlsym(library_, "spd_open"));
#endif
#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DT_NEEDED)
  spd_open = &::spd_open;
#endif
  if (!spd_open) {
    CleanUp(true);
    return false;
  }

#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN)
  spd_say =
      reinterpret_cast<decltype(this->spd_say)>(
          dlsym(library_, "spd_say"));
#endif
#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DT_NEEDED)
  spd_say = &::spd_say;
#endif
  if (!spd_say) {
    CleanUp(true);
    return false;
  }

#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN)
  spd_stop =
      reinterpret_cast<decltype(this->spd_stop)>(
          dlsym(library_, "spd_stop"));
#endif
#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DT_NEEDED)
  spd_stop = &::spd_stop;
#endif
  if (!spd_stop) {
    CleanUp(true);
    return false;
  }

#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN)
  spd_close =
      reinterpret_cast<decltype(this->spd_close)>(
          dlsym(library_, "spd_close"));
#endif
#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DT_NEEDED)
  spd_close = &::spd_close;
#endif
  if (!spd_close) {
    CleanUp(true);
    return false;
  }

#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN)
  spd_pause =
      reinterpret_cast<decltype(this->spd_pause)>(
          dlsym(library_, "spd_pause"));
#endif
#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DT_NEEDED)
  spd_pause = &::spd_pause;
#endif
  if (!spd_pause) {
    CleanUp(true);
    return false;
  }

#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN)
  spd_resume =
      reinterpret_cast<decltype(this->spd_resume)>(
          dlsym(library_, "spd_resume"));
#endif
#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DT_NEEDED)
  spd_resume = &::spd_resume;
#endif
  if (!spd_resume) {
    CleanUp(true);
    return false;
  }

#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN)
  spd_set_notification_on =
      reinterpret_cast<decltype(this->spd_set_notification_on)>(
          dlsym(library_, "spd_set_notification_on"));
#endif
#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DT_NEEDED)
  spd_set_notification_on = &::spd_set_notification_on;
#endif
  if (!spd_set_notification_on) {
    CleanUp(true);
    return false;
  }

#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN)
  spd_set_voice_rate =
      reinterpret_cast<decltype(this->spd_set_voice_rate)>(
          dlsym(library_, "spd_set_voice_rate"));
#endif
#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DT_NEEDED)
  spd_set_voice_rate = &::spd_set_voice_rate;
#endif
  if (!spd_set_voice_rate) {
    CleanUp(true);
    return false;
  }

#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN)
  spd_set_voice_pitch =
      reinterpret_cast<decltype(this->spd_set_voice_pitch)>(
          dlsym(library_, "spd_set_voice_pitch"));
#endif
#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DT_NEEDED)
  spd_set_voice_pitch = &::spd_set_voice_pitch;
#endif
  if (!spd_set_voice_pitch) {
    CleanUp(true);
    return false;
  }

#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN)
  spd_list_synthesis_voices =
      reinterpret_cast<decltype(this->spd_list_synthesis_voices)>(
          dlsym(library_, "spd_list_synthesis_voices"));
#endif
#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DT_NEEDED)
  spd_list_synthesis_voices = &::spd_list_synthesis_voices;
#endif
  if (!spd_list_synthesis_voices) {
    CleanUp(true);
    return false;
  }

#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN)
  spd_set_synthesis_voice =
      reinterpret_cast<decltype(this->spd_set_synthesis_voice)>(
          dlsym(library_, "spd_set_synthesis_voice"));
#endif
#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DT_NEEDED)
  spd_set_synthesis_voice = &::spd_set_synthesis_voice;
#endif
  if (!spd_set_synthesis_voice) {
    CleanUp(true);
    return false;
  }

#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN)
  spd_list_modules =
      reinterpret_cast<decltype(this->spd_list_modules)>(
          dlsym(library_, "spd_list_modules"));
#endif
#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DT_NEEDED)
  spd_list_modules = &::spd_list_modules;
#endif
  if (!spd_list_modules) {
    CleanUp(true);
    return false;
  }

#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN)
  spd_set_output_module =
      reinterpret_cast<decltype(this->spd_set_output_module)>(
          dlsym(library_, "spd_set_output_module"));
#endif
#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DT_NEEDED)
  spd_set_output_module = &::spd_set_output_module;
#endif
  if (!spd_set_output_module) {
    CleanUp(true);
    return false;
  }

#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN)
  spd_set_language =
      reinterpret_cast<decltype(this->spd_set_language)>(
          dlsym(library_, "spd_set_language"));
#endif
#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DT_NEEDED)
  spd_set_language = &::spd_set_language;
#endif
  if (!spd_set_language) {
    CleanUp(true);
    return false;
  }


  loaded_ = true;
  return true;
}

void LibSpeechdLoader::CleanUp(bool unload) {
#if defined(LIBRARY_LOADER_OUT_RELEASE_GEN_LIBRARY_LOADERS_LIBSPEECHD_H_DLOPEN)
  if (unload) {
    dlclose(library_);
    library_ = NULL;
  }
#endif
  loaded_ = false;
  spd_open = NULL;
  spd_say = NULL;
  spd_stop = NULL;
  spd_close = NULL;
  spd_pause = NULL;
  spd_resume = NULL;
  spd_set_notification_on = NULL;
  spd_set_voice_rate = NULL;
  spd_set_voice_pitch = NULL;
  spd_list_synthesis_voices = NULL;
  spd_set_synthesis_voice = NULL;
  spd_list_modules = NULL;
  spd_set_output_module = NULL;
  spd_set_language = NULL;

}
