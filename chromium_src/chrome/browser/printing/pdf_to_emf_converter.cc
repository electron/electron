// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/pdf_to_emf_converter.h"

#include <stdint.h>

#include <queue>
#include <utility>

#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/macros.h"
#include "chrome/common/chrome_utility_messages.h"
#include "chrome/common/chrome_utility_printing_messages.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_data.h"
#include "content/public/browser/utility_process_host.h"
#include "content/public/browser/utility_process_host_client.h"
#include "printing/emf_win.h"
#include "printing/pdf_render_settings.h"
#include "ui/base/l10n/l10n_util.h"

namespace printing {

namespace {

using content::BrowserThread;

class PdfToEmfConverterImpl;

// Allows to delete temporary directory after all temporary files created inside
// are closed. Windows cannot delete directory with opened files. Directory is
// used to store PDF and metafiles. PDF should be gone by the time utility
// process exits. Metafiles should be gone when all LazyEmf destroyed.
class RefCountedTempDir
    : public base::RefCountedThreadSafe<RefCountedTempDir,
                                        BrowserThread::DeleteOnFileThread> {
 public:
  RefCountedTempDir() { ignore_result(temp_dir_.CreateUniqueTempDir()); }
  bool IsValid() const { return temp_dir_.IsValid(); }
  const base::FilePath& GetPath() const { return temp_dir_.path(); }

 private:
  friend struct BrowserThread::DeleteOnThread<BrowserThread::FILE>;
  friend class base::DeleteHelper<RefCountedTempDir>;
  ~RefCountedTempDir() {}

  base::ScopedTempDir temp_dir_;
  DISALLOW_COPY_AND_ASSIGN(RefCountedTempDir);
};

typedef scoped_ptr<base::File, BrowserThread::DeleteOnFileThread>
    ScopedTempFile;

// Wrapper for Emf to keep only file handle in memory, and load actual data only
// on playback. Emf::InitFromFile() can play metafile directly from disk, but it
// can't open file handles. We need file handles to reliably delete temporary
// files, and to efficiently interact with utility process.
class LazyEmf : public MetafilePlayer {
 public:
  LazyEmf(const scoped_refptr<RefCountedTempDir>& temp_dir, ScopedTempFile file)
      : temp_dir_(temp_dir), file_(file.Pass()) {
    CHECK(file_);
  }
  ~LazyEmf() override { Close(); }

  bool SafePlayback(HDC hdc) const override;
  bool SaveTo(base::File* file) const override;

 private:
  void Close() const;
  bool LoadEmf(Emf* emf) const;

  mutable scoped_refptr<RefCountedTempDir> temp_dir_;
  mutable ScopedTempFile file_;  // Mutable because of consts in base class.

