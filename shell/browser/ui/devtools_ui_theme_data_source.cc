// Copyright (c) 2024 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/devtools_ui_theme_data_source.h"

#include <string>
#include <string_view>
#include <utility>

#include "base/memory/ref_counted_memory.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/color/chrome_color_provider_utils.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/browser/devtools_frontend_host.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "net/base/url_util.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_utils.h"

namespace electron {

namespace {
GURL GetThemeUrl(const std::string& path) {
  return GURL(std::string(content::kChromeDevToolsScheme) +
              url::kStandardSchemeSeparator +
              std::string(chrome::kChromeUIThemeHost) + "/" + path);
}

scoped_refptr<base::RefCountedMemory> CreateNotFoundResponse() {
  const char kHttpNotFound[] = "HTTP/1.1 404 Not Found\n\n";
  return base::MakeRefCounted<base::RefCountedStaticMemory>(
      base::byte_span_from_cstring(kHttpNotFound));
}

std::string GetMimeTypeForUrl(const GURL& url) {
  std::string filename = url.ExtractFileName();
  if (base::EndsWith(filename, ".html", base::CompareCase::INSENSITIVE_ASCII)) {
    return "text/html";
  } else if (base::EndsWith(filename, ".css",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "text/css";
  } else if (base::EndsWith(filename, ".js",
                            base::CompareCase::INSENSITIVE_ASCII) ||
             base::EndsWith(filename, ".mjs",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "application/javascript";
  } else if (base::EndsWith(filename, ".png",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/png";
  } else if (base::EndsWith(filename, ".map",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "application/json";
  } else if (base::EndsWith(filename, ".ts",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "application/x-typescript";
  } else if (base::EndsWith(filename, ".gif",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/gif";
  } else if (base::EndsWith(filename, ".svg",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "image/svg+xml";
  } else if (base::EndsWith(filename, ".manifest",
                            base::CompareCase::INSENSITIVE_ASCII)) {
    return "text/cache-manifest";
  }
  return "text/html";
}
}  // namespace

std::string ThemeDataSource::GetSource() {
  // kChromeUIThemeHost
  return chrome::kChromeUIThemeHost;
}

void ThemeDataSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    GotDataCallback callback) {
  // TODO(crbug.com/40050262): Simplify usages of |path| since |url| is
  // available.
  const std::string path = content::URLDataSource::URLToRequestPath(url);
  // Default scale factor if not specified.
  float scale = 1.0f;
  // All frames by default if not specified.
  int frame = -1;
  std::string parsed_path;
  webui::ParsePathAndImageSpec(GetThemeUrl(path), &parsed_path, &scale, &frame);

  // kColorsCssPath should stay consistent with COLORS_CSS_SELECTOR in
  // colors_css_updater.js.
  constexpr std::string_view kColorsCssPath = "colors.css";
  if (parsed_path == kColorsCssPath) {
    SendColorsCss(url, wc_getter, std::move(callback));
    return;
  }

  std::move(callback).Run(CreateNotFoundResponse());
}

std::string ThemeDataSource::GetMimeType(const GURL& url) {
  return GetMimeTypeForUrl(url);
}

void ThemeDataSource::SendColorsCss(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    content::URLDataSource::GotDataCallback callback) {
  const ui::ColorProvider& color_provider = wc_getter.Run()->GetColorProvider();

  std::string sets_param;
  std::vector<std::string_view> color_id_sets;
  bool generate_rgb_vars = false;
  std::string generate_rgb_vars_query_value;
  if (net::GetValueForKeyInQuery(url, "generate_rgb_vars",
                                 &generate_rgb_vars_query_value)) {
    generate_rgb_vars =
        base::ToLowerASCII(generate_rgb_vars_query_value) == "true";
  }
  bool shadow_host = false;
  std::string shadow_host_query_value;
  if (net::GetValueForKeyInQuery(url, "shadow_host",
                                 &shadow_host_query_value)) {
    shadow_host = base::ToLowerASCII(shadow_host_query_value) == "true";
  }
  if (!net::GetValueForKeyInQuery(url, "sets", &sets_param)) {
    LOG(ERROR)
        << "colors.css requires a 'sets' query parameter to specify the "
           "color "
           "id sets returned e.g chrome://theme/colors.css?sets=ui,chrome";
    std::move(callback).Run(nullptr);
    return;
  }
  color_id_sets = base::SplitStringPiece(sets_param, ",", base::TRIM_WHITESPACE,
                                         base::SPLIT_WANT_ALL);

  using ColorIdCSSCallback = base::RepeatingCallback<std::string(ui::ColorId)>;
  auto generate_color_mapping =
      [&color_id_sets, &color_provider, &generate_rgb_vars](
          std::string set_name, ui::ColorId start, ui::ColorId end,
          ColorIdCSSCallback color_css_name) {
        // Only return these mappings if specified in the query parameter.
        auto it = std::ranges::find(color_id_sets, set_name);
        if (it == color_id_sets.end()) {
          return std::string();
        }
        color_id_sets.erase(it);
        std::string css_string;
        for (ui::ColorId id = start; id < end; ++id) {
          const SkColor color = color_provider.GetColor(id);
          std::string css_id_to_color_mapping =
              absl::StrFormat("%s:%s;", color_css_name.Run(id),
                              ui::ConvertSkColorToCSSColor(color));
          base::StrAppend(&css_string, {css_id_to_color_mapping});
          if (generate_rgb_vars) {
            // Also generate a r,g,b string for each color so apps can construct
            // colors with their own opacities in css.
            const std::string css_rgb_color_str =
                color_utils::SkColorToRgbString(color);
            const std::string css_id_to_rgb_color_mapping = absl::StrFormat(
                "%s-rgb:%s;", color_css_name.Run(id), css_rgb_color_str);
            base::StrAppend(&css_string, {css_id_to_rgb_color_mapping});
          }
        }
        return css_string;
      };

  // Convenience lambda for wrapping
  // |ConvertColorProviderColorIdToCSSColorId|.
  auto generate_color_provider_mapping = [&generate_color_mapping](
                                             std::string set_name,
                                             ui::ColorId start, ui::ColorId end,
                                             std::string (*color_id_name)(
                                                 ui::ColorId)) {
    auto color_id_to_css_name = base::BindRepeating(
        [](std::string (*color_id_name)(ui::ColorId), ui::ColorId id) {
          return ui::ConvertColorProviderColorIdToCSSColorId(color_id_name(id));
        },
        color_id_name);
    return generate_color_mapping(set_name, start, end, color_id_to_css_name);
  };

  std::string css_selector;
  if (shadow_host) {
    css_selector = ":host";
  } else {
    // This selector requires more specificity than other existing CSS
    // selectors that define variables. We increase the specificity by adding
    // a pseudoselector.
    css_selector = "html:not(#z)";
  }

  std::string css_string = base::StrCat(
      {css_selector, "{", "--user-color-source: baseline-default;",
       generate_color_provider_mapping("ui", ui::kUiColorsStart,
                                       ui::kUiColorsEnd, ui::ColorIdName),
       generate_color_provider_mapping("chrome", kChromeColorsStart,
                                       kChromeColorsEnd, &ChromeColorIdName),
       "}"});
  if (!color_id_sets.empty()) {
    LOG(ERROR) << "Unrecognized color set(s) specified for "
                  "chrome://theme/colors.css: "
               << base::JoinString(color_id_sets, ",");
    std::move(callback).Run(nullptr);
    return;
  }

  std::move(callback).Run(
      base::MakeRefCounted<base::RefCountedString>(std::move(css_string)));
}
}  // namespace electron
