// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_DATA_PIPE_HOLDER_H_
#define SHELL_BROWSER_API_ATOM_API_DATA_PIPE_HOLDER_H_

#include <string>

#include "gin/handle.h"
#include "gin/wrappable.h"
#include "services/network/public/cpp/data_element.h"
#include "services/network/public/mojom/data_pipe_getter.mojom.h"

namespace electron {

namespace api {

// Retains reference to the data pipe.
class DataPipeHolder : public gin::Wrappable<DataPipeHolder> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static gin::Handle<DataPipeHolder> Create(
      v8::Isolate* isolate,
      const network::DataElement& element);
  static gin::Handle<DataPipeHolder> From(v8::Isolate* isolate,
                                          const std::string& id);

  network::mojom::DataPipeGetterPtr Release();

  const std::string& id() const { return id_; }

 private:
  explicit DataPipeHolder(const network::DataElement& element);
  ~DataPipeHolder() override;

  std::string id_;
  network::mojom::DataPipeGetterPtr data_pipe_;

  DISALLOW_COPY_AND_ASSIGN(DataPipeHolder);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ATOM_API_DATA_PIPE_HOLDER_H_
