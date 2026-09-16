(async () => {
  try {
    postMessage(typeof LanguageModel === 'undefined' ? 'no LanguageModel' : String(await LanguageModel.availability()));
  } catch (e) {
    postMessage('threw: ' + e.message);
  }
})();