  DISALLOW_COPY_AND_ASSIGN(LazyEmf);
};

// Converts PDF into EMF.
// Class uses 3 threads: UI, IO and FILE.
// Internal workflow is following:
// 1. Create instance on the UI thread. (files_, settings_,)
// 2. Create pdf file on the FILE thread.
// 3. Start utility process and start conversion on the IO thread.
// 4. Utility process returns page count.
// 5. For each page:
//   1. Clients requests page with file handle to a temp file.
//   2. Utility converts the page, save it to the file and reply.
//
// All these steps work sequentially, so no data should be accessed
// simultaneously by several threads.
class PdfToEmfUtilityProcessHostClient
    : public content::UtilityProcessHostClient {
 public:
  PdfToEmfUtilityProcessHostClient(
      base::WeakPtr<PdfToEmfConverterImpl> converter,
      const PdfRenderSettings& settings);

  void Start(const scoped_refptr<base::RefCountedMemory>& data,
             const PdfToEmfConverter::StartCallback& start_callback);

  void GetPage(int page_number,
               const PdfToEmfConverter::GetPageCallback& get_page_callback);

  void Stop();

  // UtilityProcessHostClient implementation.
  void OnProcessCrashed(int exit_code) override;
  void OnProcessLaunchFailed() override;
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  class GetPageCallbackData {
    MOVE_ONLY_TYPE_FOR_CPP_03(GetPageCallbackData);

   public:
    GetPageCallbackData(int page_number,
                        PdfToEmfConverter::GetPageCallback callback)
        : page_number_(page_number), callback_(callback) {}

    GetPageCallbackData(GetPageCallbackData&& other) {
      *this = std::move(other);
    }

    GetPageCallbackData& operator=(GetPageCallbackData&& rhs) {
      page_number_ = rhs.page_number_;
      callback_ = rhs.callback_;
      emf_ = std::move(rhs.emf_);
      return *this;
    }

    int page_number() const { return page_number_; }
    const PdfToEmfConverter::GetPageCallback& callback() const {
      return callback_;
    }
    ScopedTempFile TakeEmf() { return emf_.Pass(); }
    void set_emf(ScopedTempFile emf) { emf_ = emf.Pass(); }

   private:
    int page_number_;
    PdfToEmfConverter::GetPageCallback callback_;
    ScopedTempFile emf_;
  };

  ~PdfToEmfUtilityProcessHostClient() override;

  bool Send(IPC::Message* msg);

  // Message handlers.
  void OnProcessStarted();
  void OnPageCount(int page_count);
  void OnPageDone(bool success, float scale_factor);

  void OnFailed();
  void OnTempPdfReady(ScopedTempFile pdf);
  void OnTempEmfReady(GetPageCallbackData* callback_data, ScopedTempFile emf);

  scoped_refptr<RefCountedTempDir> temp_dir_;

  // Used to suppress callbacks after PdfToEmfConverterImpl is deleted.
  base::WeakPtr<PdfToEmfConverterImpl> converter_;
  PdfRenderSettings settings_;
  scoped_refptr<base::RefCountedMemory> data_;

  // Document loaded callback.
  PdfToEmfConverter::StartCallback start_callback_;

  // Process host for IPC.
  base::WeakPtr<content::UtilityProcessHost> utility_process_host_;

  // Queue of callbacks for GetPage() requests. Utility process should reply
  // with PageDone in the same order as requests were received.
  // Use containers that keeps element pointers valid after push() and pop().
  typedef std::queue<GetPageCallbackData> GetPageCallbacks;
  GetPageCallbacks get_page_callbacks_;

  DISALLOW_COPY_AND_ASSIGN(PdfToEmfUtilityProcessHostClient);
};

class PdfToEmfConverterImpl : public PdfToEmfConverter {
 public:
  PdfToEmfConverterImpl();

  ~PdfToEmfConverterImpl() override;

  void Start(const scoped_refptr<base::RefCountedMemory>& data,
             const PdfRenderSettings& conversion_settings,
             const StartCallback& start_callback) override;

  void GetPage(int page_number,
               const GetPageCallback& get_page_callback) override;

  // Helps to cancel callbacks if this object is destroyed.
  void RunCallback(const base::Closure& callback);

 private:
  scoped_refptr<PdfToEmfUtilityProcessHostClient> utility_client_;
  base::WeakPtrFactory<PdfToEmfConverterImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PdfToEmfConverterImpl);
};

ScopedTempFile CreateTempFile(scoped_refptr<RefCountedTempDir>* temp_dir) {
  if (!temp_dir->get())
    *temp_dir = new RefCountedTempDir();
  ScopedTempFile file;
  if (!(*temp_dir)->IsValid())
    return file.Pass();
  base::FilePath path;
  if (!base::CreateTemporaryFileInDir((*temp_dir)->GetPath(), &path)) {
    PLOG(ERROR) << "Failed to create file in "
                << (*temp_dir)->GetPath().value();
    return file.Pass();
  }
  file.reset(new base::File(path,
                            base::File::FLAG_CREATE_ALWAYS |
                            base::File::FLAG_WRITE |
                            base::File::FLAG_READ |
                            base::File::FLAG_DELETE_ON_CLOSE |
                            base::File::FLAG_TEMPORARY));
  if (!file->IsValid()) {
    PLOG(ERROR) << "Failed to create " << path.value();
    file.reset();
  }
  return file.Pass();
}

ScopedTempFile CreateTempPdfFile(
    const scoped_refptr<base::RefCountedMemory>& data,
    scoped_refptr<RefCountedTempDir>* temp_dir) {
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);

  ScopedTempFile pdf_file = CreateTempFile(temp_dir);
  if (!pdf_file ||
      static_cast<int>(data->size()) !=
          pdf_file->WriteAtCurrentPos(data->front_as<char>(), data->size())) {
    pdf_file.reset();
    return pdf_file.Pass();
  }
  pdf_file->Seek(base::File::FROM_BEGIN, 0);
  return pdf_file.Pass();
}

