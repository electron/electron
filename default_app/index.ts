async function getOcticonSvg (name: string) {
  try {
    const response = await fetch(`octicon/${name}.svg`)
    const div = document.createElement('div')
    div.innerHTML = await response.text()
    return div
  } catch {
    return null
  }
}

async function loadSVG (element: HTMLSpanElement) {
  for (const cssClass of element.classList) {
    if (cssClass.startsWith('octicon-')) {
      const icon = await getOcticonSvg(cssClass.substr(8))
      if (icon) {
        for (const elemClass of element.classList) {
          icon.classList.add(elemClass)
        }
        element.before(icon)
        element.remove()
        break
      }
    }
  }
}

for (const element of document.querySelectorAll<HTMLSpanElement>('.octicon')) {
  loadSVG(element)
}
