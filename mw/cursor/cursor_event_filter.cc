#include "mw/cursor/cursor_event_filter.h"
#include "content/common/view_messages.h"

namespace atom {

std::string CursorChangeEvent::toString(
  const content::WebCursor& cursor) {
    content::WebCursor::CursorInfo* info = new content::WebCursor::CursorInfo();
    cursor.GetCursorInfo(info);

    switch(info->type){
      case blink::WebCursorInfo::Type::TypePointer:
        return "Pointer";
      break;
      case blink::WebCursorInfo::Type::TypeCross:
        return "Cross";
      break;
      case blink::WebCursorInfo::Type::TypeHand:
        return "Hand";
      break;
      case blink::WebCursorInfo::Type::TypeIBeam:
        return "IBeam";
      break;
      case blink::WebCursorInfo::Type::TypeWait:
        return "Wait";
      break;
      case blink::WebCursorInfo::Type::TypeHelp:
        return "Help";
      break;
      case blink::WebCursorInfo::Type::TypeEastResize:
        return "EastResize";
      break;
      case blink::WebCursorInfo::Type::TypeNorthResize:
        return "NorthResize";
      break;
      case blink::WebCursorInfo::Type::TypeNorthEastResize:
        return "NorthEastResize";
      break;
      case blink::WebCursorInfo::Type::TypeNorthWestResize:
        return "NorthWestResize";
      break;
      case blink::WebCursorInfo::Type::TypeSouthResize:
        return "SouthResize";
      break;
      case blink::WebCursorInfo::Type::TypeSouthEastResize:
        return "SouthEastResize";
      break;
      case blink::WebCursorInfo::Type::TypeSouthWestResize:
        return "SouthWestResize";
      break;
      case blink::WebCursorInfo::Type::TypeWestResize:
        return "WestResize";
      break;
      case blink::WebCursorInfo::Type::TypeNorthSouthResize:
        return "NorthSouthResize";
      break;
      case blink::WebCursorInfo::Type::TypeEastWestResize:
        return "EastWestResize";
      break;
      case blink::WebCursorInfo::Type::TypeNorthEastSouthWestResize:
        return "NorthEastSouthWestResize";
      break;
      case blink::WebCursorInfo::Type::TypeNorthWestSouthEastResize:
        return "NorthWestSouthEastResize";
      break;
      case blink::WebCursorInfo::Type::TypeColumnResize:
        return "ColumnResize";
      break;
      case blink::WebCursorInfo::Type::TypeRowResize:
        return "RowResize";
      break;
      case blink::WebCursorInfo::Type::TypeMiddlePanning:
        return "MiddlePanning";
      break;
      case blink::WebCursorInfo::Type::TypeEastPanning:
        return "EastPanning";
      break;
      case blink::WebCursorInfo::Type::TypeNorthPanning:
        return "NorthPanning";
      break;
      case blink::WebCursorInfo::Type::TypeNorthEastPanning:
        return "NorthEastPanning";
      break;
      case blink::WebCursorInfo::Type::TypeNorthWestPanning:
        return "NorthWestPanning";
      break;
      case blink::WebCursorInfo::Type::TypeSouthPanning:
        return "SouthPanning";
      break;
      case blink::WebCursorInfo::Type::TypeSouthEastPanning:
        return "SouthEastPanning";
      break;
      case blink::WebCursorInfo::Type::TypeSouthWestPanning:
        return "SouthWestPanning";
      break;
      case blink::WebCursorInfo::Type::TypeWestPanning:
        return "WestPanning";
      break;
      case blink::WebCursorInfo::Type::TypeMove:
        return "Move";
      break;
      case blink::WebCursorInfo::Type::TypeVerticalText:
        return "VerticalText";
      break;
      case blink::WebCursorInfo::Type::TypeCell:
        return "Cell";
      break;
      case blink::WebCursorInfo::Type::TypeContextMenu:
        return "ContextMenu";
      break;
      case blink::WebCursorInfo::Type::TypeAlias:
        return "Alias";
      break;
      case blink::WebCursorInfo::Type::TypeProgress:
        return "Progress";
      break;
      case blink::WebCursorInfo::Type::TypeNoDrop:
        return "NoDrop";
      break;
      case blink::WebCursorInfo::Type::TypeCopy:
        return "Copy";
      break;
      case blink::WebCursorInfo::Type::TypeNone:
        return "None";
      break;
      case blink::WebCursorInfo::Type::TypeNotAllowed:
        return "NotAllowed";
      break;
      case blink::WebCursorInfo::Type::TypeZoomIn:
        return "ZoomIn";
      break;
      case blink::WebCursorInfo::Type::TypeZoomOut:
        return "ZoomOut";
      break;
      case blink::WebCursorInfo::Type::TypeGrab:
        return "Grab";
      break;
      case blink::WebCursorInfo::Type::TypeGrabbing:
        return "Grabbing";
      break;
      case blink::WebCursorInfo::Type::TypeCustom:
        return "Custom";
      break;
      default:
        return "Pointer";
      break;
    }
}

}