bool LazyEmf::SafePlayback(HDC hdc) const {
  Emf emf;
  bool result = LoadEmf(&emf) && emf.SafePlayback(hdc);
  // TODO(vitalybuka): Fix destruction of metafiles. For some reasons
  // instances of Emf are not deleted. crbug.com/411683
  // It's known that the Emf going to be played just once to a printer. So just
  // release file here.
  Close();
  return result;
}

bool LazyEmf::SaveTo(base::File* file) const {
  Emf emf;
  return LoadEmf(&emf) && emf.SaveTo(file);
}

void LazyEmf::Close() const {
  file_.reset();
  temp_dir_ = NULL;
}

bool LazyEmf::LoadEmf(Emf* emf) const {
  file_->Seek(base::File::FROM_BEGIN, 0);
  int64_t size = file_->GetLength();
  if (size <= 0)
    return false;
  std::vector<char> data(size);
  if (file_->ReadAtCurrentPos(data.data(), data.size()) != size)
    return false;
  return emf->InitFromData(data.data(), data.size());
}

PdfToEmfUtilityProcessHostClient::PdfToEmfUtilityProcessHostClient(
    base::WeakPtr<PdfToEmfConverterImpl> converter,
    const PdfRenderSettings& settings)
    : converter_(converter), settings_(settings) {
}

PdfToEmfUtilityProcessHostClient::~PdfToEmfUtilityProcessHostClient() {
}

void PdfToEmfUtilityProcessHostClient::Start(
    const scoped_refptr<base::RefCountedMemory>& data,
    const PdfToEmfConverter::StartCallback& start_callback) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(BrowserThread::IO,
                            FROM_HERE,
                            base::Bind(&PdfToEmfUtilityProcessHostClient::Start,
                                       this,
                                       data,
                                       start_callback));
    return;
  }
  data_ = data;

  // Store callback before any OnFailed() call to make it called on failure.
  start_callback_ = start_callback;

  // NOTE: This process _must_ be sandboxed, otherwise the pdf dll will load
  // gdiplus.dll, change how rendering happens, and not be able to correctly
  // generate when sent to a metafile DC.
  utility_process_host_ =
      content::UtilityProcessHost::Create(
          this, base::MessageLoop::current()->task_runner())->AsWeakPtr();
  utility_process_host_->SetName(l10n_util::GetStringUTF16(
      IDS_UTILITY_PROCESS_EMF_CONVERTOR_NAME));
  // Should reply with OnProcessStarted().
  Send(new ChromeUtilityMsg_StartupPing);
}

void PdfToEmfUtilityProcessHostClient::OnProcessStarted() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!utility_process_host_)
    return OnFailed();

  scoped_refptr<base::RefCountedMemory> data = data_;
  data_ = NULL;
  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::FILE,
      FROM_HERE,
      base::Bind(&CreateTempPdfFile, data, &temp_dir_),
      base::Bind(&PdfToEmfUtilityProcessHostClient::OnTempPdfReady, this));
}

void PdfToEmfUtilityProcessHostClient::OnTempPdfReady(ScopedTempFile pdf) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!utility_process_host_ || !pdf)
    return OnFailed();
  base::ProcessHandle process = utility_process_host_->GetData().handle;
  // Should reply with OnPageCount().
  Send(new ChromeUtilityMsg_RenderPDFPagesToMetafiles(
      IPC::GetFileHandleForProcess(pdf->GetPlatformFile(), process, false),
      settings_));
}

void PdfToEmfUtilityProcessHostClient::OnPageCount(int page_count) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (start_callback_.is_null())
    return OnFailed();
  BrowserThread::PostTask(BrowserThread::UI,
                          FROM_HERE,
                          base::Bind(&PdfToEmfConverterImpl::RunCallback,
                                     converter_,
                                     base::Bind(start_callback_, page_count)));
  start_callback_.Reset();
}

void PdfToEmfUtilityProcessHostClient::GetPage(
    int page_number,
    const PdfToEmfConverter::GetPageCallback& get_page_callback) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO,
        FROM_HERE,
        base::Bind(&PdfToEmfUtilityProcessHostClient::GetPage,
                   this,
                   page_number,
                   get_page_callback));
    return;
  }

  // Store callback before any OnFailed() call to make it called on failure.
  get_page_callbacks_.push(GetPageCallbackData(page_number, get_page_callback));

  if (!utility_process_host_)
    return OnFailed();

  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::FILE,
      FROM_HERE,
      base::Bind(&CreateTempFile, &temp_dir_),
      base::Bind(&PdfToEmfUtilityProcessHostClient::OnTempEmfReady,
                 this,
                 &get_page_callbacks_.back()));
}

