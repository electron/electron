async function testIt() {
  const grantedDevices = await navigator.hid.getDevices()
  let grantedDeviceList = ''
  grantedDevices.forEach(device => {
    grantedDeviceList += `<hr>${device.productName}</hr>`
  })
  document.getElementById('granted-devices').innerHTML = grantedDeviceList
  const grantedDevices2 = await navigator.hid.requestDevice({
    filters: []
  })

  grantedDeviceList = ''
   grantedDevices2.forEach(device => {
    grantedDeviceList += `<hr>${device.productName}</hr>`
  })
  document.getElementById('granted-devices2').innerHTML = grantedDeviceList
}

document.getElementById('clickme').addEventListener('click',testIt)
