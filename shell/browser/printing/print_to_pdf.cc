// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/printing/print_to_pdf.h"

#include <cmath>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/containers/circular_deque.h"
#include "base/containers/fixed_flat_map.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/memory/ref_counted_memory.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "components/printing/browser/print_to_pdf/pdf_print_result.h"
#include "components/printing/browser/print_to_pdf/pdf_print_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "printing/mojom/print.mojom.h"  // nogncheck
#include "shell/browser/javascript_environment.h"
#include "shell/browser/printing/print_view_manager_electron.h"
#include "shell/common/gin_helper/conversion_error.h"
#include "shell/common/gin_helper/locker.h"
#include "shell/common/gin_helper/options_reader.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_util.h"
#include "third_party/abseil-cpp/absl/types/variant.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-primitive.h"

namespace electron {

namespace {

using gin_helper::ConversionError;
using gin_helper::OptionsReader;

struct PaperSize {
  double width;   // inches
  double height;  // inches
};

// printToPDF's named page sizes, in inches.
constexpr auto kPaperFormats =
    base::MakeFixedFlatMap<std::string_view, PaperSize>({
        {"letter", {8.5, 11}},
        {"legal", {8.5, 14}},
        {"tabloid", {11, 17}},
        {"ledger", {17, 11}},
        {"a0", {33.1, 46.8}},
        {"a1", {23.4, 33.1}},
        {"a2", {16.54, 23.4}},
        {"a3", {11.7, 16.54}},
        {"a4", {8.27, 11.7}},
        {"a5", {5.83, 8.27}},
        {"a6", {4.13, 5.83}},
    });

// Everything printToPDF takes from its options, validated; turned into
// PrintPagesParams once the frame (and so the URL) is known.
// Numbers are optional only so that a non-finite value can fall back to the
// print_to_pdf default, as it did when it was dropped crossing into a
// base::Value.
struct PdfRequest {
  int request_id = 0;
  bool landscape = false;
  bool display_header_footer = false;
  std::string header_template;
  std::string footer_template;
  bool print_background = false;
  std::optional<double> scale = 1.0;
  std::optional<double> paper_width = 8.5;
  std::optional<double> paper_height = 11;
  std::optional<double> margin_top = 0.4;
  std::optional<double> margin_bottom = 0.4;
  std::optional<double> margin_left = 0.4;
  std::optional<double> margin_right = 0.4;
  std::string page_ranges;
  bool prefer_css_page_size = false;
  bool generate_tagged_pdf = false;
  bool generate_document_outline = false;
};

// options.pageSize: a format name (any case) or {width, height} in inches.
bool ReadPageSize(const OptionsReader& options, PaperSize* out) {
  v8::Local<v8::Value> value;
  if (!options.GetValue("pageSize", &value))
    return true;
  ConversionError& error = options.error();
  if (value->IsString()) {
    std::string name = gin::V8ToString(options.isolate(), value);
    auto format = kPaperFormats.find(base::ToLowerASCII(name));
    if (format == kPaperFormats.end()) {
      error.Fail(base::StrCat({"Invalid pageSize ", name}));
      return false;
    }
    *out = format->second;
    return true;
  }
  if (value->IsObject() && !value->IsFunction()) {
    ConversionError unused;
    OptionsReader size(options.isolate(), value.As<v8::Object>(), unused);
    v8::Local<v8::Value> width;
    v8::Local<v8::Value> height;
    if (!size.GetValue("width", &width) || !width->IsNumber() ||
        !size.GetValue("height", &height) || !height->IsNumber()) {
      error.Fail("width and height properties are required for pageSize",
                 ConversionError::Kind::kTypeError);
      return false;
    }
    *out = {width.As<v8::Number>()->Value(), height.As<v8::Number>()->Value()};
    return true;
  }
  error.Fail("pageSize must be a string or an object",
             ConversionError::Kind::kTypeError);
  return false;
}

// The JavaScript `margin !== undefined && !(margin <= limit)` this replaces,
// with `<=`'s coercion (null is 0, BigInt compares exactly, anything
// unconvertible throws); type checking proper comes after.
bool MarginExceeds(const OptionsReader& margins,
                   v8::Local<v8::Value> value,
                   double limit) {
  if (margins.failed() || value.IsEmpty() || value->IsUndefined())
    return false;
  if (value->IsBigInt()) {
    bool lossless;
    int64_t as_int = value.As<v8::BigInt>()->Int64Value(&lossless);
    if (!lossless) {
      // Beyond int64: only its sign matters against a finite limit.
      int sign_bit = 0;
      int word_count = 0;
      value.As<v8::BigInt>()->ToWordsArray(&sign_bit, &word_count, nullptr);
      return sign_bit == 0;
    }
    return !(as_int <= limit);
  }
  double number;
  if (!value->NumberValue(margins.isolate()->GetCurrentContext()).To(&number)) {
    margins.error().Fail("Exception reading margins");
    return true;
  }
  return !(number <= limit);
}

// `const {top, bottom, left, right} = margins`: all four read up front.
bool ReadMargin(const OptionsReader& margins,
                std::string_view key,
                v8::Local<v8::Value>* out) {
  if (margins.failed())
    return false;
  if (!margins.object()
           ->Get(margins.isolate()->GetCurrentContext(),
                 gin::StringToV8(margins.isolate(), key))
           .ToLocal(out)) {
    margins.error().Fail("Exception reading margins");
    return false;
  }
  return true;
}

void DropIfNotFinite(std::optional<double>& value) {
  if (value && !std::isfinite(*value))
    value.reset();
}

// Reads and validates printToPDF's options in the order, and with the
// messages, the API has always reported them.
std::optional<PdfRequest> ReadPdfRequest(v8::Isolate* isolate,
                                         v8::Local<v8::Value> value,
                                         ConversionError& error) {
  PdfRequest request;
  std::optional<OptionsReader> options =
      OptionsReader::Of(isolate, value, "options", error);
  if (!options)
    return std::nullopt;

  std::optional<OptionsReader> margins;
  if (v8::Local<v8::Value> margins_value;
      options->GetValue("margins", &margins_value)) {
    if (!margins_value->IsObject() || margins_value->IsFunction()) {
      error.Fail("margins must be a object", ConversionError::Kind::kTypeError);
      return std::nullopt;
    }
    margins.emplace(isolate, margins_value.As<v8::Object>(), error, "margins");
  }

  PaperSize paper{8.5, 11};
  if (!ReadPageSize(*options, &paper))
    return std::nullopt;
  request.paper_width = paper.width;
  request.paper_height = paper.height;

  if (margins) {
    v8::Local<v8::Value> top, bottom, left, right;
    if (!ReadMargin(*margins, "top", &top) ||
        !ReadMargin(*margins, "bottom", &bottom) ||
        !ReadMargin(*margins, "left", &left) ||
        !ReadMargin(*margins, "right", &right)) {
      return std::nullopt;
    }
    // Both pairs are compared, each pair stopping at its first excess.
    const bool bad_height = MarginExceeds(*margins, top, paper.height) ||
                            MarginExceeds(*margins, bottom, paper.height);
    const bool bad_width = MarginExceeds(*margins, left, paper.width) ||
                           MarginExceeds(*margins, right, paper.width);
    if (error.failed())  // a coercion threw
      return std::nullopt;
    if (bad_height || bad_width) {
      error.Fail("margins must be less than or equal to pageSize");
      return std::nullopt;
    }
  }

  options->Get("landscape", &request.landscape);
  options->Get("displayHeaderFooter", &request.display_header_footer);
  options->Get("headerTemplate", &request.header_template);
  options->Get("footerTemplate", &request.footer_template);
  options->Get("printBackground", &request.print_background);
  options->Get("scale", &*request.scale);
  if (margins) {
    margins->Get("top", &*request.margin_top);
    margins->Get("bottom", &*request.margin_bottom);
    margins->Get("left", &*request.margin_left);
    margins->Get("right", &*request.margin_right);
  }
  options->Get("pageRanges", &request.page_ranges);
  options->Get("preferCSSPageSize", &request.prefer_css_page_size);
  options->Get("generateTaggedPDF", &request.generate_tagged_pdf);
  options->Get("generateDocumentOutline", &request.generate_document_outline);
  if (error.failed())
    return std::nullopt;
  for (std::optional<double>* number :
       {&request.scale, &request.paper_width, &request.paper_height,
        &request.margin_top, &request.margin_bottom, &request.margin_left,
        &request.margin_right}) {
    DropIfNotFinite(*number);
  }

  static int next_request_id = 0;
  request.request_id = ++next_request_id;
  return request;
}

class PdfQueue;

// One queued printToPDF call.
struct PdfJob {
  PdfJob(v8::Isolate* isolate,
         PrintToPDFFrame frame,
         PrintToPDFFrameGone frame_gone)
      : promise(isolate),
        frame(std::move(frame)),
        frame_gone_message(frame_gone.message),
        frame_gone_type_error(frame_gone.type_error) {}
  gin_helper::Promise<v8::Local<v8::Value>> promise;
  PrintToPDFFrame frame;
  std::string frame_gone_message;
  bool frame_gone_type_error;
  PdfRequest request;
};

// Serialises printToPDF jobs per frame tree. Jobs in different trees run
// concurrently.
class PdfQueue {
 public:
  static PdfQueue& Get() {
    static base::NoDestructor<PdfQueue> queue;
    return *queue;
  }

