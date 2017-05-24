#import "atom/browser/api/atom_api_screen.h"

#import <Cocoa/Cocoa.h>

namespace atom {

namespace api {
#if defined(OS_MACOSX)
int Screen::getMenuBarHeight(){
  return [[NSApp mainMenu] menuBarHeight];
}
#endif
}// namespace api

}// namespace atom
