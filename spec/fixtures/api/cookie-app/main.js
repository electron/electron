const { app, session } = require('electron')

app.on('ready', async function () {
  const url = 'http://foo.bar'
  const persistentSession = session.fromPartition('persist:ence-test')

  const set = () => {
    return new Promise((resolve, reject) => {
      persistentSession.cookies.set({
        url,
        name: 'test',
        value: 'true',
        expirationDate: Date.now() + 60000
      }, error => {
        if (error) {
          reject(error)
        } else {
          resolve()
        }
      })
    })
  }

  const get = () => {
    return new Promise((resolve, reject) => {
      persistentSession.cookies.get({ url }, (error, list) => {
        if (error) {
          reject(error)
        } else {
          resolve(list)
        }
      })
    })
  }

  const maybeRemove = (pred) => {
    return new Promise((resolve, reject) => {
      if (pred()) {
        persistentSession.cookies.remove(url, 'test', error => {
          if (error) {
            reject(error)
          } else {
            resolve()
          }
        })
      } else {
        resolve()
      }
    })
  }

  try {
    await maybeRemove(() => process.env.PHASE === 'one')
    const one = await get()
    await set()
    const two = await get()
    await maybeRemove(() => process.env.PHASE === 'two')
    const three = await get()

    process.stdout.write(`${one.length}${two.length}${three.length}`)
  } catch (e) {
    process.stdout.write('ERROR')
  } finally {
    process.stdout.end()

    setImmediate(() => {
      app.quit()
    })
  }
})
