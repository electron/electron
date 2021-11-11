export const enum IPC_MESSAGES {
  BROWSER_CLIPBOARD_SYNC = 'BROWSER_CLIPBOARD_SYNC',
  BROWSER_GET_LAST_WEB_PREFERENCES = 'BROWSER_GET_LAST_WEB_PREFERENCES',
  BROWSER_PRELOAD_ERROR = 'BROWSER_PRELOAD_ERROR',
  BROWSER_SANDBOX_LOAD = 'BROWSER_SANDBOX_LOAD',
  BROWSER_WINDOW_CLOSE = 'BROWSER_WINDOW_CLOSE',
  BROWSER_GET_PROCESS_MEMORY_INFO = 'BROWSER_GET_PROCESS_MEMORY_INFO',

  GUEST_INSTANCE_VISIBILITY_CHANGE = 'GUEST_INSTANCE_VISIBILITY_CHANGE',

  GUEST_VIEW_INTERNAL_DESTROY_GUEST = 'GUEST_VIEW_INTERNAL_DESTROY_GUEST',
  GUEST_VIEW_INTERNAL_DISPATCH_EVENT = 'GUEST_VIEW_INTERNAL_DISPATCH_EVENT',

  GUEST_VIEW_MANAGER_CREATE_AND_ATTACH_GUEST = 'GUEST_VIEW_MANAGER_CREATE_AND_ATTACH_GUEST',
  GUEST_VIEW_MANAGER_DETACH_GUEST = 'GUEST_VIEW_MANAGER_DETACH_GUEST',
  GUEST_VIEW_MANAGER_FOCUS_CHANGE = 'GUEST_VIEW_MANAGER_FOCUS_CHANGE',
  GUEST_VIEW_MANAGER_CALL = 'GUEST_VIEW_MANAGER_CALL',
  GUEST_VIEW_MANAGER_PROPERTY_GET = 'GUEST_VIEW_MANAGER_PROPERTY_GET',
  GUEST_VIEW_MANAGER_PROPERTY_SET = 'GUEST_VIEW_MANAGER_PROPERTY_SET',

  GUEST_WINDOW_MANAGER_WINDOW_OPEN = 'GUEST_WINDOW_MANAGER_WINDOW_OPEN',
  GUEST_WINDOW_MANAGER_WINDOW_CLOSED = 'GUEST_WINDOW_MANAGER_WINDOW_CLOSED',
  GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE = 'GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE',
  GUEST_WINDOW_MANAGER_WINDOW_METHOD = 'GUEST_WINDOW_MANAGER_WINDOW_METHOD',
  GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD = 'GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD',
  GUEST_WINDOW_POSTMESSAGE = 'GUEST_WINDOW_POSTMESSAGE',

  RENDERER_WEB_FRAME_METHOD = 'RENDERER_WEB_FRAME_METHOD',

  INSPECTOR_CONFIRM = 'INSPECTOR_CONFIRM',
  INSPECTOR_CONTEXT_MENU = 'INSPECTOR_CONTEXT_MENU',
  INSPECTOR_SELECT_FILE = 'INSPECTOR_SELECT_FILE',

  DESKTOP_CAPTURER_GET_SOURCES = 'DESKTOP_CAPTURER_GET_SOURCES',
}
