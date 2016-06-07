// Implementation of chrome.i18n.getMessage
// https://developer.chrome.com/extensions/i18n#method-getMessage
//
// Does not implement predefined messages:
// https://developer.chrome.com/extensions/i18n#overview-predefined

const {ipcRenderer} = require('electron')
const fs = require('fs')
const path = require('path')

let metadata

const getExtensionMetadata = () => {
  if (!metadata) {
    metadata = ipcRenderer.sendSync('CHROME_I18N_MANIFEST', chrome.runtime.id)
  }
  return metadata
}

const getMessagesPath = (language) => {
  const {srcDirectory, default_locale} = getExtensionMetadata()
  const localesDirectory = path.join(srcDirectory, '_locales')
  let messagesPath = path.join(localesDirectory, language, 'messages.json')
  if (!fs.statSyncNoException(messagesPath)) {
    messagesPath = path.join(localesDirectory, default_locale, 'messages.json')
  }
  return messagesPath
}

const getMessages = (language) => {
  try {
    return JSON.parse(fs.readFileSync(getMessagesPath(language))) || {}
  } catch (error) {
    return {}
  }
}

const getLanguage = () => {
  return navigator.language.replace(/-.*$/, '').toLowerCase()
}

const replaceNumberedSubstitutions = (message, substitutions) => {
  return message.replace(/\$(\d+)/, (_, number) => {
    const index = parseInt(number, 10) - 1
    return substitutions[index] || ''
  })
}

const replacePlaceholders = (message, placeholders, substitutions) => {
  if (typeof substitutions === 'string') {
    substitutions = [substitutions]
  }
  if (!Array.isArray(substitutions)) {
    substitutions = []
  }

  if (placeholders) {
    Object.keys(placeholders).forEach((name) => {
      let {content} = placeholders[name]
      content = replaceNumberedSubstitutions(content, substitutions)
      message = message.replace(new RegExp(`\\$${name}\\$`, 'gi'), content)
    })
  }

  return replaceNumberedSubstitutions(message, substitutions)
}

module.exports = {
  getMessage (messageName, substitutions) {
    const language = getLanguage()
    const messages = getMessages(language)
    if (messages.hasOwnProperty(messageName)) {
      const {message, placeholders} = messages[messageName]
      return replacePlaceholders(message, placeholders, substitutions)
    }
  }
}
