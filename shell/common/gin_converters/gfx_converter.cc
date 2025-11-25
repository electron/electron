// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/gfx_converter.h"

#include <string>

#include "gin/data_object_builder.h"
#include "shell/common/color_util.h"
#include "shell/common/gin_helper/dictionary.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/resize_utils.h"
#include "ui/gfx/geometry/size.h"

namespace gin {

v8::Local<v8::Value> Converter<gfx::Point>::ToV8(v8::Isolate* isolate,
                                                 const gfx::Point& val) {
  auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
  dict.Set("x", val.x());
  dict.Set("y", val.y());
  return dict.GetHandle();
}

bool Converter<gfx::Point>::FromV8(v8::Isolate* isolate,
                                   v8::Local<v8::Value> val,
                                   gfx::Point* out) {
  gin::Dictionary dict(isolate);
  if (!gin::ConvertFromV8(isolate, val, &dict))
    return false;
  double x, y;
  if (!dict.Get("x", &x) || !dict.Get("y", &y))
    return false;
  *out = gfx::Point(static_cast<int>(std::round(x)),
                    static_cast<int>(std::round(y)));
  return true;
}

v8::Local<v8::Value> Converter<gfx::PointF>::ToV8(v8::Isolate* isolate,
                                                  const gfx::PointF& val) {
  auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
  dict.Set("x", val.x());
  dict.Set("y", val.y());
  return dict.GetHandle();
}

bool Converter<gfx::PointF>::FromV8(v8::Isolate* isolate,
                                    v8::Local<v8::Value> val,
                                    gfx::PointF* out) {
  gin::Dictionary dict(isolate);
  if (!gin::ConvertFromV8(isolate, val, &dict))
    return false;
  float x, y;
  if (!dict.Get("x", &x) || !dict.Get("y", &y))
    return false;
  *out = gfx::PointF(x, y);
  return true;
}

v8::Local<v8::Value> Converter<gfx::Size>::ToV8(v8::Isolate* isolate,
                                                const gfx::Size& val) {
  auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
  dict.Set("width", val.width());
  dict.Set("height", val.height());
  return dict.GetHandle();
}

bool Converter<gfx::Size>::FromV8(v8::Isolate* isolate,
                                  v8::Local<v8::Value> val,
                                  gfx::Size* out) {
  gin::Dictionary dict(isolate);
  if (!gin::ConvertFromV8(isolate, val, &dict))
    return false;
  int width, height;
  if (!dict.Get("width", &width) || !dict.Get("height", &height))
    return false;
  *out = gfx::Size(width, height);
  return true;
}

v8::Local<v8::Value> Converter<gfx::Rect>::ToV8(v8::Isolate* isolate,
                                                const gfx::Rect& val) {
  auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
  dict.Set("x", val.x());
  dict.Set("y", val.y());
  dict.Set("width", val.width());
  dict.Set("height", val.height());
  return dict.GetHandle();
}

bool Converter<gfx::Rect>::FromV8(v8::Isolate* isolate,
                                  v8::Local<v8::Value> val,
                                  gfx::Rect* out) {
  gin::Dictionary dict(isolate);
  if (!gin::ConvertFromV8(isolate, val, &dict))
    return false;
  float x, y, width, height;
  if (!dict.Get("x", &x) || !dict.Get("y", &y) || !dict.Get("width", &width) ||
      !dict.Get("height", &height))
    return false;

  *out = ToRoundedRect(gfx::RectF(x, y, width, height));
  return true;
}

v8::Local<v8::Value> Converter<gfx::Insets>::ToV8(v8::Isolate* isolate,
                                                  const gfx::Insets& val) {
  return gin::DataObjectBuilder(isolate)
      .Set("top", val.top())
      .Set("left", val.left())
      .Set("bottom", val.bottom())
      .Set("right", val.right())
      .Build();
}

bool Converter<gfx::Insets>::FromV8(v8::Isolate* isolate,
                                    v8::Local<v8::Value> val,
                                    gfx::Insets* out) {
  gin::Dictionary dict(isolate);
  if (!gin::ConvertFromV8(isolate, val, &dict))
    return false;
  double top, left, right, bottom;
  if (!dict.Get("top", &top))
    return false;
  if (!dict.Get("left", &left))
    return false;
  if (!dict.Get("bottom", &bottom))
    return false;
  if (!dict.Get("right", &right))
    return false;
  *out = gfx::Insets::TLBR(top, left, bottom, right);
  return true;
}

template <>
struct Converter<display::Display::AccelerometerSupport> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const display::Display::AccelerometerSupport& val) {
    switch (val) {
      case display::Display::AccelerometerSupport::AVAILABLE:
        return StringToV8(isolate, "available");
      case display::Display::AccelerometerSupport::UNAVAILABLE:
        return StringToV8(isolate, "unavailable");
      default:
        return StringToV8(isolate, "unknown");
    }
  }
};

