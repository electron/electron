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