  void Add(int frame_tree, std::unique_ptr<PdfJob> job) {
    base::circular_deque<std::unique_ptr<PdfJob>>& jobs = queues_[frame_tree];
    jobs.push_back(std::move(job));
    if (jobs.size() == 1)
      PostStart(frame_tree);
  }

 private:
  friend class base::NoDestructor<PdfQueue>;
  PdfQueue() = default;

  // Settles the front job of |frame_tree| when run or, if the print manager
  // drops its callback (the WebContents went away), when destroyed; either
  // way the next job then starts.
  class Completion {
   public:
    Completion(PdfQueue* queue, int frame_tree)
        : queue_(queue), frame_tree_(frame_tree) {}
    ~Completion() {
      // Un-run: the frame's WebContents is being torn down; move on once
      // that is over.
      if (queue_) {
        base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
            FROM_HERE,
            base::BindOnce(&PdfQueue::Finish, base::Unretained(queue_.get()),
                           frame_tree_, std::nullopt, nullptr));
      }
    }
    void Run(print_to_pdf::PdfPrintResult result,
             scoped_refptr<base::RefCountedMemory> data) {
      std::exchange(queue_, nullptr)->Finish(frame_tree_, result, data);
    }

   private:
    raw_ptr<PdfQueue> queue_;  // process-lifetime
    int frame_tree_;
  };

