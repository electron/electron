## Class: LanguageModel

> Implement local AI language models

Process: [Utility](../glossary.md#utility-process)

### `new LanguageModel(initialState)`

* `initialState` Object
  * `contextUsage` number
  * `contextWindow` number

> [!NOTE]
> Do not use this constructor directly outside of the class itself, as it will not be properly connected to the `localAIHandler`

### Static Methods

The `LanguageModel` class has the following static methods:

#### `LanguageModel.create(options)` _Experimental_

* `options` [LanguageModelCreateOptions](structures/language-model-create-options.md)

Returns `Promise<LanguageModel>`. Creates a new `LanguageModel` with the provided `options`.

#### `LanguageModel.availability([options])` _Experimental_

* `options` [LanguageModelCreateCoreOptions](structures/language-model-create-core-options.md) (optional)

Returns `Promise<string>`

Determines the availability of the language model and returns one of the following strings:

* `available`
* `downloadable`
* `downloading`
* `unavailable`

### Instance Properties

The following properties are available on instances of `LanguageModel`:

#### `languageModel.contextUsage` _Experimental_

A `number` representing how many tokens are currently in the context window.

#### `languageModel.contextWindow` _Experimental_

A `number` representing the size of the context window, in tokens.

### Instance Methods

The following methods are available on instances of `LanguageModel`:

#### `languageModel.prompt(input, options)` _Experimental_

* `input` [LanguageModelMessage[]](structures/language-model-message.md)
* `options` [LanguageModelPromptOptions](structures/language-model-prompt-options.md)

Returns `Promise<string> | Promise<import('stream/web').ReadableStream<string>>`. Prompt the model for a response.

#### `languageModel.append(input, options)` _Experimental_

* `input` [LanguageModelMessage[]](structures/language-model-message.md)
* `options` [LanguageModelAppendOptions](structures/language-model-append-options.md)

Returns `Promise<undefined>`. Append a message without prompting for a response.

#### `languageModel.measureContextUsage(input, options)` _Experimental_

* `input` [LanguageModelMessage[]](structures/language-model-message.md)
* `options` [LanguageModelPromptOptions](structures/language-model-prompt-options.md)

Returns `Promise<number>`. Measure how many tokens the input would use.

#### `languageModel.clone(options)` _Experimental_

* `options` [LanguageModelCloneOptions](structures/language-model-clone-options.md)

Returns `Promise<LanguageModel>`. Clones the `LanguageModel` such that the
context and initial prompt should be preserved.

#### `languageModel.destroy()` _Experimental_

Destroys the model, and any ongoing executions are aborted.
