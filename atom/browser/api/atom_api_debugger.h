// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_DEBUGGER_H_
#define ATOM_BROWSER_API_ATOM_API_DEBUGGER_H_

#include <map>
#include <string>

#include "atom/browser/api/trackable_object.h"
#include "base/callback.h"
#include "base/values.h"
#include "content/public/browser/devtools_agent_host_client.h"
#include "native_mate/handle.h"

namespace content {
class DevToolsAgentHost;
class WebContents;
}

namespace mate {
class Arguments;
}

namespace atom {

namespace api {

class Debugger: public mate::TrackableObject<Debugger>,
                public content::DevToolsAgentHostClient {
 public:
  using SendCommandCallback =
      base::Callback<void(const base::DictionaryValue&,
                          const base::DictionaryValue&)>;

  static mate::Handle<Debugger> Create(
      v8::Isolate* isolate, content::WebContents* web_contents);

  // mate::TrackableObject:
  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  Debugger(v8::Isolate* isolate, content::WebContents* web_contents);
  ~Debugger() override;

  // content::DevToolsAgentHostClient:
  void AgentHostClosed(content::DevToolsAgentHost* agent_host,
                       bool replaced_with_another_client) override;
  void DispatchProtocolMessage(content::DevToolsAgentHost* agent_host,
                               const std::string& message) override;

 private:
  using PendingRequestMap = std::map<int, SendCommandCallback>;

  void Attach(mate::Arguments* args);
  bool IsAttached();
  void Detach();
  void SendCommand(mate::Arguments* args);

  content::WebContents* web_contents_;  // Weak Reference.
  scoped_refptr<content::DevToolsAgentHost> agent_host_;

  PendingRequestMap pending_requests_;
  int previous_request_id_;

  DISALLOW_COPY_AND_ASSIGN(Debugger);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_DEBUGGER_H_
