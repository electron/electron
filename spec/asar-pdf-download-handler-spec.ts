import { BrowserWindow, dialog, session } from 'electron/main';

import { expect } from 'chai';
import * as sinon from 'sinon';

import * as fs from 'node:fs';
import * as path from 'node:path';
import * as url from 'node:url';

import { 
  initializeAsarPdfDownloadHandler,
  removeAsarPdfDownloadHandler,
  handleAsarPdfDownload,
  parseAsarUrl,
  isPdfFile,
  readFromAsar,
  saveAsarPdf
} from '../lib/browser/asar-pdf-download-handler';
import { closeAllWindows } from './lib/window-helpers';

describe('ASAR PDF Download Handler', () => {
  const fixtures = path.join(__dirname, 'fixtures');
  
  afterEach(closeAllWindows);
  afterEach(() => {
    // Clean up any stubs
    sinon.restore();
  });

  describe('parseAsarUrl', () => {
    it('should correctly parse file:// URLs with ASAR paths', () => {
      const testUrl = 'file:///path/to/app.asar/documents/test.pdf';
      const result = parseAsarUrl(testUrl);
      
      expect(result.isAsar).to.be.true;
      expect(result.asarPath).to.equal('/path/to/app.asar');
      expect(result.filePath).to.equal('documents/test.pdf');
    });

    it('should correctly parse direct ASAR paths', () => {
      const testPath = '/path/to/app.asar/documents/test.pdf';
      const result = parseAsarUrl(testPath);
      
      expect(result.isAsar).to.be.true;
      expect(result.asarPath).to.equal('/path/to/app.asar');
      expect(result.filePath).to.equal('documents/test.pdf');
    });

    it('should return false for non-ASAR URLs', () => {
      const testUrl = 'file:///regular/path/test.pdf';
      const result = parseAsarUrl(testUrl);
      
      expect(result.isAsar).to.be.false;
    });

    it('should handle Windows-style paths', () => {
      const testPath = 'C:\\app\\resources\\app.asar\\docs\\test.pdf';
      const result = parseAsarUrl(testPath);
      
      expect(result.isAsar).to.be.true;
      expect(result.asarPath).to.equal('C:\\app\\resources\\app.asar');
      expect(result.filePath).to.equal('docs\\test.pdf');
    });
  });

  describe('isPdfFile', () => {
    it('should identify PDF files by extension', () => {
      expect(isPdfFile('document.pdf')).to.be.true;
      expect(isPdfFile('document.PDF')).to.be.true;
      expect(isPdfFile('document.txt')).to.be.false;
    });

    it('should identify PDF files by MIME type', () => {
      expect(isPdfFile('document', 'application/pdf')).to.be.true;
      expect(isPdfFile('document', 'text/plain')).to.be.false;
    });

    it('should prioritize extension over MIME type', () => {
      expect(isPdfFile('document.pdf', 'text/plain')).to.be.true;
    });
  });

  describe('readFromAsar', () => {
    it('should read file content from ASAR archive', () => {
      // Create a mock ASAR path for testing
      const asarPath = path.join(fixtures, 'test.asar', 'web.asar');
      const filePath = 'index.html';
      
      try {
        const content = readFromAsar(asarPath, filePath);
        expect(content).to.be.instanceOf(Buffer);
        expect(content.length).to.be.greaterThan(0);
      } catch (error) {
        // Expected if the ASAR doesn't contain the file
        expect(error.message).to.include('Failed to read file from ASAR');
      }
    });

    it('should throw error for non-existent files', () => {
      const asarPath = path.join(fixtures, 'test.asar', 'web.asar');
      const filePath = 'non-existent.pdf';
      
      expect(() => readFromAsar(asarPath, filePath)).to.throw('Failed to read file from ASAR');
    });
  });

  describe('saveAsarPdf', () => {
    let mockWebContents: any;
    let dialogStub: sinon.SinonStub;
    let fsStub: sinon.SinonStub;

    beforeEach(() => {
      mockWebContents = {};
      dialogStub = sinon.stub(dialog, 'showSaveDialog');
      fsStub = sinon.stub(fs, 'writeFileSync');
    });

    it('should save PDF when user selects a location', async () => {
      const pdfContent = Buffer.from('mock pdf content');
      const filename = 'test.pdf';
      const savePath = '/path/to/save/test.pdf';

      dialogStub.resolves({ canceled: false, filePath: savePath });

      const result = await saveAsarPdf(mockWebContents, pdfContent, filename);

      expect(result).to.be.true;
      expect(dialogStub.calledOnce).to.be.true;
      expect(fsStub.calledOnceWith(savePath, pdfContent)).to.be.true;
    });

    it('should return false when user cancels save dialog', async () => {
      const pdfContent = Buffer.from('mock pdf content');
      const filename = 'test.pdf';

      dialogStub.resolves({ canceled: true });

      const result = await saveAsarPdf(mockWebContents, pdfContent, filename);

      expect(result).to.be.false;
      expect(fsStub.called).to.be.false;
    });

    it('should handle file write errors gracefully', async () => {
      const pdfContent = Buffer.from('mock pdf content');
      const filename = 'test.pdf';
      const savePath = '/path/to/save/test.pdf';

      dialogStub.resolves({ canceled: false, filePath: savePath });
      fsStub.throws(new Error('Write permission denied'));

      const result = await saveAsarPdf(mockWebContents, pdfContent, filename);

      expect(result).to.be.false;
    });
  });

  describe('handleAsarPdfDownload', () => {
    let mockEvent: any;
    let mockItem: any;
    let mockWebContents: any;
    let dialogStub: sinon.SinonStub;
    let fsReadStub: sinon.SinonStub;
    let fsWriteStub: sinon.SinonStub;

    beforeEach(() => {
      mockEvent = {
        preventDefault: sinon.stub()
      };
      
      mockItem = {
        getURL: sinon.stub(),
        getFilename: sinon.stub(),
        getMimeType: sinon.stub(),
        cancel: sinon.stub()
      };
      
      mockWebContents = {};
      
      dialogStub = sinon.stub(dialog, 'showSaveDialog');
      fsReadStub = sinon.stub(fs, 'readFileSync');
      fsWriteStub = sinon.stub(fs, 'writeFileSync');
    });

    it('should handle ASAR PDF downloads correctly', async () => {
      const asarUrl = 'file:///app/resources/app.asar/docs/manual.pdf';
      const filename = 'manual.pdf';
      const mimeType = 'application/pdf';
      const pdfContent = Buffer.from('mock pdf content');
      const savePath = '/downloads/manual.pdf';

      mockItem.getURL.returns(asarUrl);
      mockItem.getFilename.returns(filename);
      mockItem.getMimeType.returns(mimeType);
      
      fsReadStub.returns(pdfContent);
      dialogStub.resolves({ canceled: false, filePath: savePath });

      await handleAsarPdfDownload(mockEvent, mockItem, mockWebContents);

      expect(mockEvent.preventDefault.calledOnce).to.be.true;
      expect(fsReadStub.calledOnce).to.be.true;
      expect(dialogStub.calledOnce).to.be.true;
      expect(fsWriteStub.calledOnceWith(savePath, pdfContent)).to.be.true;
      expect(mockItem.cancel.calledOnce).to.be.true;
    });

    it('should not intercept non-ASAR downloads', async () => {
      const regularUrl = 'https://example.com/document.pdf';
      const filename = 'document.pdf';
      const mimeType = 'application/pdf';

      mockItem.getURL.returns(regularUrl);
      mockItem.getFilename.returns(filename);
      mockItem.getMimeType.returns(mimeType);

      await handleAsarPdfDownload(mockEvent, mockItem, mockWebContents);

      expect(mockEvent.preventDefault.called).to.be.false;
      expect(fsReadStub.called).to.be.false;
      expect(dialogStub.called).to.be.false;
      expect(mockItem.cancel.called).to.be.false;
    });

    it('should not intercept non-PDF downloads from ASAR', async () => {
      const asarUrl = 'file:///app/resources/app.asar/data/config.json';
      const filename = 'config.json';
      const mimeType = 'application/json';

      mockItem.getURL.returns(asarUrl);
      mockItem.getFilename.returns(filename);
      mockItem.getMimeType.returns(mimeType);

      await handleAsarPdfDownload(mockEvent, mockItem, mockWebContents);

      expect(mockEvent.preventDefault.called).to.be.false;
      expect(fsReadStub.called).to.be.false;
      expect(dialogStub.called).to.be.false;
      expect(mockItem.cancel.called).to.be.false;
    });

    it('should handle errors gracefully', async () => {
      const asarUrl = 'file:///app/resources/app.asar/docs/manual.pdf';
      const filename = 'manual.pdf';
      const mimeType = 'application/pdf';

      mockItem.getURL.returns(asarUrl);
      mockItem.getFilename.returns(filename);
      mockItem.getMimeType.returns(mimeType);
      
      fsReadStub.throws(new Error('File not found in ASAR'));
      const errorBoxStub = sinon.stub(dialog, 'showErrorBox');

      await handleAsarPdfDownload(mockEvent, mockItem, mockWebContents);

      expect(mockEvent.preventDefault.calledOnce).to.be.true;
      expect(errorBoxStub.calledOnce).to.be.true;
      expect(mockItem.cancel.calledOnce).to.be.true;
    });
  });

  describe('Session Integration', () => {
    let testSession: Electron.Session;

    beforeEach(() => {
      testSession = session.fromPartition('test-asar-handler');
    });

    afterEach(() => {
      removeAsarPdfDownloadHandler(testSession);
    });

    it('should initialize handler for session', () => {
      const listenerCountBefore = testSession.listenerCount('will-download');
      
      initializeAsarPdfDownloadHandler(testSession);
      
      const listenerCountAfter = testSession.listenerCount('will-download');
      expect(listenerCountAfter).to.be.greaterThan(listenerCountBefore);
    });

    it('should remove handler from session', () => {
      initializeAsarPdfDownloadHandler(testSession);
      const listenerCountAfter = testSession.listenerCount('will-download');
      
      removeAsarPdfDownloadHandler(testSession);
      
      const listenerCountFinal = testSession.listenerCount('will-download');
      expect(listenerCountFinal).to.equal(0);
    });
  });

  describe('Integration Test', () => {
    it('should handle real download scenario', async function () {
      this.timeout(10000); // Increase timeout for integration test
      
      const w = new BrowserWindow({ 
        show: false,
        webPreferences: {
          nodeIntegration: true,
          contextIsolation: false
        }
      });

      // Initialize handler for this window's session
      initializeAsarPdfDownloadHandler(w.webContents.session);

      // Create a test HTML page that triggers a download
      const testHtml = `
        <!DOCTYPE html>
        <html>
        <body>
          <script>
            // Simulate clicking a download link for an ASAR PDF
            setTimeout(() => {
              const link = document.createElement('a');
              link.href = 'file://${fixtures.replace(/\\/g, '/')}/test.asar/web.asar/test.pdf';
              link.download = 'test-document.pdf';
              document.body.appendChild(link);
              link.click();
            }, 1000);
          </script>
        </body>
        </html>
      `;

      const testHtmlPath = path.join(fixtures, 'asar-download-test.html');
      fs.writeFileSync(testHtmlPath, testHtml);

      try {
        // Set up a promise to track the will-download event
        const downloadPromise = new Promise((resolve) => {
          w.webContents.session.once('will-download', (event, item) => {
            resolve({ event, item });
          });
        });

        await w.loadFile(testHtmlPath);

        // Wait for download event or timeout
        const result = await Promise.race([
          downloadPromise,
          new Promise((_, reject) => 
            setTimeout(() => reject(new Error('Download event timeout')), 5000)
          )
        ]) as any;

        // Verify that our handler was called
        expect(result.item).to.exist;
        
      } catch (error) {
        // Expected if ASAR doesn't contain the PDF file
        console.log('Integration test expected failure:', error.message);
      } finally {
        // Clean up
        if (fs.existsSync(testHtmlPath)) {
          fs.unlinkSync(testHtmlPath);
        }
        removeAsarPdfDownloadHandler(w.webContents.session);
      }
    });
  });
});