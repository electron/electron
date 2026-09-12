// Copyright (c) 2026 Electron contributors.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/printing/printer_capabilities.h"

#include <optional>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "printing/buildflags/buildflags.h"
#include "shell/common/thread_restrictions.h"

#if BUILDFLAG(IS_LINUX) && BUILDFLAG(USE_CUPS)
#include <cups/ppd.h>

#include <array>
#include <memory>

#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/files/scoped_temp_file.h"
#include "printing/backend/cups_deleters.h"
#endif

#if BUILDFLAG(IS_WIN)
#include <windows.h>

#include <algorithm>
#include <array>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "printing/backend/printing_info_win.h"
#include "printing/backend/win_helper.h"
#endif

#if BUILDFLAG(USE_CUPS_IPP) && !BUILDFLAG(IS_LINUX)
#include "printing/backend/cups_connection.h"
#include "printing/backend/cups_printer.h"
#endif

namespace electron {

bool IsPrinterMediaIdValid(std::string_view id) {
  // IPP keyword characters also cover decimal Windows driver IDs. Restricting
  // these characters avoids option-string quoting ambiguities in print servers.
  return !id.empty() && id.size() <= 255 &&
         base::ContainsOnlyChars(id,
                                 "abcdefghijklmnopqrstuvwxyz"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._");
}

namespace {

#if BUILDFLAG(IS_LINUX) && BUILDFLAG(USE_CUPS)
// GTK submits PPD options through its CUPS backend. Use that driver's IDs, not
// IPP keywords: GTK 3 cannot encode media-col collections. Keep authentication,
// credential caching and job submission in GTK rather than bypassing it.
// Chromium's Linux CUPS backend also uses these deprecated PPD APIs.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
base::ListValue GetPpdMediaOptions(ppd_file_t* ppd, const char* name) {
  base::ListValue result;
  const ppd_option_t* option = ppdFindOption(ppd, name);
  if (!option || option->num_choices <= 0)
    return result;
  const ppd_choice_t* default_choice = ppdFindMarkedChoice(ppd, name);
  // SAFETY: libcups owns this array and reports its length in num_choices.
  const auto choices = UNSAFE_BUFFERS(
      base::span(option->choices, static_cast<size_t>(option->num_choices)));
  for (const ppd_choice_t& choice : choices) {
    if (!IsPrinterMediaIdValid(choice.choice))
      continue;
    result.Append(base::DictValue()
                      .Set("id", choice.choice)
                      .Set("displayName", choice.text)
                      .Set("isDefault", &choice == default_choice));
  }
  return result;
}

std::pair<std::string, base::DictValue> GetGtkPrinterCapabilities(
    const std::string& printer_name) {
  printing::ScopedDestination destination(
      cupsGetNamedDest(CUPS_HTTP_DEFAULT, printer_name.c_str(), nullptr));
  if (!destination)
    return {"Failed to open printer", base::DictValue()};

  base::ScopedTempFile temporary_file;
  if (!temporary_file.Create())
    return {"Failed to read printer capabilities", base::DictValue()};
  std::array<char, 1024> filename;
  if (base::strlcpy(filename, temporary_file.path().value()) >=
      filename.size()) {
    return {"Failed to read printer capabilities", base::DictValue()};
  }
  time_t modification_time = 0;
  if (cupsGetPPD3(CUPS_HTTP_DEFAULT, destination->name, &modification_time,
                  filename.data(), filename.size()) != HTTP_STATUS_OK) {
    return {"Failed to read printer capabilities", base::DictValue()};
  }
  std::unique_ptr<ppd_file_t, decltype(&ppdClose)> ppd(
      ppdOpenFile(filename.data()), &ppdClose);
  if (!ppd)
    return {"Failed to read printer capabilities", base::DictValue()};
  ppdLocalize(ppd.get());
  ppdMarkDefaults(ppd.get());
  cupsMarkOptions(ppd.get(), destination->num_options, destination->options);
  return {std::string(),
          base::DictValue()
              .Set("inputTrays", GetPpdMediaOptions(ppd.get(), "InputSlot"))
              .Set("mediaTypes", GetPpdMediaOptions(ppd.get(), "MediaType"))};
}
#pragma clang diagnostic pop
#endif

#if BUILDFLAG(USE_CUPS_IPP) && !BUILDFLAG(IS_LINUX)
base::ListValue GetCupsPrinterMediaOptions(const printing::CupsPrinter& printer,
                                           const char* option_name) {
  base::ListValue options;
  // These IDs are sent as members of media-col, not as legacy PPD options.
  if (!printer.CheckOptionSupported("media-col", option_name))
    return options;

  ipp_attribute_t* default_attribute =
      printer.GetDefaultOptionValue(option_name);
  if (!default_attribute) {
    ipp_attribute_t* media_col = printer.GetDefaultOptionValue("media-col");
    if (media_col && ippGetCount(media_col) > 0) {
      default_attribute = ippFindAttribute(ippGetCollection(media_col, 0),
                                           option_name, IPP_TAG_KEYWORD);
    }
  }
  const char* default_id = ippGetString(default_attribute, 0, nullptr);
  for (const auto id : printer.GetSupportedOptionValueStrings(option_name)) {
    if (!IsPrinterMediaIdValid(id))
      continue;
    const std::string value(id);
    const char* display_name =
        printer.GetLocalizedOptionValueName(option_name, value.c_str());
    options.Append(base::DictValue()
                       .Set("id", value)
                       .Set("displayName", display_name ? display_name : value)
                       .Set("isDefault", default_id && id == default_id));
  }
  return options;
}
#endif

#if BUILDFLAG(IS_WIN)
template <typename Id, size_t NameLength>
std::optional<base::ListValue> GetPrinterMediaOptions(
    const wchar_t* printer,
    const wchar_t* port,
    WORD ids_capability,
    WORD names_capability,
    const DEVMODE* dev_mode,
    std::optional<unsigned> default_id) {
  base::ListValue options;
  const int count =
      DeviceCapabilitiesW(printer, port, ids_capability, nullptr, dev_mode);
  const int name_count =
      DeviceCapabilitiesW(printer, port, names_capability, nullptr, dev_mode);
  // Drivers need not expose either capability.
  if (count <= 0 || name_count <= 0)
    return options;
  if (count != name_count || count > 65536)
    return std::nullopt;

  // Match Chromium's allocation strategy for DeviceCapabilities: leave extra
  // room for drivers whose count changes between the size and data queries.
  std::vector<Id> ids(count * 2);
  std::vector<std::array<wchar_t, NameLength>> names(count * 2);
  const int ids_read =
      DeviceCapabilitiesW(printer, port, ids_capability,
                          reinterpret_cast<wchar_t*>(ids.data()), dev_mode);
  const int names_read =
      DeviceCapabilitiesW(printer, port, names_capability,
                          reinterpret_cast<wchar_t*>(names.data()), dev_mode);
  if (ids_read != count || names_read != count)
    return std::nullopt;

  for (int i = 0; i < count; ++i) {
    if (ids[i] == 0)
      continue;
    const auto& name = names[i];
    const auto end = std::ranges::find(name, L'\0');
    options.Append(
        base::DictValue()
            .Set("id", base::NumberToString(ids[i]))
            .Set("displayName", base::WideToUTF8(std::wstring_view(
                                    name.data(), end - name.begin())))
            .Set("isDefault", default_id && *default_id == ids[i]));
  }
  return options;
}
#endif

}  // namespace

std::pair<std::string, base::DictValue> GetPrinterCapabilities(
    const std::string& printer_name) {
#if BUILDFLAG(IS_WIN)
  ScopedAllowBlockingForElectron allow_blocking;
  printing::ScopedPrinterHandle printer;
  if (!printer.OpenPrinterWithName(base::UTF8ToWide(printer_name).c_str()))
    return {"Failed to open printer", base::DictValue()};

  printing::PrinterInfo5 info;
  if (!info.Init(printer.Get()))
    return {"Failed to read printer information", base::DictValue()};
  auto dev_mode = printing::CreateDevMode(printer.Get(), nullptr);
  if (!dev_mode)
    return {"Failed to read printer defaults", base::DictValue()};

  std::optional<unsigned> default_tray;
  std::optional<unsigned> default_media;
  if (dev_mode->dmFields & DM_DEFAULTSOURCE)
    default_tray = static_cast<WORD>(dev_mode->dmDefaultSource);
  if (dev_mode->dmFields & DM_MEDIATYPE)
    default_media = dev_mode->dmMediaType;

  auto trays = GetPrinterMediaOptions<WORD, 24>(
      info.get()->pPrinterName, info.get()->pPortName, DC_BINS, DC_BINNAMES,
      dev_mode.get(), default_tray);
  auto media = GetPrinterMediaOptions<DWORD, 64>(
      info.get()->pPrinterName, info.get()->pPortName, DC_MEDIATYPES,
      DC_MEDIATYPENAMES, dev_mode.get(), default_media);
  if (!trays || !media)
    return {"Printer capabilities changed during the query", base::DictValue()};

  return {std::string(), base::DictValue()
                             .Set("inputTrays", std::move(*trays))
                             .Set("mediaTypes", std::move(*media))};
#elif BUILDFLAG(IS_LINUX) && BUILDFLAG(USE_CUPS)
  return GetGtkPrinterCapabilities(printer_name);
#elif BUILDFLAG(USE_CUPS_IPP)
  auto connection = printing::CupsConnection::Create();
  auto printer = connection->GetPrinter(printer_name);
  if (!printer)
    return {"Failed to open printer", base::DictValue()};
  if (!printer->EnsureDestInfo())
    return {"Failed to read printer capabilities", base::DictValue()};
  return {std::string(), base::DictValue()
                             .Set("inputTrays", GetCupsPrinterMediaOptions(
                                                    *printer, "media-source"))
                             .Set("mediaTypes", GetCupsPrinterMediaOptions(
                                                    *printer, "media-type"))};
#else
  return {"Printer capabilities require the CUPS IPP backend",
          base::DictValue()};
#endif
}

}  // namespace electron