template <>
struct Converter<display::Display::TouchSupport> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const display::Display::TouchSupport& val) {
    switch (val) {
      case display::Display::TouchSupport::AVAILABLE:
        return StringToV8(isolate, "available");
      case display::Display::TouchSupport::UNAVAILABLE:
        return StringToV8(isolate, "unavailable");
      default:
        return StringToV8(isolate, "unknown");
    }
  }
};

v8::Local<v8::Value> Converter<display::Display>::ToV8(
    v8::Isolate* isolate,
    const display::Display& val) {
  auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
  dict.Set("accelerometerSupport", val.accelerometer_support());
  dict.Set("bounds", val.bounds());
  dict.Set("colorDepth", val.color_depth());
  dict.Set("colorSpace", val.GetColorSpaces()
                             .GetRasterAndCompositeColorSpace(
                                 gfx::ContentColorUsage::kWideColorGamut)
                             .ToString());
  dict.Set("depthPerComponent", val.depth_per_component());
  dict.Set("detected", val.detected());
  dict.Set("displayFrequency", val.display_frequency());
  dict.Set("id", val.id());
  dict.Set("internal", val.IsInternal());
  dict.Set("label", val.label());
  dict.Set("maximumCursorSize", val.maximum_cursor_size());
  dict.Set("monochrome", val.is_monochrome());
  dict.Set("nativeOrigin", val.native_origin());
  dict.Set("rotation", val.RotationAsDegree());
  dict.Set("scaleFactor", val.device_scale_factor());
  dict.Set("size", val.size());
  dict.Set("workArea", val.work_area());
  dict.Set("workAreaSize", val.work_area_size());
  dict.Set("touchSupport", val.touch_support());
  return dict.GetHandle();
}

v8::Local<v8::Value> Converter<gfx::ResizeEdge>::ToV8(
    v8::Isolate* isolate,
    const gfx::ResizeEdge val) {
  switch (val) {
    case gfx::ResizeEdge::kRight:
      return StringToV8(isolate, "right");
    case gfx::ResizeEdge::kBottom:
      return StringToV8(isolate, "bottom");
    case gfx::ResizeEdge::kTop:
      return StringToV8(isolate, "top");
    case gfx::ResizeEdge::kLeft:
      return StringToV8(isolate, "left");
    case gfx::ResizeEdge::kTopLeft:
      return StringToV8(isolate, "top-left");
    case gfx::ResizeEdge::kTopRight:
      return StringToV8(isolate, "top-right");
    case gfx::ResizeEdge::kBottomLeft:
      return StringToV8(isolate, "bottom-left");
    case gfx::ResizeEdge::kBottomRight:
      return StringToV8(isolate, "bottom-right");
    default:
      return StringToV8(isolate, "unknown");
  }
}

bool Converter<WrappedSkColor>::FromV8(v8::Isolate* isolate,
                                       v8::Local<v8::Value> val,
                                       WrappedSkColor* out) {
  std::string str;
  if (!gin::ConvertFromV8(isolate, val, &str))
    return false;
  *out = electron::ParseCSSColor(str).value_or(SK_ColorWHITE);
  return true;
}

