// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_RENDER_PROCESS_PREFERENCES_H_
#define ATOM_BROWSER_API_ATOM_API_RENDER_PROCESS_PREFERENCES_H_

#include "atom/browser/render_process_preferences.h"
#include "native_mate/handle.h"
#include "native_mate/wrappable.h"

namespace atom {

namespace api {

class RenderProcessPreferences
    : public mate::Wrappable<RenderProcessPreferences> {
 public:
  static mate::Handle<RenderProcessPreferences>
      ForAllWebContents(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  int AddEntry(const base::DictionaryValue& entry);
  void RemoveEntry(int id);

 protected:
  RenderProcessPreferences(
      v8::Isolate* isolate,
      const atom::RenderProcessPreferences::Predicate& predicate);
  ~RenderProcessPreferences() override;

 private:
  atom::RenderProcessPreferences preferences_;

  DISALLOW_COPY_AND_ASSIGN(RenderProcessPreferences);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_RENDER_PROCESS_PREFERENCES_H_
