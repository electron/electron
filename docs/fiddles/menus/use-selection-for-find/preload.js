const { ipcRenderer } = require('electron/renderer')

function getSelectedText () {
  // `window.getSelection()` does not include text selected inside form fields.
  const element = document.activeElement
  if (element instanceof HTMLInputElement || element instanceof HTMLTextAreaElement) {
    return element.value.slice(element.selectionStart ?? 0, element.selectionEnd ?? 0)
  }
  return window.getSelection()?.toString() ?? ''
}

window.addEventListener('DOMContentLoaded', () => {
  const searchField = document.getElementById('search')
  const findText = document.getElementById('find-text')
  const status = document.getElementById('status')

  // highlight-start
  ipcRenderer.on('find:use-selection', () => {
    const text = getSelectedText()
    if (text) ipcRenderer.send('find:set-text', text)
  })
  // highlight-end

  ipcRenderer.on('find:text-changed', (_event, text) => {
    findText.dataset.text = text
    status.textContent = ''
  })

  ipcRenderer.on('find:focus', () => {
    searchField.focus()
  })

  ipcRenderer.on('find:result', (_event, activeMatch, matches) => {
    status.textContent = matches ? `(${activeMatch} of ${matches})` : '(no matches)'
  })

  searchField.addEventListener('keydown', (event) => {
    if (event.key !== 'Enter') return
    if (searchField.value) {
      ipcRenderer.send('find:search', searchField.value)
      // `findInPage` also matches text inside form fields, so clear the
      // field to keep it from counting as a match.
      searchField.value = ''
    } else {
      // Enter / Shift+Enter in the empty field moves to the next / previous match.
      ipcRenderer.send('find:next', !event.shiftKey)
    }
  })
})
