# localAIHandler

> Proxy Built-in AI APIs to a local LLM implementation

Process: [Utility](../glossary.md#utility-process)

This module is intended to be used by a script registered to a session via
[`ses.registerLocalAIHandlerScript(options)`](./session.md#sesregisterlocalaihandlerhandler-experimental)

## Methods

The `localAIHandler` module has the following methods:

#### `localAIHandler.setPromptAPIHandler(handler)` _Experimental_

* `handler` Function\<Promise\<typeof [LanguageModel](language-model.md)\>\> | null
  * `details` Object
    * `webContentsId` (Integer | null) - The [unique id](web-contents.md#contentsid-readonly) of
      the [WebContents](web-contents.md) calling the Prompt API. The WebContents id may be null
      if the Prompt API is called from a service worker or shared worker.
    * `securityOrigin` string - Origin of the page calling the Prompt API.
