const pasteBtn = document.getElementById('paste-to')

pasteBtn.addEventListener('click', async () => {
  await window.clipboard.writeText('What a demo!')
  const message = `Clipboard contents: ${await window.clipboard.readText()}`
  document.getElementById('paste-from').innerHTML = message
})
