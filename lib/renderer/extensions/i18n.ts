// Implementation of chrome.i18n.getMessage
// https://developer.chrome.com/extensions/i18n#method-getMessage
//
// Does not implement predefined messages:
// https://developer.chrome.com/extensions/i18n#overview-predefined

import * as ipcRendererUtils from '@electron/internal/renderer/ipc-renderer-internal-utils';

interface Placeholder {
  content: string;
  example?: string;
}

const getMessages = (extensionId: number) => {
  try {
    const data = ipcRendererUtils.invokeSync<string>('CHROME_GET_MESSAGES', extensionId);
    return JSON.parse(data) || {};
  } catch {
    return {};
  }
};

const replaceNumberedSubstitutions = (message: string, substitutions: string[]) => {
  return message.replace(/\$(\d+)/, (_, number) => {
    const index = parseInt(number, 10) - 1;
    return substitutions[index] || '';
  });
};

const replacePlaceholders = (message: string, placeholders: Record<string, Placeholder>, substitutions: string[] | string) => {
  if (typeof substitutions === 'string') substitutions = [substitutions];
  if (!Array.isArray(substitutions)) substitutions = [];

  if (placeholders) {
    Object.keys(placeholders).forEach((name: string) => {
      let { content } = placeholders[name];
      const substitutionsArray = Array.isArray(substitutions) ? substitutions : [];
      content = replaceNumberedSubstitutions(content, substitutionsArray);
      message = message.replace(new RegExp(`\\$${name}\\$`, 'gi'), content);
    });
  }

  return replaceNumberedSubstitutions(message, substitutions);
};

const getMessage = (extensionId: number, messageName: string, substitutions: string[]) => {
  const messages = getMessages(extensionId);
  if (Object.prototype.hasOwnProperty.call(messages, messageName)) {
    const { message, placeholders } = messages[messageName];
    return replacePlaceholders(message, placeholders, substitutions);
  }
};

exports.setup = (extensionId: number) => {
  return {
    getMessage (messageName: string, substitutions: string[]) {
      return getMessage(extensionId, messageName, substitutions);
    }
  };
};