v8::Local<v8::Value> Converter<gfx::ColorSpace>::ToV8(
    v8::Isolate* isolate,
    const gfx::ColorSpace& val) {
  auto dict = gin_helper::Dictionary::CreateEmpty(isolate);

  // Convert primaries to string
  std::string primaries;
  switch (val.GetPrimaryID()) {
    case gfx::ColorSpace::PrimaryID::BT709:
      primaries = "bt709";
      break;
    case gfx::ColorSpace::PrimaryID::BT470M:
      primaries = "bt470m";
      break;
    case gfx::ColorSpace::PrimaryID::BT470BG:
      primaries = "bt470bg";
      break;
    case gfx::ColorSpace::PrimaryID::SMPTE170M:
      primaries = "smpte170m";
      break;
    case gfx::ColorSpace::PrimaryID::SMPTE240M:
      primaries = "smpte240m";
      break;
    case gfx::ColorSpace::PrimaryID::FILM:
      primaries = "film";
      break;
    case gfx::ColorSpace::PrimaryID::BT2020:
      primaries = "bt2020";
      break;
    case gfx::ColorSpace::PrimaryID::SMPTEST428_1:
      primaries = "smptest428-1";
      break;
    case gfx::ColorSpace::PrimaryID::SMPTEST431_2:
      primaries = "smptest431-2";
      break;
    case gfx::ColorSpace::PrimaryID::P3:
      primaries = "p3";
      break;
    case gfx::ColorSpace::PrimaryID::XYZ_D50:
      primaries = "xyz-d50";
      break;
    case gfx::ColorSpace::PrimaryID::ADOBE_RGB:
      primaries = "adobe-rgb";
      break;
    case gfx::ColorSpace::PrimaryID::APPLE_GENERIC_RGB:
      primaries = "apple-generic-rgb";
      break;
    case gfx::ColorSpace::PrimaryID::WIDE_GAMUT_COLOR_SPIN:
      primaries = "wide-gamut-color-spin";
      break;
    case gfx::ColorSpace::PrimaryID::CUSTOM:
      primaries = "custom";
      break;
    case gfx::ColorSpace::PrimaryID::EBU_3213_E:
      primaries = "ebu-3213-e";
      break;
    case gfx::ColorSpace::PrimaryID::INVALID:
      primaries = "invalid";
      break;
  }

  // Convert transfer function to string
  std::string transfer;
  switch (val.GetTransferID()) {
    case gfx::ColorSpace::TransferID::BT709:
      transfer = "bt709";
      break;
    case gfx::ColorSpace::TransferID::BT709_APPLE:
      transfer = "bt709-apple";
      break;
    case gfx::ColorSpace::TransferID::GAMMA18:
      transfer = "gamma18";
      break;
    case gfx::ColorSpace::TransferID::GAMMA22:
      transfer = "gamma22";
      break;
    case gfx::ColorSpace::TransferID::GAMMA24:
      transfer = "gamma24";
      break;
    case gfx::ColorSpace::TransferID::GAMMA28:
      transfer = "gamma28";
      break;
    case gfx::ColorSpace::TransferID::SMPTE170M:
      transfer = "smpte170m";
      break;
    case gfx::ColorSpace::TransferID::SMPTE240M:
      transfer = "smpte240m";
      break;
    case gfx::ColorSpace::TransferID::LINEAR:
      transfer = "linear";
      break;
    case gfx::ColorSpace::TransferID::LOG:
      transfer = "log";
      break;
    case gfx::ColorSpace::TransferID::LOG_SQRT:
      transfer = "log-sqrt";
      break;
    case gfx::ColorSpace::TransferID::IEC61966_2_4:
      transfer = "iec61966-2-4";
      break;
    case gfx::ColorSpace::TransferID::BT1361_ECG:
      transfer = "bt1361-ecg";
      break;
    case gfx::ColorSpace::TransferID::SRGB:
      transfer = "srgb";
      break;
    case gfx::ColorSpace::TransferID::BT2020_10:
      transfer = "bt2020-10";
      break;
    case gfx::ColorSpace::TransferID::BT2020_12:
      transfer = "bt2020-12";
      break;
    case gfx::ColorSpace::TransferID::PQ:
      transfer = "pq";
      break;
    case gfx::ColorSpace::TransferID::SMPTEST428_1:
      transfer = "smptest428-1";
      break;
    case gfx::ColorSpace::TransferID::HLG:
      transfer = "hlg";
      break;
    case gfx::ColorSpace::TransferID::SRGB_HDR:
      transfer = "srgb-hdr";
      break;
    case gfx::ColorSpace::TransferID::LINEAR_HDR:
      transfer = "linear-hdr";
      break;
    case gfx::ColorSpace::TransferID::CUSTOM:
      transfer = "custom";
      break;
    case gfx::ColorSpace::TransferID::CUSTOM_HDR:
      transfer = "custom-hdr";
      break;
    case gfx::ColorSpace::TransferID::SCRGB_LINEAR_80_NITS:
      transfer = "scrgb-linear-80-nits";
      break;
    case gfx::ColorSpace::TransferID::INVALID:
      transfer = "invalid";
      break;
  }

  // Convert matrix to string
  std::string matrix;
  switch (val.GetMatrixID()) {
    case gfx::ColorSpace::MatrixID::RGB:
      matrix = "rgb";
      break;
    case gfx::ColorSpace::MatrixID::BT709:
      matrix = "bt709";
      break;
    case gfx::ColorSpace::MatrixID::FCC:
      matrix = "fcc";
      break;
    case gfx::ColorSpace::MatrixID::BT470BG:
      matrix = "bt470bg";
      break;
    case gfx::ColorSpace::MatrixID::SMPTE170M:
      matrix = "smpte170m";
      break;
    case gfx::ColorSpace::MatrixID::SMPTE240M:
      matrix = "smpte240m";
      break;
    case gfx::ColorSpace::MatrixID::YCOCG:
      matrix = "ycocg";
      break;
    case gfx::ColorSpace::MatrixID::BT2020_NCL:
      matrix = "bt2020-ncl";
      break;
    case gfx::ColorSpace::MatrixID::YDZDX:
      matrix = "ydzdx";
      break;
    case gfx::ColorSpace::MatrixID::GBR:
      matrix = "gbr";
      break;
    case gfx::ColorSpace::MatrixID::INVALID:
      matrix = "invalid";
      break;
  }

  // Convert range to string
  std::string range;
  switch (val.GetRangeID()) {
    case gfx::ColorSpace::RangeID::LIMITED:
      range = "limited";
      break;
    case gfx::ColorSpace::RangeID::FULL:
      range = "full";
      break;
    case gfx::ColorSpace::RangeID::DERIVED:
      range = "derived";
      break;
    case gfx::ColorSpace::RangeID::INVALID:
      range = "invalid";
      break;
  }

  dict.Set("primaries", primaries);
  dict.Set("transfer", transfer);
  dict.Set("matrix", matrix);
  dict.Set("range", range);

  return dict.GetHandle();
}

