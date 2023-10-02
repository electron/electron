const { parseDocs } = require('@electron/docs-parser');
const fs = require('node:fs');
const path = require('node:path');

const { getElectronVersion } = require('./lib/get-version');

async function generateElectronAPI(){
  try{
    const baseDirectory = path.resolve(__dirname, '..');
    const packageMode = 'single';
    const useReadme = false;
    const moduleVersion = getElectronVersion();


    const api = await parseDocs({
      baseDirectory,
      packageMode,
      useReadme,
      moduleVersion
    });
    
    const ouputFilePath = path.resolve(baseDirectory, 'electron-api.json');
    await fs.promises.writeFile(outputFilePath, JSON.stringify(api,null, 2));

    console.log('Electron API documentation generated Successfully');
  }catch(err){
    console.error('Error generating Electron API documentaion:',err);
    process.exit(1);
  }
}

generateElectronAPI();
