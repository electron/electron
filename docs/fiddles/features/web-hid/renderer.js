async function testIt () {
  const grantedDevices = await navigator.hid.getDevices()
  let grantedDeviceList = ''
  for (const device of grantedDevices) {
    grantedDeviceList += `<hr>${device.productName}</hr>`
  }
  document.getElementById('granted-devices').innerHTML = grantedDeviceList
  const grantedDevices2 = await navigator.hid.requestDevice({
    filters: []
  })

  grantedDeviceList = ''
  for (const device of grantedDevices2) {
    grantedDeviceList += `<hr>${device.productName}</hr>`
  }
  document.getElementById('granted-devices2').innerHTML = grantedDeviceList
}

document.getElementById('clickme').addEventListener('click', testIt)
