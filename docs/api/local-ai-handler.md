# localAIHandler

> Proxy Built-in AI APIs to a local LLM implementation

Process: [Utility](../glossary.md#utility-process)

This module is intended to be used by a script registered to a session via
[`ses.registerLocalAIHandlerScript(options)`](./session.md#sesregisterlocalaihandlerhandler-experimental)

## Methods

The `localAIHandler` module has the following methods:

#### `localAIHandler.setPromptAPIHandler(handler)`

* `handler` Function\<Promise\<[void](https://github.com/webmachinelearning/prompt-api?tab=readme-ov-file#full-api-surface-in-web-idl)\>\> | null
  * `details` Object
    * `webContentsId` (Integer | null) - The [unique id](web-contents.md#contentsid-readonly) of
      the [WebContents](web-contents.md) calling the Prompt API. The WebContents id may be null
      if the Prompt API is called from a service worker or shared worker.
    * `securityOrigin` string - Origin of the page calling the Prompt API.
