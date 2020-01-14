// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_API_ATOM_API_CLIPBOARD_H_
#define SHELL_COMMON_API_ATOM_API_CLIPBOARD_H_

#include <string>
#include <vector>

#include "ui/base/clipboard/clipboard.h"
#include "ui/gfx/image/image.h"
#include "v8/include/v8.h"

namespace gin_helper {
class Arguments;
class Dictionary;
}  // namespace gin_helper

namespace electron {

namespace api {

class Clipboard {
 public:
  static ui::ClipboardBuffer GetClipboardBuffer(gin_helper::Arguments* args);
  static std::vector<base::string16> AvailableFormats(
      gin_helper::Arguments* args);
  static bool Has(const std::string& format_string,
                  gin_helper::Arguments* args);
  static void Clear(gin_helper::Arguments* args);

  static std::string Read(const std::string& format_string);
  static void Write(const gin_helper::Dictionary& data,
                    gin_helper::Arguments* args);

  static base::string16 ReadText(gin_helper::Arguments* args);
  static void WriteText(const base::string16& text,
                        gin_helper::Arguments* args);

  static base::string16 ReadRTF(gin_helper::Arguments* args);
  static void WriteRTF(const std::string& text, gin_helper::Arguments* args);

  static base::string16 ReadHTML(gin_helper::Arguments* args);
  static void WriteHTML(const base::string16& html,
                        gin_helper::Arguments* args);

  static v8::Local<v8::Value> ReadBookmark(gin_helper::Arguments* args);
  static void WriteBookmark(const base::string16& title,
                            const std::string& url,
                            gin_helper::Arguments* args);

  static gfx::Image ReadImage(gin_helper::Arguments* args);
  static void WriteImage(const gfx::Image& image, gin_helper::Arguments* args);

  static base::string16 ReadFindText();
  static void WriteFindText(const base::string16& text);

  static v8::Local<v8::Value> ReadBuffer(const std::string& format_string,
                                         gin_helper::Arguments* args);
  static void WriteBuffer(const std::string& format_string,
                          const v8::Local<v8::Value> buffer,
                          gin_helper::Arguments* args);

 private:
  DISALLOW_COPY_AND_ASSIGN(Clipboard);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_COMMON_API_ATOM_API_CLIPBOARD_H_
