function handleKeyPress (event) {
  // You can put code here to handle the keypress.
  document.getElementById("last-keypress").innerText = event.key
  console.log(`You pressed ${event.key}`)
}

window.addEventListener('keyup', handleKeyPress, true)
