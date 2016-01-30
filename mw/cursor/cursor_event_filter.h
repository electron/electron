#ifndef MW_CURSOR_EVENT_H_
#define MW_CURSOR_EVENT_H_

#include "content/common/cursors/webcursor.h"
#include "ipc/ipc_message_macros.h"

#define IPC_MESSAGE_HANDLER_CODE(msg_class, member_func, code) \
  IPC_MESSAGE_FORWARD_CODE(msg_class, this, \
    _IpcMessageHandlerClass::member_func, code)

#define IPC_MESSAGE_FORWARD_CODE(msg_class, obj, member_func, code)            \
    case msg_class::ID: {                                                      \
        TRACK_RUN_IN_THIS_SCOPED_REGION(member_func);                          \
        if (!msg_class::Dispatch(&ipc_message__, obj, this, param__,           \
                                 &member_func))                                \
          ipc_message__.set_dispatch_error();                                  \
          code;                                                                \
      }                                                                        \
      break;

namespace atom {

class CursorChangeEvent {
 public:
  static std::string toString(const content::WebCursor& cursor);
};

}

#endif  // MW_CURSOR_EVENT_H_
