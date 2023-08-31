const copyBtn = document.getElementById('copy-to')
const copyInput = document.getElementById('copy-to-input')

copyBtn.addEventListener('click', () => {
  if (copyInput.value !== '') copyInput.value = ''
  copyInput.placeholder = 'Copied! Paste here to see.'
  window.clipboard.writeText('Electron Demo!')
})
