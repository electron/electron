exports.closeWindow = (window) => {
  if (window == null || window.isDestroyed()) {
    return Promise.resolve()
  } else {
    return new Promise((resolve, reject) => {
      window.once('closed', () => {
        resolve()
      })
      window.setClosable(true)
      window.close()
    })
  }
}
