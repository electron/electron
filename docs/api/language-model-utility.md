## Class: LanguageModelUtility

> Implement local AI language models

Process: [Utility](../glossary.md#utility-process)

### `new LanguageModelUtility(initialState)`

* `initialState` Object
  * `contextUsage` number
  * `contextWindow` number

> [!NOTE]
> Do not use this constructor directly outside of the class itself, as it will not be properly connected to the `localAIHandler`

### Static Methods

The `LanguageModelUtility` class has the following static methods:

#### `LanguageModelUtility.create(options)` _Experimental_

* `options` [LanguageModelCreateOptions](structures/language-model-create-options.md)

Returns `Promise<LanguageModelUtility>`. Creates a new `LanguageModelUtility` with the provided `options`.

#### `LanguageModelUtility.availability([options])` _Experimental_

* `options` [LanguageModelCreateCoreOptions](structures/language-model-create-core-options.md) (optional)

Returns `Promise<string>`

Determines the availability of the language model and returns one of the following strings:

* `available`
* `downloadable`
* `downloading`
* `unavailable`

### Instance Properties

The following properties are available on instances of `LanguageModelUtility`:

#### `languageModelUtility.contextUsage` _Experimental_

A `number` representing how many tokens are currently in the context window.

#### `languageModelUtility.contextWindow` _Experimental_

A `number` representing the size of the context window, in tokens.

### Instance Methods

The following methods are available on instances of `LanguageModelUtility`:

#### `languageModelUtility.prompt(input, options)` _Experimental_

* `input` [LanguageModelMessage[]](structures/language-model-message.md)
* `options` [LanguageModelPromptOptions](structures/language-model-prompt-options.md)

Returns `Promise<string> | Promise<import('stream/web').ReadableStream<string>>`. Prompt the model for a response.

#### `languageModelUtility.append(input, options)` _Experimental_

* `input` [LanguageModelMessage[]](structures/language-model-message.md)
* `options` [LanguageModelAppendOptions](structures/language-model-append-options.md)

Returns `Promise<undefined>`. Append a message without prompting for a response.

#### `languageModelUtility.measureContextUsage(input, options)` _Experimental_

* `input` [LanguageModelMessage[]](structures/language-model-message.md)
* `options` [LanguageModelPromptOptions](structures/language-model-prompt-options.md)

Returns `Promise<number>`. Measure how many tokens the input would use.

#### `languageModelUtility.clone(options)` _Experimental_

* `options` [LanguageModelCloneOptions](structures/language-model-clone-options.md)

Returns `Promise<LanguageModelUtility>`. Clones the `LanguageModelUtility` such that the
context and initial prompt should be preserved.

#### `languageModelUtility.destroy()` _Experimental_

Destroys the model, and any ongoing executions are aborted.
