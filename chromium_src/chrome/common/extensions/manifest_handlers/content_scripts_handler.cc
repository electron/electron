// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/manifest_handlers/content_scripts_handler.h"

#include <stddef.h>

#include "base/files/file_util.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/common/url_constants.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_resource.h"
#include "extensions/common/host_id.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handlers/permissions_parser.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/common/url_pattern.h"
#include "extensions/common/url_pattern_set.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace extensions {

namespace keys = extensions::manifest_keys;
namespace values = manifest_values;
namespace errors = manifest_errors;

namespace {

// Helper method that loads either the include_globs or exclude_globs list
// from an entry in the content_script lists of the manifest.
bool LoadGlobsHelper(const base::DictionaryValue* content_script,
                     int content_script_index,
                     const char* globs_property_name,
                     base::string16* error,
                     void(UserScript::*add_method)(const std::string& glob),
                     UserScript* instance) {
  if (!content_script->HasKey(globs_property_name))
    return true;  // they are optional

  const base::ListValue* list = NULL;
  if (!content_script->GetList(globs_property_name, &list)) {
    *error = ErrorUtils::FormatErrorMessageUTF16(
        errors::kInvalidGlobList,
        base::IntToString(content_script_index),
        globs_property_name);
    return false;
  }

  for (size_t i = 0; i < list->GetSize(); ++i) {
    std::string glob;
    if (!list->GetString(i, &glob)) {
      *error = ErrorUtils::FormatErrorMessageUTF16(
          errors::kInvalidGlob, base::IntToString(content_script_index),
          globs_property_name, base::SizeTToString(i));
      return false;
    }

    (instance->*add_method)(glob);
  }

  return true;
}

// Helper method that loads a UserScript object from a dictionary in the
// content_script list of the manifest.
bool LoadUserScriptFromDictionary(const base::DictionaryValue* content_script,
                                  int definition_index,
                                  Extension* extension,
                                  base::string16* error,
                                  UserScript* result) {
  // run_at
  if (content_script->HasKey(keys::kRunAt)) {
    std::string run_location;
    if (!content_script->GetString(keys::kRunAt, &run_location)) {
      *error = ErrorUtils::FormatErrorMessageUTF16(
          errors::kInvalidRunAt,
          base::IntToString(definition_index));
      return false;
    }

    if (run_location == values::kRunAtDocumentStart) {
      result->set_run_location(UserScript::DOCUMENT_START);
    } else if (run_location == values::kRunAtDocumentEnd) {
      result->set_run_location(UserScript::DOCUMENT_END);
    } else if (run_location == values::kRunAtDocumentIdle) {
      result->set_run_location(UserScript::DOCUMENT_IDLE);
    } else {
      *error = ErrorUtils::FormatErrorMessageUTF16(
          errors::kInvalidRunAt,
          base::IntToString(definition_index));
      return false;
    }
  }

  // all frames
  if (content_script->HasKey(keys::kAllFrames)) {
    bool all_frames = false;
    if (!content_script->GetBoolean(keys::kAllFrames, &all_frames)) {
      *error = ErrorUtils::FormatErrorMessageUTF16(
            errors::kInvalidAllFrames, base::IntToString(definition_index));
      return false;
    }
    result->set_match_all_frames(all_frames);
  }

  // match about blank
  if (content_script->HasKey(keys::kMatchAboutBlank)) {
    bool match_about_blank = false;
    if (!content_script->GetBoolean(keys::kMatchAboutBlank,
                                    &match_about_blank)) {
      *error = ErrorUtils::FormatErrorMessageUTF16(
          errors::kInvalidMatchAboutBlank, base::IntToString(definition_index));
      return false;
    }
    result->set_match_about_blank(match_about_blank);
  }

  // matches (required)
  const base::ListValue* matches = NULL;
  if (!content_script->GetList(keys::kMatches, &matches)) {
    *error = ErrorUtils::FormatErrorMessageUTF16(
        errors::kInvalidMatches,
        base::IntToString(definition_index));
    return false;
  }

  if (matches->GetSize() == 0) {
    *error = ErrorUtils::FormatErrorMessageUTF16(
        errors::kInvalidMatchCount,
        base::IntToString(definition_index));
    return false;
  }
  for (size_t j = 0; j < matches->GetSize(); ++j) {
    std::string match_str;
    if (!matches->GetString(j, &match_str)) {
      *error = ErrorUtils::FormatErrorMessageUTF16(
          errors::kInvalidMatch, base::IntToString(definition_index),
          base::SizeTToString(j), errors::kExpectString);
      return false;
    }

    URLPattern pattern(UserScript::ValidUserScriptSchemes(
        PermissionsData::CanExecuteScriptEverywhere(extension)));

    URLPattern::ParseResult parse_result = pattern.Parse(match_str);
    if (parse_result != URLPattern::PARSE_SUCCESS) {
      *error = ErrorUtils::FormatErrorMessageUTF16(
          errors::kInvalidMatch, base::IntToString(definition_index),
          base::SizeTToString(j),
          URLPattern::GetParseResultString(parse_result));
      return false;
    }

    // TODO(aboxhall): check for webstore
    if (!PermissionsData::CanExecuteScriptEverywhere(extension) &&
        pattern.scheme() != content::kChromeUIScheme) {
      // Exclude SCHEME_CHROMEUI unless it's been explicitly requested.
      // If the --extensions-on-chrome-urls flag has not been passed, requesting
      // a chrome:// url will cause a parse failure above, so there's no need to
      // check the flag here.
      pattern.SetValidSchemes(
          pattern.valid_schemes() & ~URLPattern::SCHEME_CHROMEUI);
    }

    if (pattern.MatchesScheme(url::kFileScheme) &&
        !PermissionsData::CanExecuteScriptEverywhere(extension)) {
      extension->set_wants_file_access(true);
      if (!(extension->creation_flags() & Extension::ALLOW_FILE_ACCESS)) {
        pattern.SetValidSchemes(
            pattern.valid_schemes() & ~URLPattern::SCHEME_FILE);
      }
    }

    result->add_url_pattern(pattern);
  }

  // exclude_matches
  if (content_script->HasKey(keys::kExcludeMatches)) {  // optional
    const base::ListValue* exclude_matches = NULL;
    if (!content_script->GetList(keys::kExcludeMatches, &exclude_matches)) {
      *error = ErrorUtils::FormatErrorMessageUTF16(
          errors::kInvalidExcludeMatches,
          base::IntToString(definition_index));
      return false;
    }

    for (size_t j = 0; j < exclude_matches->GetSize(); ++j) {
      std::string match_str;
      if (!exclude_matches->GetString(j, &match_str)) {
        *error = ErrorUtils::FormatErrorMessageUTF16(
            errors::kInvalidExcludeMatch, base::IntToString(definition_index),
            base::SizeTToString(j), errors::kExpectString);
        return false;
      }

      int valid_schemes = UserScript::ValidUserScriptSchemes(
          PermissionsData::CanExecuteScriptEverywhere(extension));
      URLPattern pattern(valid_schemes);

      URLPattern::ParseResult parse_result = pattern.Parse(match_str);
      if (parse_result != URLPattern::PARSE_SUCCESS) {
        *error = ErrorUtils::FormatErrorMessageUTF16(
            errors::kInvalidExcludeMatch, base::IntToString(definition_index),
            base::SizeTToString(j),
            URLPattern::GetParseResultString(parse_result));
        return false;
      }

      result->add_exclude_url_pattern(pattern);
    }
  }

  // include/exclude globs (mostly for Greasemonkey compatibility)
  if (!LoadGlobsHelper(content_script, definition_index, keys::kIncludeGlobs,
                       error, &UserScript::add_glob, result)) {
      return false;
  }

  if (!LoadGlobsHelper(content_script, definition_index, keys::kExcludeGlobs,
                       error, &UserScript::add_exclude_glob, result)) {
      return false;
  }

  // js and css keys
  const base::ListValue* js = NULL;
  if (content_script->HasKey(keys::kJs) &&
      !content_script->GetList(keys::kJs, &js)) {
    *error = ErrorUtils::FormatErrorMessageUTF16(
        errors::kInvalidJsList,
        base::IntToString(definition_index));
    return false;
  }

  const base::ListValue* css = NULL;
  if (content_script->HasKey(keys::kCss) &&
      !content_script->GetList(keys::kCss, &css)) {
    *error = ErrorUtils::
        FormatErrorMessageUTF16(errors::kInvalidCssList,
        base::IntToString(definition_index));
    return false;
  }

  // The manifest needs to have at least one js or css user script definition.
  if (((js ? js->GetSize() : 0) + (css ? css->GetSize() : 0)) == 0) {
    *error = ErrorUtils::FormatErrorMessageUTF16(
        errors::kMissingFile,
        base::IntToString(definition_index));
    return false;
  }

  if (js) {
    for (size_t script_index = 0; script_index < js->GetSize();
         ++script_index) {
      const base::Value* value;
      std::string relative;
      if (!js->Get(script_index, &value) || !value->GetAsString(&relative)) {
        *error = ErrorUtils::FormatErrorMessageUTF16(
            errors::kInvalidJs, base::IntToString(definition_index),
            base::SizeTToString(script_index));
        return false;
      }
      GURL url = extension->GetResourceURL(relative);
      ExtensionResource resource = extension->GetResource(relative);
      result->js_scripts().push_back(UserScript::File(
          resource.extension_root(), resource.relative_path(), url));
    }
  }

  if (css) {
    for (size_t script_index = 0; script_index < css->GetSize();
         ++script_index) {
      const base::Value* value;
      std::string relative;
      if (!css->Get(script_index, &value) || !value->GetAsString(&relative)) {
        *error = ErrorUtils::FormatErrorMessageUTF16(
            errors::kInvalidCss, base::IntToString(definition_index),
            base::SizeTToString(script_index));
        return false;
      }
      GURL url = extension->GetResourceURL(relative);
      ExtensionResource resource = extension->GetResource(relative);
      result->css_scripts().push_back(UserScript::File(
          resource.extension_root(), resource.relative_path(), url));
    }
  }

  return true;
}

// Returns false and sets the error if script file can't be loaded,
// or if it's not UTF-8 encoded.
static bool IsScriptValid(const base::FilePath& path,
                          const base::FilePath& relative_path,
                          int message_id,
                          std::string* error) {
  std::string content;
  if (!base::PathExists(path) ||
      !base::ReadFileToString(path, &content)) {
    *error = l10n_util::GetStringFUTF8(
        message_id,
        relative_path.LossyDisplayName());
    return false;
  }

  if (!base::IsStringUTF8(content)) {
    *error = l10n_util::GetStringFUTF8(
        IDS_EXTENSION_BAD_FILE_ENCODING,
        relative_path.LossyDisplayName());
    return false;
  }

  return true;
}

struct EmptyUserScriptList {
  UserScriptList user_script_list;
};

static base::LazyInstance<EmptyUserScriptList> g_empty_script_list =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

ContentScriptsInfo::ContentScriptsInfo() {
}

ContentScriptsInfo::~ContentScriptsInfo() {
}

// static
const UserScriptList& ContentScriptsInfo::GetContentScripts(
    const Extension* extension) {
  ContentScriptsInfo* info = static_cast<ContentScriptsInfo*>(
      extension->GetManifestData(keys::kContentScripts));
  return info ? info->content_scripts
              : g_empty_script_list.Get().user_script_list;
}

// static
bool ContentScriptsInfo::ExtensionHasScriptAtURL(const Extension* extension,
                                                 const GURL& url) {
  const UserScriptList& content_scripts = GetContentScripts(extension);
  for (UserScriptList::const_iterator iter = content_scripts.begin();
      iter != content_scripts.end(); ++iter) {
    if (iter->MatchesURL(url))
      return true;
  }
  return false;
}

// static
URLPatternSet ContentScriptsInfo::GetScriptableHosts(
    const Extension* extension) {
  const UserScriptList& content_scripts = GetContentScripts(extension);
  URLPatternSet scriptable_hosts;
  for (UserScriptList::const_iterator content_script =
           content_scripts.begin();
       content_script != content_scripts.end();
       ++content_script) {
    URLPatternSet::const_iterator pattern =
        content_script->url_patterns().begin();
    for (; pattern != content_script->url_patterns().end(); ++pattern)
      scriptable_hosts.AddPattern(*pattern);
  }
  return scriptable_hosts;
}

ContentScriptsHandler::ContentScriptsHandler() {
}

ContentScriptsHandler::~ContentScriptsHandler() {
}

const std::vector<std::string> ContentScriptsHandler::Keys() const {
  static const char* const keys[] = {
    keys::kContentScripts
  };
  return std::vector<std::string>(keys, keys + arraysize(keys));
}

bool ContentScriptsHandler::Parse(Extension* extension, base::string16* error) {
  std::unique_ptr<ContentScriptsInfo> content_scripts_info(new ContentScriptsInfo);
  const base::ListValue* scripts_list = NULL;
  if (!extension->manifest()->GetList(keys::kContentScripts, &scripts_list)) {
    *error = base::ASCIIToUTF16(errors::kInvalidContentScriptsList);
    return false;
  }

  for (size_t i = 0; i < scripts_list->GetSize(); ++i) {
    const base::DictionaryValue* script_dict = NULL;
    if (!scripts_list->GetDictionary(i, &script_dict)) {
      *error = ErrorUtils::FormatErrorMessageUTF16(
          errors::kInvalidContentScript, base::SizeTToString(i));
      return false;
    }

    UserScript user_script;
    if (!LoadUserScriptFromDictionary(script_dict,
                                      i,
                                      extension,
                                      error,
                                      &user_script)) {
      return false;  // Failed to parse script context definition.
    }

    user_script.set_host_id(HostID(HostID::EXTENSIONS, extension->id()));
    if (extension->converted_from_user_script()) {
      user_script.set_emulate_greasemonkey(true);
      // Greasemonkey matches all frames.
      user_script.set_match_all_frames(true);
    }
    user_script.set_id(UserScript::GenerateUserScriptID());
    content_scripts_info->content_scripts.push_back(user_script);
  }
  extension->SetManifestData(keys::kContentScripts,
                             content_scripts_info.release());
  PermissionsParser::SetScriptableHosts(
      extension, ContentScriptsInfo::GetScriptableHosts(extension));
  return true;
}

bool ContentScriptsHandler::Validate(
    const Extension* extension,
    std::string* error,
    std::vector<InstallWarning>* warnings) const {
  // Validate that claimed script resources actually exist,
  // and are UTF-8 encoded.
  ExtensionResource::SymlinkPolicy symlink_policy;
  if ((extension->creation_flags() &
       Extension::FOLLOW_SYMLINKS_ANYWHERE) != 0) {
    symlink_policy = ExtensionResource::FOLLOW_SYMLINKS_ANYWHERE;
  } else {
    symlink_policy = ExtensionResource::SYMLINKS_MUST_RESOLVE_WITHIN_ROOT;
  }

  const UserScriptList& content_scripts =
      ContentScriptsInfo::GetContentScripts(extension);
  for (size_t i = 0; i < content_scripts.size(); ++i) {
    const UserScript& script = content_scripts[i];

    for (size_t j = 0; j < script.js_scripts().size(); j++) {
      const UserScript::File& js_script = script.js_scripts()[j];
      const base::FilePath& path = ExtensionResource::GetFilePath(
          js_script.extension_root(), js_script.relative_path(),
          symlink_policy);
      if (!IsScriptValid(path, js_script.relative_path(),
                         IDS_EXTENSION_LOAD_JAVASCRIPT_FAILED, error))
        return false;
    }

    for (size_t j = 0; j < script.css_scripts().size(); j++) {
      const UserScript::File& css_script = script.css_scripts()[j];
      const base::FilePath& path = ExtensionResource::GetFilePath(
          css_script.extension_root(), css_script.relative_path(),
          symlink_policy);
      if (!IsScriptValid(path, css_script.relative_path(),
                         IDS_EXTENSION_LOAD_CSS_FAILED, error))
        return false;
    }
  }

  return true;
}

}  // namespace extensions
