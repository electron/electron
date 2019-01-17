# C++ Resources

Looking at the electron source code without context can be overwhelming. Here are a list of some objects, and links about those objects. Not intended to be comprehensive, and not all may be necessary to contribute to electron. Familiarity with these objects has been helpful for me.

* [Smart pointers](https://www.chromium.org/developers/smart-pointer-guidelines)
* [static vs dynamic libraries](https://medium.com/@StueyGK/static-libraries-vs-dynamic-libraries-af78f0b5f1e4)

## Chromium Objects

* RenderProcessHost, RenderViewHost, RenderProcess, RenderView: https://www.chromium.org/developers/design-documents/multi-process-architecture
* WebContents, WebFrame: https://www.chromium.org/developers/design-documents/displaying-a-web-page-in-chrome
* gin: https://chromium.googlesource.com/chromium/src.git/+/lkgr/gin/
* SiteInstance and BrowserInstance: https://www.chromium.org/developers/design-documents/site-isolation
