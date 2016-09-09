const browserActions = {}

const updateBrowserAction = (extensionId, details) => {
  browserActions[extensionId] = browserActions[extensionId] || {}
  if (details.tabId) {
    browserActions[extensionId][tabId] = browserActions[extensionId][tabId] || {}
    browserActions[extensionId][tabId] = Object.assign(browserActions[extensionId][tabId], details)
  } else {
    browserActions[extensionId] = Object.assign(browserActions[extensionId], details)
  }
}

const getBrowserAction = (extensionId, details) => {
  if (details.tabId) {
    if (browserActions[extensionId] && browserActions[extensionId][details.tabId]) {
      return browserActions[extensionId][details.tabId]
    }
  }
  return browserActions[extensionId] || {}
}

module.exports = {
  onDestroyed: (extensionId, tabId) => {
    Object.keys(browserActions).forEach((extensionId) => {
      if (browserActions[extensionId]) {
        delete browserActions[extensionId][tabId]
      }
    })
  },

  setDefaultBrowserAction: (extensionId, details) => {
    updateBrowserAction(extensionId, details)
  },

  setTitle: (extensionId, details) => {
    updateBrowserAction(extensionId, details)
  },

  getTitle: (extensionId, details) => {
    return getBrowserAction(extensionId, details).title
  },

  setBadgeText: (extensionId, details) => {
    updateBrowserAction(extensionId, details)
  },

  getBadgeText: (extensionId, details) => {
    return getBrowserAction(extensionId, details).text
  },

  setBadgeBackgroundColor: (extensionId, details) => {
    updateBrowserAction(extensionId, details)
  },

  getBadgeBackgroundColor: (extensionId, details) => {
    return getBrowserAction(extensionId, details).color
  },

  setPopup: (extensionId, details) => {
    updateBrowserAction(extensionId, details)
  },

  getPopup: (extensionId, details) => {
    return getBrowserAction(extensionId, details).popup
  }
}
