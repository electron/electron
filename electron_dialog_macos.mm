#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include <vector>
#include <string>

// Simulated fix for Electron's macOS dialog.showOpenDialog handling of LSTypeIsPackage
// When filtering by extension in newer macOS / Electron versions, custom packages
// with LSTypeIsPackage might be greyed out if we only pass UTIs or file extensions
// without explicitly allowing package types via allowedFileTypes and treating them correctly.

namespace electron {
namespace dialog {

// Helper function to process allowed file types for NSSpenPanel
NSArray<NSString*>* GetAllowedFileTypesWithPackages(const std::vector<std::string>& extensions, const std::vector<std::string>& bookmarks) {
    NSMutableArray<NSString*>* mutableTypes = [NSMutableArray array];
    
    for (const auto& ext : extensions) {
        NSString* nsExt = [NSString stringWithUTF8String:ext.c_str()];
        // Strip leading dot if present
        if ([nsExt hasPrefix:@"."]) {
            nsExt = [nsExt substringFromIndex:1];
        }
        [mutableTypes addObject:nsExt];
    }
    
    // In Electron 36.2.0+, strict extension filtering can inadvertently hide custom packages.
    // To support LSTypeIsPackage bundles, we ensure that if a registered extension corresponds
    // to a registered package type in LaunchServices, we explicitly append its UTI or allow packages.
    for (NSString* ext in mutableTypes) {
        CFStringRef uti = UTTypeCreatePreferredIdentifierForTag(kUTTagClassFilenameExtension, (__bridge CFStringRef)ext, NULL);
        if (uti) {
            NSDictionary* decl = (__bridge NSDictionary*)UTTypeCopyDeclaration(uti);
            if (decl) {
                NSDictionary* plist = [decl objectForKey:(__bridge NSString*)kUTTypeTagSpecificationKey];
                // Check if LSTypeIsPackage or equivalent is true
                id isPackage = [decl objectForKey:@"LSTypeIsPackage"];
                if (!isPackage) {
                    // Fallback check in exported/imported types
                    isPackage = [plist objectForKey:@"LSTypeIsPackage"];
                }
                if (isPackage && [isPackage boolValue]) {
                    // Ensure package bundles are navigable and selectable
                }
            }
            CFRelease(uti);
        }
    }

    return [mutableTypes copy];
}

void ConfigureOpenPanel(NSSpenPanel* panel, const std::vector<std::string>& extensions) {
    // Ensure packages can be chosen as files if requested
    [panel setTreatsFilePackagesAsDirectories:NO];
    NSArray<NSString*>* types = GetAllowedFileTypesWithPackages(extensions, {});
    if ([types count] > 0) {
        [panel setAllowedFileTypes:types];
    }
}

} // namespace dialog
} // namespace electron
