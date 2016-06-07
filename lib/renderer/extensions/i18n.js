const {ipcRenderer} = require('electron')
const fs = require('fs')
const path = require('path')

const getExtensionMetadata = () => {
  return ipcRenderer.sendSync('CHROME_I18N_MANIFEST', chrome.runtime.id)
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

module.exports = {
  getMessage (messageName, substitutions) {
    const language = getLanguage()
    const messages = getMessages(language)
    if (messages.hasOwnProperty(messageName)) {
      return messages[messageName].message
    }
  }
}
