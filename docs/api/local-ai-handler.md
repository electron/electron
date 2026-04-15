# localAIHandler

> Proxy built-in AI APIs to a local LLM implementation

Process: [Utility](../glossary.md#utility-process)

This module is intended to be used by a script registered to a session via
[`ses.registerLocalAIHandler(handler)`](./session.md#sesregisterlocalaihandlerhandler-experimental)

## Methods

The `localAIHandler` module has the following methods:

#### `localAIHandler.setPromptAPIHandler(handler)` _Experimental_

* `handler` Function\<typeof [LanguageModel](language-model.md) | null\> | null
  * `details` Object
    * `webContentsId` Integer - The [unique id](web-contents.md#contentsid-readonly) of
      the [WebContents](web-contents.md) calling the Prompt API.
    * `securityOrigin` string - Origin of the page calling the Prompt API.

Sets the handler for new Prompt API binding requests from the renderer process. This happens
once per pair of `webContentsId` and `securityOrigin`. Returning `null` from the handler
will reject the creation of a new Prompt API session in the renderer. Clearing the handler by
calling `setPromptAPIHandler(null)` will prevent new Prompt API sessions from being started,
but will not invalidate existing ones. If you want to invalidate existing Prompt API sessions,
clear the local AI handler for the session using `ses.registerLocalAIHandler(null)`.

> [!NOTE] If a renderer calls the Prompt API before `setPromptAPIHandler()` has been called,
> the request is queued. Once the handler is set, all queued requests are flushed. If too
> many requests are queued, the oldest pending request is dropped and pending promises in
> the renderer will be rejected. To avoid this, be sure to call `setPromptAPIHandler()` as
> early as possible.
