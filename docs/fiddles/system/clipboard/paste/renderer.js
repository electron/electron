const pasteBtn = document.getElementById('paste-to')
const inputField = document.getElementById('input-field')

pasteBtn.addEventListener('click', async () => {
  await window.clipboard.writeText('What a demo!')
  const message = `Clipboard contents: ${await window.clipboard.readText()}`
  document.getElementById('paste-from').innerHTML = message
})

// Add event listener for the context menu paste action
inputField.addEventListener('contextmenu', (event) => {
  event.preventDefault()
  navigator.clipboard.readText().then(text => {
    const start = inputField.selectionStart
    const end = inputField.selectionEnd
    const textBefore = inputField.value.substring(0, start)
    const textAfter = inputField.value.substring(end)
    inputField.value = `${textBefore}${text}${textAfter}`
    inputField.setSelectionRange(start + text.length, start + text.length)
  })
})
