async function testIt() {
  const noDevicesFoundMsg = 'No devices found'
  const grantedDevices = await navigator.usb.getDevices()
  let grantedDeviceList = ''
  if (grantedDevices.length > 0) {    
    grantedDevices.forEach(device => {
      grantedDeviceList += `<hr>${device.productName}</hr>`
    })    
  } else {
    grantedDeviceList = noDevicesFoundMsg
  }
  document.getElementById('granted-devices').innerHTML = grantedDeviceList

  grantedDeviceList = ''
  try {
    const grantedDevice = await navigator.usb.requestDevice({
      filters: []
    })
    grantedDeviceList += `<hr>${grantedDevice.productName}</hr>`
    
  } catch (ex) {
    if (ex.name === 'NotFoundError') {
      grantedDeviceList = noDevicesFoundMsg
    }
  }
  document.getElementById('granted-devices2').innerHTML = grantedDeviceList
}

document.getElementById('clickme').addEventListener('click',testIt)
