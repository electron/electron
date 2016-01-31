// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include "atom/common/mouse_util.h"

using Cursor = blink::WebCursorInfo::Type;

namespace atom {

std::string CursorTypeToString(const content::WebCursor& cursor) {
  content::WebCursor::CursorInfo* info = new content::WebCursor::CursorInfo();
  cursor.GetCursorInfo(info);

  switch (info->type) {
    case Cursor::TypePointer: return "pointer";
    case Cursor::TypeCross: return "cross";
    case Cursor::TypeHand: return "hand";
    case Cursor::TypeIBeam: return "i-beam";
    case Cursor::TypeWait: return "wait";
    case Cursor::TypeHelp: return "help";
    case Cursor::TypeEastResize: return "east-resize";
    case Cursor::TypeNorthResize: return "north-resize";
    case Cursor::TypeNorthEastResize: return "north-east-resize";
    case Cursor::TypeNorthWestResize: return "north-west-resize";
    case Cursor::TypeSouthResize: return "south-resize";
    case Cursor::TypeSouthEastResize: return "south-east-resize";
    case Cursor::TypeSouthWestResize: return "south-west-resize";
    case Cursor::TypeWestResize: return "west-resize";
    case Cursor::TypeNorthSouthResize: return "north-south-resize";
    case Cursor::TypeEastWestResize: return "east-west-resize";
    case Cursor::TypeNorthEastSouthWestResize:
      return "north-east-south-west-resize";
    case Cursor::TypeNorthWestSouthEastResize:
      return "north-west-south-east-resize";
    case Cursor::TypeColumnResize: return "column-resize";
    case Cursor::TypeRowResize: return "row-resize";
    case Cursor::TypeMiddlePanning: return "middle-panning";
    case Cursor::TypeEastPanning: return "east-panning";
    case Cursor::TypeNorthPanning: return "north-panning";
    case Cursor::TypeNorthEastPanning: return "north-east-panning";
    case Cursor::TypeNorthWestPanning: return "north-west-panning";
    case Cursor::TypeSouthPanning: return "south-panning";
    case Cursor::TypeSouthEastPanning: return "south-east-panning";
    case Cursor::TypeSouthWestPanning: return "south-west-panning";
    case Cursor::TypeWestPanning: return "west-panning";
    case Cursor::TypeMove: return "move";
    case Cursor::TypeVerticalText: return "vertical-text";
    case Cursor::TypeCell: return "cell";
    case Cursor::TypeContextMenu: return "context-menu";
    case Cursor::TypeAlias: return "alias";
    case Cursor::TypeProgress: return "progress";
    case Cursor::TypeNoDrop: return "nodrop";
    case Cursor::TypeCopy: return "copy";
    case Cursor::TypeNone: return "none";
    case Cursor::TypeNotAllowed: return "not-allowed";
    case Cursor::TypeZoomIn: return "zoom-in";
    case Cursor::TypeZoomOut: return "zoom-out";
    case Cursor::TypeGrab: return "grab";
    case Cursor::TypeGrabbing: return "grabbing";
    case Cursor::TypeCustom: return "custom";
    default: return "pointer";
  }
}

}  // namespace atom
