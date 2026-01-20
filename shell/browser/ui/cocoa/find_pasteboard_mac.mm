#import <AppKit/AppKit.h>

void WriteTextToFindPasteboard(NSString* text) {
  if (!text || text.length == 0)
    return;

  NSPasteboard* findPasteboard =
      [NSPasteboard pasteboardWithName:NSPasteboardNameFind];

  [findPasteboard clearContents];
  [findPasteboard setString:text
                    forType:NSPasteboardTypeString];
}
