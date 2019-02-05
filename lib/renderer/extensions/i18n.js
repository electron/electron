'use strict'

// Implementation of chrome.i18n.getMessage
// https://developer.chrome.com/extensions/i18n#method-getMessage
//
// Does not implement predefined messages:
// https://developer.chrome.com/extensions/i18n#overview-predefined

const ipcRendererUtils = require('@electron/internal/renderer/ipc-renderer-internal-utils')

const getMessages = (extensionId) => {
  try {
    const data = ipcRendererUtils.invokeSync('CHROME_GET_MESSAGES', extensionId)
    return JSON.parse(data) || {}
  } catch (error) {
    return {}
  }
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
      let { content } = placeholders[name]
      content = replaceNumberedSubstitutions(content, substitutions)
      message = message.replace(new RegExp(`\\$${name}\\$`, 'gi'), content)
    })
  }

  return replaceNumberedSubstitutions(message, substitutions)
}

const getMessage = (extensionId, messageName, substitutions) => {
  const messages = getMessages(extensionId)
  if (messages.hasOwnProperty(messageName)) {
    const { message, placeholders } = messages[messageName]
    return replacePlaceholders(message, placeholders, substitutions)
  }
}

exports.setup = (extensionId) => {
  return {
    getMessage (messageName, substitutions) {
      return getMessage(extensionId, messageName, substitutions)
    }
  }
}
