// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "shell/browser/ui/devtools_ui.h"

#include <memory>
#include <string>
#include <utility>

#include "base/memory/ref_counted_memory.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/timer/elapsed_timer.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/color/chrome_color_provider_utils.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/browser/devtools_frontend_host.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "net/base/url_util.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_utils.h"

namespace electron {

namespace {

std::string PathWithoutParams(const std::string& path) {
  return GURL(base::StrCat({content::kChromeDevToolsScheme,
                            url::kStandardSchemeSeparator,
                            chrome::kChromeUIDevToolsHost}))
      .Resolve(path)
      .path()
      .substr(1);
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

GURL GetThemeUrl(const std::string& path) {
  return GURL(std::string(content::kChromeUIScheme) + "://" +
              std::string(chrome::kChromeUIThemeHost) + "/" + path);
}

class BundledDataSource : public content::URLDataSource {
 public:
  BundledDataSource() = default;
  ~BundledDataSource() override = default;

  // disable copy
  BundledDataSource(const BundledDataSource&) = delete;
  BundledDataSource& operator=(const BundledDataSource&) = delete;

  // content::URLDataSource implementation.
  std::string GetSource() override { return chrome::kChromeUIDevToolsHost; }

  void StartDataRequest(const GURL& url,
                        const content::WebContents::Getter& wc_getter,
                        GotDataCallback callback) override {
    const std::string path = content::URLDataSource::URLToRequestPath(url);
    // Serve request from local bundle.
    std::string bundled_path_prefix(chrome::kChromeUIDevToolsBundledPath);
    bundled_path_prefix += "/";
    if (base::StartsWith(path, bundled_path_prefix,
                         base::CompareCase::INSENSITIVE_ASCII)) {
      StartBundledDataRequest(path.substr(bundled_path_prefix.length()),
                              std::move(callback));
      return;
    }

    // We do not handle remote and custom requests.
    std::move(callback).Run(CreateNotFoundResponse());
  }

  std::string GetMimeType(const GURL& url) override {
    return GetMimeTypeForUrl(url);
  }

  bool ShouldAddContentSecurityPolicy() override { return false; }

  bool ShouldDenyXFrameOptions() override { return false; }

  bool ShouldServeMimeTypeAsContentTypeHeader() override { return true; }

  void StartBundledDataRequest(const std::string& path,
                               GotDataCallback callback) {
    std::string filename = PathWithoutParams(path);
    scoped_refptr<base::RefCountedMemory> bytes =
        content::DevToolsFrontendHost::GetFrontendResourceBytes(filename);

    DLOG_IF(WARNING, !bytes)
        << "Unable to find dev tool resource: " << filename
        << ". If you compiled with debug_devtools=1, try running with "
           "--debug-devtools.";
    std::move(callback).Run(bytes);
  }
};

class ThemeDataSource : public content::URLDataSource {
 public:
  ThemeDataSource() = default;
  ~ThemeDataSource() override = default;

  ThemeDataSource(const ThemeDataSource&) = delete;
  ThemeDataSource& operator=(const ThemeDataSource&) = delete;

  // kChromeUIThemeHost
  std::string GetSource() override { return chrome::kChromeUIThemeHost; }

  void StartDataRequest(const GURL& url,
                        const content::WebContents::Getter& wc_getter,
                        GotDataCallback callback) override {
    // TODO(crbug.com/40050262): Simplify usages of |path| since |url| is
    // available.
    const std::string path = content::URLDataSource::URLToRequestPath(url);
    // Default scale factor if not specified.
    float scale = 1.0f;
    // All frames by default if not specified.
    int frame = -1;
    std::string parsed_path;
    webui::ParsePathAndImageSpec(GetThemeUrl(path), &parsed_path, &scale,
                                 &frame);

    // kColorsCssPath should stay consistent with COLORS_CSS_SELECTOR in
    // colors_css_updater.js.
    constexpr char kColorsCssPath[] = "colors.css";
    if (parsed_path == kColorsCssPath) {
      SendColorsCss(url, wc_getter, std::move(callback));
      return;
    }
    /** TODO(BILL)
    int resource_id = ResourcesUtil::GetThemeResourceId(parsed_path);

    // Limit the maximum scale we'll respond to.  Very large scale factors can
    // take significant time to serve or, at worst, crash the browser due to
    // OOM. We don't want to clamp to the max scale factor, though, for devices
    // that use 2x scale without 2x data packs, as well as omnibox requests for
    // larger (but still reasonable) scales (see below).
    const float max_scale = ui::GetScaleForResourceScaleFactor(
        ui::ResourceBundle::GetSharedInstance().GetMaxResourceScaleFactor());
    const float unreasonable_scale = max_scale * 32;
    // TODO(reveman): Add support frames beyond 0 (crbug.com/750064).
    if ((resource_id == -1) || (scale >= unreasonable_scale) || (frame > 0)) {
      // Either we have no data to send back, or the requested scale is
      // unreasonably large.  This shouldn't happen normally, as chrome://theme/
      // URLs are only used by WebUI pages and component extensions.  However,
      // the user can also enter these into the omnibox, so we need to fail
      // gracefully.
      std::move(callback).Run(nullptr);
    } else if ((GetMimeType(url) == "image/png") &&
               ((scale > max_scale) || (frame != -1))) {
      // This will extract and scale frame 0 of animated images.
      // TODO(reveman): Support scaling of animated images and avoid scaling and
      // re-encode when specific frame is specified (crbug.com/750064).
      DCHECK_LE(frame, 0);
      SendThemeImage(std::move(callback), resource_id, scale);
    } else {
      SendThemeBitmap(std::move(callback), resource_id, scale);
    }
    */
  }

  std::string GetMimeType(const GURL& url) override {
    const std::string_view file_path = url.path_piece();

    if (base::EndsWith(file_path, ".css",
                       base::CompareCase::INSENSITIVE_ASCII)) {
      return "text/css";
    }

    return "image/png";
  }

  void SendColorsCss(const GURL& url,
                     const content::WebContents::Getter& wc_getter,
                     content::URLDataSource::GotDataCallback callback) {
    base::ElapsedTimer timer;
    const ui::ColorProvider& color_provider =
        wc_getter.Run()->GetColorProvider();

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
    color_id_sets = base::SplitStringPiece(
        sets_param, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

    using ColorIdCSSCallback =
        base::RepeatingCallback<std::string(ui::ColorId)>;
    auto generate_color_mapping = [&color_id_sets, &color_provider,
                                   &generate_rgb_vars](
                                      std::string set_name, ui::ColorId start,
                                      ui::ColorId end,
                                      ColorIdCSSCallback color_css_name) {
      // Only return these mappings if specified in the query parameter.
      auto it = base::ranges::find(color_id_sets, set_name);
      if (it == color_id_sets.end()) {
        return std::string();
      }
      color_id_sets.erase(it);
      std::string css_string;
      for (ui::ColorId id = start; id < end; ++id) {
        const SkColor color = color_provider.GetColor(id);
        std::string css_id_to_color_mapping =
            base::StringPrintf("%s:%s;", color_css_name.Run(id).c_str(),
                               ui::ConvertSkColorToCSSColor(color).c_str());
        base::StrAppend(&css_string, {css_id_to_color_mapping});
        if (generate_rgb_vars) {
          // Also generate a r,g,b string for each color so apps can construct
          // colors with their own opacities in css.
          const std::string css_rgb_color_str =
              color_utils::SkColorToRgbString(color);
          const std::string css_id_to_rgb_color_mapping =
              base::StringPrintf("%s-rgb:%s;", color_css_name.Run(id).c_str(),
                                 css_rgb_color_str.c_str());
          base::StrAppend(&css_string, {css_id_to_rgb_color_mapping});
        }
      }
      return css_string;
    };

    // Convenience lambda for wrapping
    // |ConvertColorProviderColorIdToCSSColorId|.
    auto generate_color_provider_mapping =
        [&generate_color_mapping](std::string set_name, ui::ColorId start,
                                  ui::ColorId end,
                                  std::string (*color_id_name)(ui::ColorId)) {
          auto color_id_to_css_name = base::BindRepeating(
              [](std::string (*color_id_name)(ui::ColorId), ui::ColorId id) {
                return ui::ConvertColorProviderColorIdToCSSColorId(
                    color_id_name(id));
              },
              color_id_name);
          return generate_color_mapping(set_name, start, end,
                                        color_id_to_css_name);
        };

    std::string css_selector;
    if (shadow_host) {
      css_selector = ":host";
    } else {
      // This selector requires more specificity than other existing CSS
      // selectors that define variables. We increase the specifity by adding
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

    // Measures the time it takes to generate the colors.css and queue it for
    // the renderer.
    UmaHistogramTimes("WebUI.ColorsStylesheetServingDuration", timer.Elapsed());
  }
};
}  // namespace

DevToolsUI::DevToolsUI(content::BrowserContext* browser_context,
                       content::WebUI* web_ui)
    : WebUIController(web_ui) {
  web_ui->SetBindings(0);
  content::URLDataSource::Add(browser_context,
                              std::make_unique<BundledDataSource>());
  content::URLDataSource::Add(browser_context,
                              std::make_unique<ThemeDataSource>());
}

}  // namespace electron
