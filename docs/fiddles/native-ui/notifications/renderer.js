const { shell } = require('electron')

const basicNotification = {
  title: 'Basic Notification',
  body: 'Short message part'
}

const notification = {
  title: 'Notification with image',
  body: 'Short message plus a custom image',
  icon: 'https://via.placeholder.com/150'
}

const basicNotificationButton = document.getElementById('basic-noti')
const notificationButton = document.getElementById('advanced-noti')

notificationButton.addEventListener('click', () => {
  const myNotification = new window.Notification(notification.title, notification)

  myNotification.onclick = () => {
    console.log('Notification clicked')
  }
})

basicNotificationButton.addEventListener('click', () => {
  const myNotification = new window.Notification(basicNotification.title, basicNotification)

  myNotification.onclick = () => {
    console.log('Notification clicked')
  }
})

const links = document.querySelectorAll('a[href]')

for (const link of links) {
  const url = link.getAttribute('href')
  if (url.indexOf('http') === 0) {
    link.addEventListener('click', (e) => {
      e.preventDefault()
      shell.openExternal(url)
    })
  }
}