bool Converter<gfx::ColorSpace>::FromV8(v8::Isolate* isolate,
                                        v8::Local<v8::Value> val,
                                        gfx::ColorSpace* out) {
  gin::Dictionary dict(isolate);
  if (!gin::ConvertFromV8(isolate, val, &dict))
    return false;

  std::string primaries_str, transfer_str, matrix_str, range_str;

  // Default values if not specified
  gfx::ColorSpace::PrimaryID primaries = gfx::ColorSpace::PrimaryID::BT709;
  gfx::ColorSpace::TransferID transfer = gfx::ColorSpace::TransferID::SRGB;
  gfx::ColorSpace::MatrixID matrix = gfx::ColorSpace::MatrixID::RGB;
  gfx::ColorSpace::RangeID range = gfx::ColorSpace::RangeID::FULL;

  // Get primaries
  if (dict.Get("primaries", &primaries_str)) {
    if (primaries_str == "custom") {
      gin_helper::ErrorThrower(isolate).ThrowTypeError(
          "'custom' not supported.");
      return false;
    }

    if (primaries_str == "bt709")
      primaries = gfx::ColorSpace::PrimaryID::BT709;
    else if (primaries_str == "bt470m")
      primaries = gfx::ColorSpace::PrimaryID::BT470M;
    else if (primaries_str == "bt470bg")
      primaries = gfx::ColorSpace::PrimaryID::BT470BG;
    else if (primaries_str == "smpte170m")
      primaries = gfx::ColorSpace::PrimaryID::SMPTE170M;
    else if (primaries_str == "smpte240m")
      primaries = gfx::ColorSpace::PrimaryID::SMPTE240M;
    else if (primaries_str == "film")
      primaries = gfx::ColorSpace::PrimaryID::FILM;
    else if (primaries_str == "bt2020")
      primaries = gfx::ColorSpace::PrimaryID::BT2020;
    else if (primaries_str == "smptest428-1")
      primaries = gfx::ColorSpace::PrimaryID::SMPTEST428_1;
    else if (primaries_str == "smptest431-2")
      primaries = gfx::ColorSpace::PrimaryID::SMPTEST431_2;
    else if (primaries_str == "p3")
      primaries = gfx::ColorSpace::PrimaryID::P3;
    else if (primaries_str == "xyz-d50")
      primaries = gfx::ColorSpace::PrimaryID::XYZ_D50;
    else if (primaries_str == "adobe-rgb")
      primaries = gfx::ColorSpace::PrimaryID::ADOBE_RGB;
    else if (primaries_str == "apple-generic-rgb")
      primaries = gfx::ColorSpace::PrimaryID::APPLE_GENERIC_RGB;
    else if (primaries_str == "wide-gamut-color-spin")
      primaries = gfx::ColorSpace::PrimaryID::WIDE_GAMUT_COLOR_SPIN;
    else if (primaries_str == "ebu-3213-e")
      primaries = gfx::ColorSpace::PrimaryID::EBU_3213_E;
    else
      primaries = gfx::ColorSpace::PrimaryID::INVALID;
  }

  // Get transfer
  if (dict.Get("transfer", &transfer_str)) {
    if (transfer_str == "custom" || transfer_str == "custom-hdr") {
      gin_helper::ErrorThrower(isolate).ThrowTypeError(
          "'custom', 'custom-hdr' not supported.");
      return false;
    }

    if (transfer_str == "bt709")
      transfer = gfx::ColorSpace::TransferID::BT709;
    else if (transfer_str == "bt709-apple")
      transfer = gfx::ColorSpace::TransferID::BT709_APPLE;
    else if (transfer_str == "gamma18")
      transfer = gfx::ColorSpace::TransferID::GAMMA18;
    else if (transfer_str == "gamma22")
      transfer = gfx::ColorSpace::TransferID::GAMMA22;
    else if (transfer_str == "gamma24")
      transfer = gfx::ColorSpace::TransferID::GAMMA24;
    else if (transfer_str == "gamma28")
      transfer = gfx::ColorSpace::TransferID::GAMMA28;
    else if (transfer_str == "smpte170m")
      transfer = gfx::ColorSpace::TransferID::SMPTE170M;
    else if (transfer_str == "smpte240m")
      transfer = gfx::ColorSpace::TransferID::SMPTE240M;
    else if (transfer_str == "linear")
      transfer = gfx::ColorSpace::TransferID::LINEAR;
    else if (transfer_str == "log")
      transfer = gfx::ColorSpace::TransferID::LOG;
    else if (transfer_str == "log-sqrt")
      transfer = gfx::ColorSpace::TransferID::LOG_SQRT;
    else if (transfer_str == "iec61966-2-4")
      transfer = gfx::ColorSpace::TransferID::IEC61966_2_4;
    else if (transfer_str == "bt1361-ecg")
      transfer = gfx::ColorSpace::TransferID::BT1361_ECG;
    else if (transfer_str == "srgb")
      transfer = gfx::ColorSpace::TransferID::SRGB;
    else if (transfer_str == "bt2020-10")
      transfer = gfx::ColorSpace::TransferID::BT2020_10;
    else if (transfer_str == "bt2020-12")
      transfer = gfx::ColorSpace::TransferID::BT2020_12;
    else if (transfer_str == "pq")
      transfer = gfx::ColorSpace::TransferID::PQ;
    else if (transfer_str == "smptest428-1")
      transfer = gfx::ColorSpace::TransferID::SMPTEST428_1;
    else if (transfer_str == "hlg")
      transfer = gfx::ColorSpace::TransferID::HLG;
    else if (transfer_str == "srgb-hdr")
      transfer = gfx::ColorSpace::TransferID::SRGB_HDR;
    else if (transfer_str == "linear-hdr")
      transfer = gfx::ColorSpace::TransferID::LINEAR_HDR;
    else if (transfer_str == "scrgb-linear-80-nits")
      transfer = gfx::ColorSpace::TransferID::SCRGB_LINEAR_80_NITS;
    else
      transfer = gfx::ColorSpace::TransferID::INVALID;
  }

  // Get matrix
  if (dict.Get("matrix", &matrix_str)) {
    if (matrix_str == "rgb")
      matrix = gfx::ColorSpace::MatrixID::RGB;
    else if (matrix_str == "bt709")
      matrix = gfx::ColorSpace::MatrixID::BT709;
    else if (matrix_str == "fcc")
      matrix = gfx::ColorSpace::MatrixID::FCC;
    else if (matrix_str == "bt470bg")
      matrix = gfx::ColorSpace::MatrixID::BT470BG;
    else if (matrix_str == "smpte170m")
      matrix = gfx::ColorSpace::MatrixID::SMPTE170M;
    else if (matrix_str == "smpte240m")
      matrix = gfx::ColorSpace::MatrixID::SMPTE240M;
    else if (matrix_str == "ycocg")
      matrix = gfx::ColorSpace::MatrixID::YCOCG;
    else if (matrix_str == "bt2020-ncl")
      matrix = gfx::ColorSpace::MatrixID::BT2020_NCL;
    else if (matrix_str == "ydzdx")
      matrix = gfx::ColorSpace::MatrixID::YDZDX;
    else if (matrix_str == "gbr")
      matrix = gfx::ColorSpace::MatrixID::GBR;
    else
      matrix = gfx::ColorSpace::MatrixID::INVALID;
  }

  // Get range
  if (dict.Get("range", &range_str)) {
    if (range_str == "limited")
      range = gfx::ColorSpace::RangeID::LIMITED;
    else if (range_str == "full")
      range = gfx::ColorSpace::RangeID::FULL;
    else if (range_str == "derived")
      range = gfx::ColorSpace::RangeID::DERIVED;
    else
      range = gfx::ColorSpace::RangeID::INVALID;
  }

  *out = gfx::ColorSpace(primaries, transfer, matrix, range);
  return true;
}

}  // namespace gin
