function formatDevices (devices) {
  return devices.map(device => device.productName).join('<hr>')
}

async function testIt () {
  document.getElementById('granted-devices').innerHTML = formatDevices(await navigator.hid.getDevices())
  document.getElementById('granted-devices2').innerHTML = formatDevices(await navigator.hid.requestDevice({ filters: [] }))
}

document.getElementById('clickme').addEventListener('click', testIt)
