const { shell } = require('electron')

const exLinksBtn = document.getElementById('open-ex-links')

exLinksBtn.addEventListener('click', (event) => {
  shell.openExternal('https://electronjs.org')
})

const OpenAllOutboundLinks = () => {
  const links = document.querySelectorAll('a[href]')

  Array.prototype.forEach.call(links, (link) => {
    const url = link.getAttribute('href')
    if (url.indexOf('http') === 0) {
      link.addEventListener('click', (e) => {
        e.preventDefault()
        shell.openExternal(url)
      })
    }
  })
}
