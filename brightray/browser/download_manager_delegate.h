#ifndef BRIGHTRAY_BROWSER_DOWNLOAD_MANAGER_DELEGATE_H_
#define BRIGHTRAY_BROWSER_DOWNLOAD_MANAGER_DELEGATE_H_

#include "content/public/browser/download_manager_delegate.h"

namespace brightray {

class DownloadManagerDelegate : public content::DownloadManagerDelegate {
 public:
  DownloadManagerDelegate();
  ~DownloadManagerDelegate();

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadManagerDelegate);
};

}

#endif
