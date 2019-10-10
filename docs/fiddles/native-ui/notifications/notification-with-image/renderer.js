const notification = {
  title: 'Notification with image',
  body: 'Short message plus a custom image',
  icon: 'https://raw.githubusercontent.com/electron/electron-api-demos/v2.0.2/assets/img/programming.png'
}
const notificationButton = document.getElementById('advanced-noti')

notificationButton.addEventListener('click', () => {
  const myNotification = new window.Notification(notification.title, notification)

  myNotification.onclick = () => {
    console.log('Notification clicked')
  }
})
