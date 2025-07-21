# LanguageModel

> Implement local AI language models

Process: [Utility](../glossary.md#utility-process)

## Class: LanguageModel

> Implement local AI language models

Process: [Utility](../glossary.md#utility-process)

### `new LanguageModel(initialState)`

* `initialState` Object
  * `inputUsage` number
  * `inputQuota` number
  * `temperature` number
  * `topK` number

> [!NOTE]
> Do not use this constructor directly outside of the class itself, as it will not be properly connected to the `localAIHandler`

### Static Methods

The `LanguageModel` class has the following static methods:

#### `LanguageModel.create([options])` _Experimental_

* `options` [LanguageModelCreateOptions](structures/language-model-create-options.md) (optional)

Returns `Promise<LanguageModel>`

#### `LanguageModel.availability([options])` _Experimental_

* `options` [LanguageModelCreateCoreOptions](structures/language-model-create-core-options.md) (optional)

Returns `Promise<string>`

Determines the availability of the language model and returns one of the following strings:

* `available`
* `downloadable`
* `downloading`
* `unavailable`

#### `LanguageModel.params()` _Experimental_

Returns `Promise<LanguageModelParams | null>`

### Instance Properties

The following properties are available on instances of `LanguageModel`:

#### `languageModel.inputUsage` _Readonly_ _Experimental_

A `number` representing TODO.

#### `languageModel.inputQuota` _Readonly_ _Experimental_

A `number` representing TODO.

#### `languageModel.topK` _Readonly_ _Experimental_

A `number` representing TODO.

#### `languageModel.temperature` _Readonly_ _Experimental_

A `number` representing TODO.

### Instance Methods

The following methods are available on instances of `LanguageModel`:

#### `languageModel.prompt(input, [options])` _Experimental_

* `input` [LanguageModelMessage[]](structures/language-model-message.md) | string
* `options` [LanguageModelPromptOptions](structures/language-model-prompt-options.md) (optional)

<!-- TODO: This types as the wrong ReadableStream, we want the web ReadableStream -->

Returns `Promise<string> | ReadableStream<string>`

#### `languageModel.append(input, [options])` _Experimental_

* `input` [LanguageModelMessage[]](structures/language-model-message.md) | string
* `options` [LanguageModelAppendOptions](structures/language-model-append-options.md) (optional)

Returns `Promise<undefined>`

#### `languageModel.measureInputUsage(input, [options])` _Experimental_

* `input` [LanguageModelMessage[]](structures/language-model-message.md) | string
* `options` [LanguageModelPromptOptions](structures/language-model-prompt-options.md) (optional)

Returns `Promise<number>`

#### `languageModel.clone([options])` _Experimental_

* `options` [LanguageModelCloneOptions](structures/language-model-clone-options.md) (optional)

Returns `Promise<LanguageModel>`

#### `languageModel.destroy()` _Experimental_

Destroys the model
