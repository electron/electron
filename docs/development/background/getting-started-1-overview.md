# Overview

Electron is implemented in a combination of C++ and JavaScript. Electron's C++ heavily references the following APIs.

* [Chromium Content API](https://www.chromium.org/developers/content-module/content-api)
* [V8](https://v8docs.nodesource.com/)
* [Nodejs](https://nodejs.org/api/addons.html)

## The following design documents from Chrome are helpful introductions to some of the objects you see referenced in the Electron project.

* [How Blink works](https://docs.google.com/document/d/1aitSOucL0VHZa9Z2vbRJSyAIsAz24kX8LFByQ5xQnUg/edit#heading=h.nedd4zrjusm3)
* [Displaying a web page in Chrome](https://www.chromium.org/developers/design-documents/displaying-a-web-page-in-chrome)
* [MultiProcess Architecture](https://www.chromium.org/developers/design-documents/multi-process-architecture)