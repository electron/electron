// MIT License

// Copyright (c) 2016 Mohammed Boujemaoui Boulaghmoudi

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ATOM_BROWSER_UI_WIN_TOAST_LIB_H_
#define ATOM_BROWSER_UI_WIN_TOAST_LIB_H_

#include <string.h>
#include <winstring.h>

#include <SDKDDKVer.h>
#include <Shobjidl.h>
#include <WinUser.h>

#include <functiondiscoverykeys.h>
#include <propvarutil.h>
#include <roapi.h>
#include <shlobj.h>
#include <strsafe.h>
#include <windows.h>
#include <windows.ui.notifications.h>
#include <wrl/event.h>
#include <wrl/implements.h>

#include <Psapi.h>
#include <iostream>
#include <vector>

using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Details::ComPtrRef;
using Microsoft::WRL::Callback;
using Microsoft::WRL::Implements;
using Microsoft::WRL::RuntimeClassFlags;
using Microsoft::WRL::ClassicCom;
using ABI::Windows::Data::Xml::Dom::IXmlAttribute;
using ABI::Windows::Data::Xml::Dom::IXmlDocument;
using ABI::Windows::Data::Xml::Dom::IXmlElement;
using ABI::Windows::Data::Xml::Dom::IXmlNamedNodeMap;
using ABI::Windows::Data::Xml::Dom::IXmlNode;
using ABI::Windows::Data::Xml::Dom::IXmlNodeList;
using ABI::Windows::Data::Xml::Dom::IXmlText;
using ABI::Windows::UI::Notifications::ToastDismissalReason;
using ABI::Windows::UI::Notifications::ToastTemplateType;
using ABI::Windows::UI::Notifications::IToastNotificationManagerStatics;
using ABI::Windows::UI::Notifications::IToastNotifier;
using ABI::Windows::UI::Notifications::IToastNotificationFactory;
using ABI::Windows::UI::Notifications::IToastNotification;
using ABI::Windows::UI::Notifications::ToastNotification;
using ABI::Windows::UI::Notifications::ToastDismissedEventArgs;
using ABI::Windows::UI::Notifications::IToastDismissedEventArgs;
using ABI::Windows::UI::Notifications::ToastFailedEventArgs;
using ABI::Windows::UI::Notifications::IToastFailedEventArgs;
using ABI::Windows::UI::Notifications::ToastTemplateType_ToastText01;
using ABI::Windows::Foundation::ITypedEventHandler;

#define DEFAULT_SHELL_LINKS_PATH L"\\Microsoft\\Windows\\Start Menu\\Programs\\"
#define DEFAULT_LINK_FORMAT L".lnk"

namespace WinToastLib {
class WinToastStringWrapper {
 public:
  WinToastStringWrapper(_In_reads_(length) PCWSTR stringRef,
                        _In_ UINT32 length) throw();
  explicit WinToastStringWrapper(_In_ const std::wstring& stringRef) throw();
  ~WinToastStringWrapper();
  HSTRING Get() const throw();

 private:
  HSTRING _hstring;
  HSTRING_HEADER _header;
};

class WinToastHandler {
 public:
  enum WinToastDismissalReason {
    UserCanceled = ToastDismissalReason::ToastDismissalReason_UserCanceled,
    ApplicationHidden =
        ToastDismissalReason::ToastDismissalReason_ApplicationHidden,
    TimedOut = ToastDismissalReason::ToastDismissalReason_TimedOut
  };

  virtual void toastActivated() {}
  virtual void toastDismissed(WinToastDismissalReason state) {}
  virtual void toastFailed() {}
};

class WinToastTemplate {
 public:
  enum TextField { FirstLine = 0, SecondLine, ThirdLine, LineCount };

  enum WinToastTemplateType {
    ImageWithOneLine = ToastTemplateType::ToastTemplateType_ToastImageAndText01,
    ImageWithTwoLines =
        ToastTemplateType::ToastTemplateType_ToastImageAndText02,
    ImageWithThreeLines =
        ToastTemplateType::ToastTemplateType_ToastImageAndText03,
    ImageWithFourLines =
        ToastTemplateType::ToastTemplateType_ToastImageAndText04,
    TextOneLine = ToastTemplateType::ToastTemplateType_ToastText01,
    TextTwoLines = ToastTemplateType::ToastTemplateType_ToastText02,
    TextThreeLines = ToastTemplateType::ToastTemplateType_ToastText03,
    TextFourLines = ToastTemplateType::ToastTemplateType_ToastText04,
    WinToastTemplateTypeCount
  };

  explicit WinToastTemplate(
      _In_ const WinToastTemplateType& type = ImageWithTwoLines);
  ~WinToastTemplate();

  int textFieldsCount() const { return _textFields.size(); }
  bool hasImage() const { return _hasImage; }
  std::vector<std::wstring> textFields() const { return _textFields; }
  std::wstring textField(_In_ const TextField& pos) const {
    return _textFields[pos];
  }
  std::wstring imagePath() const { return _imagePath; }
  WinToastTemplateType type() const { return _type; }
  void setTextField(_In_ const std::wstring& txt, _In_ const TextField& pos);
  void setImagePath(_In_ const std::wstring& imgPath);
  void setSilent(bool is_silent);
  bool isSilent() const { return _silent; }

 private:
  static int TextFieldsCount[WinToastTemplateTypeCount];
  bool _hasImage;
  bool _silent = false;
  std::vector<std::wstring> _textFields;
  std::wstring _imagePath;
  WinToastTemplateType _type;
  void initComponentsFromType();
};

class WinToast {
 public:
  static WinToast* instance();
  static bool isCompatible();
  static std::wstring configureAUMI(_In_ const std::wstring& company,
                                    _In_ const std::wstring& name,
                                    _In_ const std::wstring& surname,
                                    _In_ const std::wstring& versionInfo);
  bool initialize();
  bool isInitialized() const { return _isInitialized; }
  bool showToast(_In_ const WinToastTemplate& toast,
                 _In_ WinToastHandler* handler);
  std::wstring appName() const;
  std::wstring appUserModelId() const;
  void setAppUserModelId(_In_ const std::wstring& appName);
  void setAppName(_In_ const std::wstring& appName);

 private:
  bool _isInitialized;
  std::wstring _appName;
  std::wstring _aumi;
  ComPtr<IXmlDocument> _xmlDocument;
  ComPtr<IToastNotificationManagerStatics> _notificationManager;
  ComPtr<IToastNotifier> _notifier;
  ComPtr<IToastNotificationFactory> _notificationFactory;
  ComPtr<IToastNotification> _notification;
  static WinToast* _instance;

  WinToast(void);
  IXmlDocument* xmlDocument() const { return _xmlDocument.Get(); }
  IToastNotifier* notifier() const { return _notifier.Get(); }
  IToastNotificationFactory* notificationFactory() const {
    return _notificationFactory.Get();
  }
  IToastNotificationManagerStatics* notificationManager() const {
    return _notificationManager.Get();
  }
  IToastNotification* notification() const { return _notification.Get(); }

  HRESULT validateShellLink();
  HRESULT createShellLink();
  HRESULT setImageField(_In_ const std::wstring& path);
  HRESULT setTextField(_In_ const std::wstring& text, _In_ int pos);
  bool makeSilent(bool is_silent);
};
}  // namespace WinToastLib

#endif  // ATOM_BROWSER_UI_WIN_TOAST_LIB_H_