  void Start(int frame_tree) {
    PdfJob& job = *queues_[frame_tree].front();
    content::RenderFrameHost* rfh = job.frame.Run();
    if (!rfh) {
      v8::Isolate* isolate = job.promise.isolate();
      v8::HandleScope handle_scope(isolate);
      v8::Local<v8::String> message =
          gin::StringToV8(isolate, job.frame_gone_message);
      job.promise.Reject(job.frame_gone_type_error
                             ? v8::Exception::TypeError(message)
                             : v8::Exception::Error(message));
      return Pop(frame_tree);
    }
    const PdfRequest& r = job.request;
    absl::variant<printing::mojom::PrintPagesParamsPtr, std::string> params =
        print_to_pdf::GetPrintPagesParams(
            rfh->GetLastCommittedURL(), r.landscape, r.display_header_footer,
            r.print_background, r.scale, r.paper_width, r.paper_height,
            r.margin_top, r.margin_bottom, r.margin_left, r.margin_right,
            r.header_template, r.footer_template, r.prefer_css_page_size,
            r.generate_tagged_pdf, r.generate_document_outline);
    if (absl::holds_alternative<std::string>(params)) {
      job.promise.RejectWithErrorMessage(base::StrCat(
          {"Invalid print parameters: ", absl::get<std::string>(params)}));
      return Pop(frame_tree);
    }
    auto* manager = PrintViewManagerElectron::FromWebContents(
        content::WebContents::FromRenderFrameHost(rfh));
    if (!manager) {
      job.promise.RejectWithErrorMessage("Failed to find print manager");
      return Pop(frame_tree);
    }
    // PdfPrintJob reports this too, but only after the print manager has
    // CHECKed it.
    if (!rfh->IsRenderFrameLive()) {
      job.promise.RejectWithErrorMessage(
          base::StrCat({"Failed to generate PDF: ",
                        print_to_pdf::PdfPrintResultToString(
                            print_to_pdf::PdfPrintResult::kPrintFailure)}));
      return Pop(frame_tree);
    }
    printing::mojom::PrintPagesParamsPtr pages =
        std::move(absl::get<printing::mojom::PrintPagesParamsPtr>(params));
    pages->params->document_cookie = r.request_id;
    manager->PrintToPdf(
        rfh, r.page_ranges, std::move(pages),
        base::BindOnce(&Completion::Run,
                       std::make_unique<Completion>(this, frame_tree)));
  }

