{
  "name": "master-chef-arab-system",
  "version": "1.0.0",
  "description": "نظام الاتحاد الدولي ماستر شيف العرب",
  "main": "main.js",
  "scripts": {
    "start": "electron .",
    "build": "electron-builder",
    "watch": "nodemon --watch src --exec electron ."
  },
  "keywords": [
    "master",
    "chef",
    "arab",
    "system"
  ],
  "author": "Your Name",
  "license": "ISC",
  "devDependencies": {
    "electron": "^28.1.0",
    "electron-builder": "^24.9.1",
    "nodemon": "^3.0.2"
  },
  "dependencies": {
    "sqlite3": "^5.1.6",
    "xlsx": "^0.19.3",
    "chart.js": "^4.4.0",
    "papaparse": "^5.4.1",
    "bootstrap": "^5.3.2",
    "jquery": "^3.7.1"
  },
  "build": {
    "appId": "com.example.masterchefarab",
    "productName": "Master Chef Arab System",
    "directories": {
      "output": "dist"
    },
    "win": {
      "target": "nsis",
      "icon": "build/icon.ico"
    },
    "mac": {
      "target": "dmg",
      "icon": "build/icon.icns"
    },
    "linux": {
      "target": "AppImage",
      "icon": "build/icon.png"
    }
  }
}
