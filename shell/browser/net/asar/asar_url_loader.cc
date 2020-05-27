// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/asar/asar_url_loader.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "content/public/browser/file_url_loader.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/data_pipe_producer.h"
#include "mojo/public/cpp/system/file_data_source.h"
#include "net/base/filename_util.h"
#include "net/base/mime_sniffer.h"
#include "net/base/mime_util.h"
#include "net/http/http_byte_range.h"
#include "net/http/http_util.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "shell/common/asar/archive.h"
#include "shell/common/asar/asar_util.h"

namespace asar {

namespace {

net::Error ConvertMojoResultToNetError(MojoResult result) {
  switch (result) {
    case MOJO_RESULT_OK:
      return net::OK;
    case MOJO_RESULT_NOT_FOUND:
      return net::ERR_FILE_NOT_FOUND;
    case MOJO_RESULT_PERMISSION_DENIED:
      return net::ERR_ACCESS_DENIED;
    case MOJO_RESULT_RESOURCE_EXHAUSTED:
      return net::ERR_INSUFFICIENT_RESOURCES;
    case MOJO_RESULT_ABORTED:
      return net::ERR_ABORTED;
    default:
      return net::ERR_FAILED;
  }
}

constexpr size_t kDefaultFileUrlPipeSize = 65536;

// Because this makes things simpler.
static_assert(kDefaultFileUrlPipeSize >= net::kMaxBytesToSniff,
              "Default file data pipe size must be at least as large as a MIME-"
              "type sniffing buffer.");

// Modified from the |FileURLLoader| in |file_url_loader_factory.cc|, to serve
// asar files instead of normal files.
class AsarURLLoader : public network::mojom::URLLoader {
 public:
  static void CreateAndStart(
      const network::ResourceRequest& request,
      network::mojom::URLLoaderRequest loader,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      scoped_refptr<net::HttpResponseHeaders> extra_response_headers) {
    // Owns itself. Will live as long as its URLLoader and URLLoaderClientPtr
    // bindings are alive - essentially until either the client gives up or all
    // file data has been sent to it.
    auto* asar_url_loader = new AsarURLLoader;
    asar_url_loader->Start(request, std::move(loader), std::move(client),
                           std::move(extra_response_headers));
  }

  // network::mojom::URLLoader:
  void FollowRedirect(
      const std::vector<std::string>& removed_headers,
      const net::HttpRequestHeaders& modified_headers,
      const net::HttpRequestHeaders& modified_cors_exempt_headers,
      const base::Optional<GURL>& new_url) override {}
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {}
  void PauseReadingBodyFromNet() override {}
  void ResumeReadingBodyFromNet() override {}

 private:
  AsarURLLoader() {}
  ~AsarURLLoader() override = default;