  void Finish(int frame_tree,
              std::optional<print_to_pdf::PdfPrintResult> result,
              scoped_refptr<base::RefCountedMemory> data) {
    PdfJob& job = *queues_[frame_tree].front();
    if (!result) {
      // Dropped un-run: leave the promise unsettled, as before.
    } else if (*result != print_to_pdf::PdfPrintResult::kPrintSuccess) {
      job.promise.RejectWithErrorMessage(
          base::StrCat({"Failed to generate PDF: ",
                        print_to_pdf::PdfPrintResultToString(*result)}));
    } else {
      v8::Isolate* isolate = job.promise.isolate();
      gin_helper::Locker locker(isolate);
      v8::HandleScope handle_scope(isolate);
      v8::Context::Scope context_scope(job.promise.GetContext());
      job.promise.Resolve(
          electron::Buffer::Copy(isolate, *data).ToLocalChecked());
    }
    Pop(frame_tree);
  }

  void Pop(int frame_tree) {
    auto it = queues_.find(frame_tree);
    it->second.pop_front();
    if (it->second.empty())
      queues_.erase(it);
    else
      PostStart(frame_tree);
  }

  // Jobs start from a fresh task, never from inside printToPDF() or a
  // previous job's completion (which can arrive mid-teardown of a frame), as
  // they did when a promise reaction started them.
  void PostStart(int frame_tree) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&PdfQueue::Start, base::Unretained(this), frame_tree));
  }

  std::map<int, base::circular_deque<std::unique_ptr<PdfJob>>> queues_;
};

}  // namespace

v8::Local<v8::Promise> PrintToPDF(v8::Isolate* isolate,
                                  int frame_tree,
                                  PrintToPDFFrame frame,
                                  PrintToPDFFrameGone frame_gone,
                                  v8::Local<v8::Value> options) {
  auto job = std::make_unique<PdfJob>(isolate, std::move(frame), frame_gone);
  v8::Local<v8::Promise> handle = job->promise.GetHandle();
  ConversionError error;
  // Reading the options can run getters and coercions that throw; that is a
  // rejection too, not an exception from printToPDF().
  v8::TryCatch try_catch(isolate);
  std::optional<PdfRequest> request = ReadPdfRequest(isolate, options, error);
  if (try_catch.HasCaught()) {
    v8::Local<v8::Value> exception = try_catch.Exception();
    try_catch.Reset();
    job->promise.Reject(exception);
    return handle;
  }
  if (!request) {
    job->promise.Reject(error.ToException(isolate));
    return handle;
  }
  job->request = std::move(*request);
  PdfQueue::Get().Add(frame_tree, std::move(job));
  return handle;
}

}  // namespace electron
