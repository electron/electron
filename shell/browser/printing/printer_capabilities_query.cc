// Copyright (c) 2026 Electron contributors.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/printing/printer_capabilities_query.h"

#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "base/process/process.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/service_process_host.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/result_codes.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "shell/services/printing/public/mojom/printer_capabilities_service.mojom.h"

namespace electron {

namespace {

constexpr auto kQueryTimeout = base::Seconds(30);
using QueryResult = std::pair<std::string, base::DictValue>;
using QueryCallback = base::OnceCallback<void(QueryResult)>;

#if DCHECK_IS_ON()
struct TestConfig {
  std::string behavior;
  base::TimeDelta timeout;
};

std::optional<TestConfig>& NextTestConfig() {
  static base::NoDestructor<std::optional<TestConfig>> config;
  return *config;
}
#endif

// Owns itself until a reply, disconnect, or deadline. All callbacks use weak
// pointers; each path settles the promise exactly once and releases the remote.
class PrinterCapabilitiesQuery {
 public:
  explicit PrinterCapabilitiesQuery(QueryCallback callback)
      : callback_(std::move(callback)) {}

  void Start(const std::string& printer_name) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    auto options =
        content::ServiceProcessHost::Options()
            .WithDisplayName("Printer capabilities")
            .WithExtraCommandLineSwitches({switches::kMessageLoopTypeUi})
            .WithProcessCallback(
                base::BindOnce(&PrinterCapabilitiesQuery::OnProcessLaunched,
                               weak_factory_.GetWeakPtr()))
            .Pass();
    auto timeout = kQueryTimeout;
#if DCHECK_IS_ON()
    if (auto config = std::exchange(NextTestConfig(), std::nullopt)) {
      options.WithExtraCommandLineSwitchKeyValues(
          {{"electron-printer-capabilities-test-behavior", config->behavior}});
      timeout = config->timeout;
    }
#endif
    content::ServiceProcessHost::Launch(remote_.BindNewPipeAndPassReceiver(),
                                        std::move(options));
    remote_.set_disconnect_handler(base::BindOnce(
        &PrinterCapabilitiesQuery::Finish, weak_factory_.GetWeakPtr(),
        "Printer capability lookup process exited unexpectedly",
        base::DictValue(), false));
    timer_.Start(FROM_HERE, timeout,
                 base::BindOnce(&PrinterCapabilitiesQuery::Finish,
                                weak_factory_.GetWeakPtr(),
                                "Printer capability lookup timed out",
                                base::DictValue(), true));
    remote_->GetCapabilities(printer_name,
                             base::BindOnce(&PrinterCapabilitiesQuery::OnResult,
                                            weak_factory_.GetWeakPtr()));
  }

 private:
  static void OnProcessLaunched(base::WeakPtr<PrinterCapabilitiesQuery> query,
                                const base::Process& process) {
    // A deadline can expire before process startup completes. Do not leave
    // a late-starting helper alive after its request has already finished.
    if (!query) {
      process.Terminate(content::RESULT_CODE_NORMAL_EXIT, false);
      return;
    }
    query->process_ = process.Duplicate();
  }

  void OnResult(const std::string& error, base::DictValue capabilities) {
    Finish(error, std::move(capabilities), false);
  }

  void Finish(const std::string& error,
              base::DictValue capabilities,
              bool terminate) {
    auto callback = std::move(callback_);
    weak_factory_.InvalidateWeakPtrs();
    timer_.Stop();
    remote_.reset();
    if (terminate && process_.IsValid())
      process_.Terminate(content::RESULT_CODE_NORMAL_EXIT, false);
    QueryResult result(error, std::move(capabilities));
    delete this;
    std::move(callback).Run(std::move(result));
  }

  QueryCallback callback_;
  mojo::Remote<mojom::PrinterCapabilitiesService> remote_;
  base::Process process_;
  base::OneShotTimer timer_;
  base::WeakPtrFactory<PrinterCapabilitiesQuery> weak_factory_{this};
};

}  // namespace

void QueryPrinterCapabilities(const std::string& printer_name,
                              QueryCallback callback) {
  (new PrinterCapabilitiesQuery(std::move(callback)))->Start(printer_name);
}

#if DCHECK_IS_ON()
bool SetNextPrinterCapabilitiesQueryForTesting(const std::string& behavior,
                                               int timeout_ms) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (behavior.empty()) {
    NextTestConfig().reset();
    return true;
  }
  if ((behavior != "block" && behavior != "crash" && behavior != "success") ||
      timeout_ms <= 0 || timeout_ms > 30000) {
    return false;
  }
  NextTestConfig() = TestConfig{behavior, base::Milliseconds(timeout_ms)};
  return true;
}
#endif

}  // namespace electron
