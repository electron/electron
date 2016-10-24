// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_API_ATOM_API_CLIPBOARD_H_
#define ATOM_COMMON_API_ATOM_API_CLIPBOARD_H_

#include <string>
#include <vector>

#include "native_mate/arguments.h"
#include "native_mate/dictionary.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/gfx/image/image.h"

namespace atom {

namespace api {

class Clipboard {
 public:
  static ui::ClipboardType GetClipboardType(mate::Arguments* args);
  static std::vector<base::string16> AvailableFormats(mate::Arguments* args);
  static bool Has(const std::string& format_string, mate::Arguments* args);
  static void Clear(mate::Arguments* args);

  static std::string Read(const std::string& format_string,
                          mate::Arguments* args);
  static void Write(const mate::Dictionary& data, mate::Arguments* args);

  static base::string16 ReadText(mate::Arguments* args);
  static void WriteText(const base::string16& text, mate::Arguments* args);

  static base::string16 ReadRtf(mate::Arguments* args);
  static void WriteRtf(const std::string& text, mate::Arguments* args);

  static base::string16 ReadHtml(mate::Arguments* args);
  static void WriteHtml(const base::string16& html, mate::Arguments* args);

  static v8::Local<v8::Value> ReadBookmark(mate::Arguments* args);
  static void WriteBookmark(const base::string16& title,
                            const std::string& url,
                            mate::Arguments* args);

  static gfx::Image ReadImage(mate::Arguments* args);
  static void WriteImage(const gfx::Image& image, mate::Arguments* args);

  static base::string16 ReadFindText();
  static void WriteFindText(const base::string16& text);

 private:
  DISALLOW_COPY_AND_ASSIGN(Clipboard);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_COMMON_API_ATOM_API_CLIPBOARD_H_
