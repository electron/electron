// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "shell/common/mouse_util.h"
#include "ui/base/cursor/mojom/cursor_type.mojom-shared.h"

using Cursor = ui::mojom::CursorType;

namespace electron {

std::string CursorTypeToString(const ui::Cursor& cursor) {
  switch (cursor.type()) {
    case Cursor::kPointer:
      return "default";
    case Cursor::kCross:
      return "crosshair";
    case Cursor::kHand:
      return "pointer";
    case Cursor::kIBeam:
      return "text";
    case Cursor::kWait:
      return "wait";
    case Cursor::kHelp:
      return "help";
    case Cursor::kEastResize:
      return "e-resize";
    case Cursor::kNorthResize:
      return "n-resize";
    case Cursor::kNorthEastResize:
      return "ne-resize";
    case Cursor::kNorthWestResize:
      return "nw-resize";
    case Cursor::kSouthResize:
      return "s-resize";
    case Cursor::kSouthEastResize:
      return "se-resize";
    case Cursor::kSouthWestResize:
      return "sw-resize";
    case Cursor::kWestResize:
      return "w-resize";
    case Cursor::kNorthSouthResize:
      return "ns-resize";
    case Cursor::kEastWestResize:
      return "ew-resize";
    case Cursor::kNorthEastSouthWestResize:
      return "nesw-resize";
    case Cursor::kNorthWestSouthEastResize:
      return "nwse-resize";
    case Cursor::kColumnResize:
      return "col-resize";
    case Cursor::kRowResize:
      return "row-resize";
    case Cursor::kMiddlePanning:
      return "m-panning";
    case Cursor::kEastPanning:
      return "e-panning";
    case Cursor::kNorthPanning:
      return "n-panning";
    case Cursor::kNorthEastPanning:
      return "ne-panning";
    case Cursor::kNorthWestPanning:
      return "nw-panning";
    case Cursor::kSouthPanning:
      return "s-panning";
    case Cursor::kSouthEastPanning:
      return "se-panning";
    case Cursor::kSouthWestPanning:
      return "sw-panning";
    case Cursor::kWestPanning:
      return "w-panning";
    case Cursor::kMove:
      return "move";
    case Cursor::kVerticalText:
      return "vertical-text";
    case Cursor::kCell:
      return "cell";
    case Cursor::kContextMenu:
      return "context-menu";
    case Cursor::kAlias:
      return "alias";
    case Cursor::kProgress:
      return "progress";
    case Cursor::kNoDrop:
      return "nodrop";
    case Cursor::kCopy:
      return "copy";
    case Cursor::kNone:
      return "none";
    case Cursor::kNotAllowed:
      return "not-allowed";
    case Cursor::kZoomIn:
      return "zoom-in";
    case Cursor::kZoomOut:
      return "zoom-out";
    case Cursor::kGrab:
      return "grab";
    case Cursor::kGrabbing:
      return "grabbing";
    case Cursor::kCustom:
      return "custom";
    default:
      return "default";
  }
}

}  // namespace electron
