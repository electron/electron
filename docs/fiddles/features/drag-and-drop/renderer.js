document.getElementById('drag').ondragstart = (event) => {
  console.log("draggin...")
  event.preventDefault()
  window.electron.startDrag('drag-and-drop.md')
}