void PdfToEmfUtilityProcessHostClient::OnTempEmfReady(
    GetPageCallbackData* callback_data,
    ScopedTempFile emf) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!utility_process_host_ || !emf)
    return OnFailed();
  base::ProcessHandle process = utility_process_host_->GetData().handle;
  IPC::PlatformFileForTransit transit =
      IPC::GetFileHandleForProcess(emf->GetPlatformFile(), process, false);
  callback_data->set_emf(emf.Pass());
  // Should reply with OnPageDone().
  Send(new ChromeUtilityMsg_RenderPDFPagesToMetafiles_GetPage(
      callback_data->page_number(), transit));
}

void PdfToEmfUtilityProcessHostClient::OnPageDone(bool success,
                                                  float scale_factor) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (get_page_callbacks_.empty())
    return OnFailed();
  GetPageCallbackData& data = get_page_callbacks_.front();
  scoped_ptr<MetafilePlayer> emf;

  if (success) {
    ScopedTempFile temp_emf = data.TakeEmf();
    if (!temp_emf)  // Unexpected message from utility process.
      return OnFailed();
    emf.reset(new LazyEmf(temp_dir_, temp_emf.Pass()));
  }

  BrowserThread::PostTask(BrowserThread::UI,
                          FROM_HERE,
                          base::Bind(&PdfToEmfConverterImpl::RunCallback,
                                     converter_,
                                     base::Bind(data.callback(),
                                                data.page_number(),
                                                scale_factor,
                                                base::Passed(&emf))));
  get_page_callbacks_.pop();
}

void PdfToEmfUtilityProcessHostClient::Stop() {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO,
        FROM_HERE,
        base::Bind(&PdfToEmfUtilityProcessHostClient::Stop, this));
    return;
  }
  Send(new ChromeUtilityMsg_RenderPDFPagesToMetafiles_Stop());
}

void PdfToEmfUtilityProcessHostClient::OnProcessCrashed(int exit_code) {
  OnFailed();
}

void PdfToEmfUtilityProcessHostClient::OnProcessLaunchFailed() {
  OnFailed();
}

bool PdfToEmfUtilityProcessHostClient::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PdfToEmfUtilityProcessHostClient, message)
    IPC_MESSAGE_HANDLER(ChromeUtilityHostMsg_ProcessStarted, OnProcessStarted)
    IPC_MESSAGE_HANDLER(
        ChromeUtilityHostMsg_RenderPDFPagesToMetafiles_PageCount, OnPageCount)
    IPC_MESSAGE_HANDLER(ChromeUtilityHostMsg_RenderPDFPagesToMetafiles_PageDone,
                        OnPageDone)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

bool PdfToEmfUtilityProcessHostClient::Send(IPC::Message* msg) {
  if (utility_process_host_)
    return utility_process_host_->Send(msg);
  delete msg;
  return false;
}

void PdfToEmfUtilityProcessHostClient::OnFailed() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!start_callback_.is_null())
    OnPageCount(0);
  while (!get_page_callbacks_.empty())
    OnPageDone(false, 0.0f);
  utility_process_host_.reset();
}

PdfToEmfConverterImpl::PdfToEmfConverterImpl() : weak_ptr_factory_(this) {
}

PdfToEmfConverterImpl::~PdfToEmfConverterImpl() {
  if (utility_client_.get())
    utility_client_->Stop();
}

void PdfToEmfConverterImpl::Start(
    const scoped_refptr<base::RefCountedMemory>& data,
    const PdfRenderSettings& conversion_settings,
    const StartCallback& start_callback) {
  DCHECK(!utility_client_.get());
  utility_client_ = new PdfToEmfUtilityProcessHostClient(
      weak_ptr_factory_.GetWeakPtr(), conversion_settings);
  utility_client_->Start(data, start_callback);
}

void PdfToEmfConverterImpl::GetPage(int page_number,
                                    const GetPageCallback& get_page_callback) {
  utility_client_->GetPage(page_number, get_page_callback);
}

void PdfToEmfConverterImpl::RunCallback(const base::Closure& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  callback.Run();
}

}  // namespace

PdfToEmfConverter::~PdfToEmfConverter() {
}

// static
scoped_ptr<PdfToEmfConverter> PdfToEmfConverter::CreateDefault() {
  return scoped_ptr<PdfToEmfConverter>(new PdfToEmfConverterImpl());
}

}  // namespace printing
