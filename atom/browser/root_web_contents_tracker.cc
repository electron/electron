#include "root_web_contents_tracker.h"
#include <unordered_set>

namespace atom {

namespace {

std::unordered_set<content::WebContents*> g_root_web_contents;

}  // namespace

RootWebContentsTracker::RootWebContentsTracker(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  g_root_web_contents.insert(web_contents);
}

bool RootWebContentsTracker::IsRootWebContents(
    content::WebContents* web_contents) {
  return g_root_web_contents.find(web_contents) != g_root_web_contents.end();
}

void RootWebContentsTracker::WebContentsDestroyed() {
  g_root_web_contents.erase(web_contents());
  delete this;
}

}  // namespace atom
