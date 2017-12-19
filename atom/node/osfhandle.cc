// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "osfhandle.h"

#if !defined(DEBUG)
#define U_I18N_IMPLEMENTATION
#define U_COMMON_IMPLEMENTATION
#define U_COMBINED_IMPLEMENTATION
#endif

#include "third_party/icu/source/common/unicode/ubidi.h"
#include "third_party/icu/source/common/unicode/uchar.h"
#include "third_party/icu/source/common/unicode/uidna.h"
#include "third_party/icu/source/common/unicode/unistr.h"
#include "third_party/icu/source/common/unicode/unorm.h"
#include "third_party/icu/source/common/unicode/urename.h"
#include "third_party/icu/source/common/unicode/ustring.h"
#include "third_party/icu/source/i18n/unicode/dtitvfmt.h"
#include "third_party/icu/source/i18n/unicode/measfmt.h"
#include "third_party/icu/source/i18n/unicode/translit.h"
#include "third_party/icu/source/i18n/unicode/ucsdet.h"
#include "third_party/icu/source/i18n/unicode/ulocdata.h"
#include "third_party/icu/source/i18n/unicode/uregex.h"
#include "third_party/icu/source/i18n/unicode/uspoof.h"
#include "third_party/icu/source/i18n/unicode/usearch.h"
#include "v8-profiler.h"
#include "v8-inspector.h"

namespace node {

void ReferenceSymbols() {
  // Following symbols are used by electron.exe but got stripped by compiler,
  // by using the symbols we can force compiler to keep the objects in node.dll,
  // thus electron.exe can link with the exported symbols.

  // v8_profiler symbols:
  v8::TracingCpuProfiler::Create(nullptr);
  // v8_inspector symbols:
  reinterpret_cast<v8_inspector::V8InspectorSession*>(nullptr)->
      canDispatchMethod(v8_inspector::StringView());
  reinterpret_cast<v8_inspector::V8InspectorClient*>(nullptr)->unmuteMetrics(0);
  // icu symbols:
  u_errorName(U_ZERO_ERROR);
  ubidi_setPara(nullptr, nullptr, 0, 0, nullptr, nullptr);
  ucsdet_getName(nullptr, nullptr);
  uidna_openUTS46(UIDNA_CHECK_BIDI, nullptr);
  ulocdata_close(nullptr);
  unorm_normalize(nullptr, 0, UNORM_NFC, 0, nullptr, 0, nullptr);
  uregex_matches(nullptr, 0, nullptr);
  uspoof_open(nullptr);
  usearch_setPattern(nullptr, nullptr, 0, nullptr);
  usearch_setPattern(nullptr, nullptr, 0, nullptr);
  UMeasureFormatWidth width = UMEASFMT_WIDTH_WIDE;
  UErrorCode status = U_ZERO_ERROR;
  icu::MeasureFormat format(icu::Locale::getRoot(), width, status);
  icu::DateInterval internal(0, 0);
  icu::DateIntervalFormat::createInstance(UnicodeString(),
                                          icu::Locale::getRoot(), status);
  reinterpret_cast<icu::Transliterator*>(nullptr)->clone();
  UParseError parse_error;
  icu::Transliterator::createFromRules(UnicodeString(), UnicodeString(),
                                       UTRANS_FORWARD, parse_error,
                                       status);
}

}  // namespace node