  void Start(const network::ResourceRequest& request,
             mojo::PendingReceiver<network::mojom::URLLoader> loader,
             mojo::PendingRemote<network::mojom::URLLoaderClient> client,
             scoped_refptr<net::HttpResponseHeaders> extra_response_headers) {
    auto head = network::mojom::URLResponseHead::New();
    head->request_start = base::TimeTicks::Now();
    head->response_start = base::TimeTicks::Now();
    head->headers = extra_response_headers;

    base::FilePath path;
    if (!net::FileURLToFilePath(request.url, &path)) {
      mojo::Remote<network::mojom::URLLoaderClient> client_remote(
          std::move(client));
      client_remote->OnComplete(
          network::URLLoaderCompletionStatus(net::ERR_FAILED));
      MaybeDeleteSelf();
      return;
    }

    // Determine whether it is an asar file.
    base::FilePath asar_path, relative_path;
    if (!GetAsarArchivePath(path, &asar_path, &relative_path)) {
      content::CreateFileURLLoaderBypassingSecurityChecks(
          request, std::move(loader), std::move(client), nullptr, false,
          extra_response_headers);
      MaybeDeleteSelf();
      return;
    }

    client_.Bind(std::move(client));
    receiver_.Bind(std::move(loader));
    receiver_.set_disconnect_handler(base::BindOnce(
        &AsarURLLoader::OnConnectionError, base::Unretained(this)));

    // Parse asar archive.
    std::shared_ptr<Archive> archive = GetOrCreateAsarArchive(asar_path);
    Archive::FileInfo info;
    if (!archive || !archive->GetFileInfo(relative_path, &info)) {
      OnClientComplete(net::ERR_FILE_NOT_FOUND);
      return;
    }

    // For unpacked path, read like normal file.
    base::FilePath real_path;
    if (info.unpacked) {
      archive->CopyFileOut(relative_path, &real_path);
      info.offset = 0;
    }

    mojo::DataPipe pipe(kDefaultFileUrlPipeSize);
    if (!pipe.consumer_handle.is_valid()) {
      OnClientComplete(net::ERR_FAILED);
      return;
    }

    // Note that while the |Archive| already opens a |base::File|, we still need
    // to create a new |base::File| here, as it might be accessed by multiple
    // requests at the same time.
    base::File file(info.unpacked ? real_path : archive->path(),
                    base::File::FLAG_OPEN | base::File::FLAG_READ);
    auto file_data_source =
        std::make_unique<mojo::FileDataSource>(std::move(file));
    mojo::DataPipeProducer::DataSource* data_source = file_data_source.get();

    std::vector<char> initial_read_buffer(net::kMaxBytesToSniff);
    auto read_result =
        data_source->Read(info.offset, base::span<char>(initial_read_buffer));
    if (read_result.result != MOJO_RESULT_OK) {
      OnClientComplete(ConvertMojoResultToNetError(read_result.result));
      return;
    }

    std::string range_header;
    net::HttpByteRange byte_range;
    if (request.headers.GetHeader(net::HttpRequestHeaders::kRange,
                                  &range_header)) {
      // Handle a simple Range header for a single range.
      std::vector<net::HttpByteRange> ranges;
      bool fail = false;
      if (net::HttpUtil::ParseRangeHeader(range_header, &ranges) &&
          ranges.size() == 1) {
        byte_range = ranges[0];
        if (!byte_range.ComputeBounds(info.size))
          fail = true;
      } else {
        fail = true;
      }

      if (fail) {
        OnClientComplete(net::ERR_REQUEST_RANGE_NOT_SATISFIABLE);
        return;
      }
    }

    uint64_t first_byte_to_send = 0;
    uint64_t total_bytes_to_send = info.size;

    if (byte_range.IsValid()) {
      first_byte_to_send = byte_range.first_byte_position();
      total_bytes_to_send =
          byte_range.last_byte_position() - first_byte_to_send + 1;
    }

    total_bytes_written_ = total_bytes_to_send;

    head->content_length = base::saturated_cast<int64_t>(total_bytes_to_send);

    if (first_byte_to_send < read_result.bytes_read) {
      // Write any data we read for MIME sniffing, constraining by range where
      // applicable. This will always fit in the pipe (see assertion near
      // |kDefaultFileUrlPipeSize| definition).
      uint32_t write_size = std::min(
          static_cast<uint32_t>(read_result.bytes_read - first_byte_to_send),
          static_cast<uint32_t>(total_bytes_to_send));
      const uint32_t expected_write_size = write_size;
      MojoResult result = pipe.producer_handle->WriteData(
          &initial_read_buffer[first_byte_to_send], &write_size,
          MOJO_WRITE_DATA_FLAG_NONE);
      if (result != MOJO_RESULT_OK || write_size != expected_write_size) {
        OnFileWritten(result);
        return;
      }

      // Discount the bytes we just sent from the total range.
      first_byte_to_send = read_result.bytes_read;
      total_bytes_to_send -= write_size;
    }

    if (!net::GetMimeTypeFromFile(path, &head->mime_type)) {
      std::string new_type;
      net::SniffMimeType(initial_read_buffer.data(), read_result.bytes_read,
                         request.url, head->mime_type,
                         net::ForceSniffFileUrlsForHtml::kDisabled, &new_type);
      head->mime_type.assign(new_type);
      head->did_mime_sniff = true;
    }
    if (head->headers) {
      head->headers->AddHeader(net::HttpRequestHeaders::kContentType,
                               head->mime_type.c_str());
    }
    client_->OnReceiveResponse(std::move(head));
    client_->OnStartLoadingResponseBody(std::move(pipe.consumer_handle));

    if (total_bytes_to_send == 0) {
      // There's definitely no more data, so we're already done.
      OnFileWritten(MOJO_RESULT_OK);
      return;
    }

    // In case of a range request, seek to the appropriate position before
    // sending the remaining bytes asynchronously. Under normal conditions
    // (i.e., no range request) this Seek is effectively a no-op.
    //
    // Note that in Electron we also need to add file offset.
    file_data_source->SetRange(
        first_byte_to_send + info.offset,
        first_byte_to_send + info.offset + total_bytes_to_send);

    data_producer_ = std::make_unique<mojo::DataPipeProducer>(
        std::move(pipe.producer_handle));
    data_producer_->Write(
        std::move(file_data_source),
        base::BindOnce(&AsarURLLoader::OnFileWritten, base::Unretained(this)));
  }

  void OnConnectionError() {
    receiver_.reset();
    MaybeDeleteSelf();
  }

  void OnClientComplete(net::Error net_error) {
    client_->OnComplete(network::URLLoaderCompletionStatus(net_error));
    client_.reset();
    MaybeDeleteSelf();
  }

  void MaybeDeleteSelf() {
    if (!receiver_.is_bound() && !client_.is_bound())
      delete this;
  }

  void OnFileWritten(MojoResult result) {
    // All the data has been written now. Close the data pipe. The consumer will
    // be notified that there will be no more data to read from now.
    data_producer_.reset();

    if (result == MOJO_RESULT_OK) {
      network::URLLoaderCompletionStatus status(net::OK);
      status.encoded_data_length = total_bytes_written_;
      status.encoded_body_length = total_bytes_written_;
      status.decoded_body_length = total_bytes_written_;
      client_->OnComplete(status);
    } else {
      client_->OnComplete(network::URLLoaderCompletionStatus(net::ERR_FAILED));
    }
    client_.reset();
    MaybeDeleteSelf();
  }

  std::unique_ptr<mojo::DataPipeProducer> data_producer_;
  mojo::Receiver<network::mojom::URLLoader> receiver_{this};
  mojo::Remote<network::mojom::URLLoaderClient> client_;

  // In case of successful loads, this holds the total number of bytes written
  // to the response (this may be smaller than the total size of the file when
  // a byte range was requested).
  // It is used to set some of the URLLoaderCompletionStatus data passed back
  // to the URLLoaderClients (eg SimpleURLLoader).
  size_t total_bytes_written_ = 0;

  DISALLOW_COPY_AND_ASSIGN(AsarURLLoader);
};

}  // namespace

void CreateAsarURLLoader(
    const network::ResourceRequest& request,
    network::mojom::URLLoaderRequest loader,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    scoped_refptr<net::HttpResponseHeaders> extra_response_headers) {
  auto task_runner = base::ThreadPool::CreateSequencedTaskRunner(
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&AsarURLLoader::CreateAndStart, request, std::move(loader),
                     std::move(client), std::move(extra_response_headers)));
}

}  // namespace asar
