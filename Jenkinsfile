pipeline {
  agent none
  stages {
    stage('Build') {
      parallel {
        stage('electron-osx-x64') {
            agent {
              label 'osx'
            }
            steps {
              sh 'script/bootstrap.py --target_arch=x64 --dev'
              sh 'npm run lint'
              sh 'script/build.py -c D'
              sh 'script/test.py --ci --rebuild_native_modules'
            }
            post {
              always {
                cleanWs()
              }
            }
        }
        stage('electron-mas-x64') {
          agent {
            label 'osx'
          }
          environment {
            MAS_BUILD = '1'
          }
          steps {
            sh 'script/bootstrap.py --target_arch=x64 --dev'
            sh 'npm run lint'
            sh 'script/build.py -c D'
            sh 'script/test.py --ci --rebuild_native_modules'
          }
          post {
            always {
              cleanWs()
            }
          }
        }
      }
    }
  }
}
