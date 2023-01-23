const pasteBtn = document.getElementById('paste-to')

pasteBtn.addEventListener('click', async () => {
  await clipboard.writeText('What a demo!')
  const message = `Clipboard contents: ${await clipboard.readText()}`
  document.getElementById('paste-from').innerHTML = message
})
