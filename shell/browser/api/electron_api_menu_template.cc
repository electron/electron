// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

// Menu.buildFromTemplate().

#include <functional>
#include <map>
#include <set>
#include <string_view>
#include <utility>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/stack_allocated.h"
#include "gin/converter.h"
#include "shell/browser/api/electron_api_menu.h"
#include "shell/browser/api/electron_api_menu_item.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "v8/include/v8.h"

namespace electron::api {

namespace {

struct TemplateEntry {
  STACK_ALLOCATED();

 public:
  // A separator with no ordering constraints splits groups.
  bool is_plain_separator() const {
    return item->type() == MenuItem::Type::kSeparator && !constrained;
  }
  bool is_separator() const {
    return item->type() == MenuItem::Type::kSeparator;
  }

  MenuItem* item = nullptr;
  // Keeps |item| alive until it is inserted.
  v8::Local<v8::Object> wrapper;
  // Ids are compared with ===.
  v8::Local<v8::Value> id;
  std::vector<v8::Local<v8::Value>> before, after, before_group, after_group;
  bool constrained = false;  // any of before/after/...GroupContaining set
};

// Array.isArray()
bool IsArray(v8::Local<v8::Value> value) {
  while (value->IsProxy())
    value = value.As<v8::Proxy>()->GetTarget();
  return value->IsArray();
}

uint32_t ArrayLength(v8::Isolate* isolate, v8::Local<v8::Object> array) {
  uint32_t length = 0;
  gin_helper::Dictionary(isolate, array).Get("length", &length);
  return length;
}

std::vector<v8::Local<v8::Value>> IdList(v8::Isolate* isolate,
                                         const gin_helper::Dictionary& dict,
                                         std::string_view key,
                                         bool* constrained) {
  std::vector<v8::Local<v8::Value>> out;
  v8::Local<v8::Value> value;
  if (!dict.Get(key, &value))
    return out;
  if (value->BooleanValue(isolate))
    *constrained = true;
  if (!IsArray(value))
    return out;
  v8::Local<v8::Object> array = value.As<v8::Object>();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  for (uint32_t i = 0, n = ArrayLength(isolate, array); i < n; ++i) {
    v8::Local<v8::Value> element;
    if (array->Get(context, i).ToLocal(&element))
      out.push_back(element);
  }
  return out;
}

bool SameId(v8::Local<v8::Value> a, v8::Local<v8::Value> b) {
  return !a.IsEmpty() && !b.IsEmpty() && a->StrictEquals(b);
}

// The ordering constraints are read off the item, where the template's extra
// fields were copied to.
void ReadConstraints(v8::Isolate* isolate, TemplateEntry* e) {
  gin_helper::Dictionary dict(isolate, e->wrapper);
  v8::Local<v8::Value> id;
  if (dict.Get("id", &id) && !id->IsUndefined())
    e->id = id;
  e->before = IdList(isolate, dict, "before", &e->constrained);
  e->after = IdList(isolate, dict, "after", &e->constrained);
  e->before_group =
      IdList(isolate, dict, "beforeGroupContaining", &e->constrained);
  e->after_group =
      IdList(isolate, dict, "afterGroupContaining", &e->constrained);
}

using Group = std::vector<const TemplateEntry*>;

int IndexOfGroupContainingId(const std::vector<Group>& groups,
                             v8::Local<v8::Value> id,
                             const Group* ignore) {
  for (size_t i = 0; i < groups.size(); ++i) {
    if (&groups[i] == ignore)
      continue;
    for (const TemplateEntry* e : groups[i]) {
      if (SameId(e->id, id))
        return static_cast<int>(i);
    }
  }
  return -1;
}

// Sort nodes topologically, depth first; cycles are broken.
std::vector<int> SortTopologically(
    int count,
    const std::map<int, std::vector<int>>& edges) {
  std::vector<int> sorted;
  std::set<int> marked;
  std::function<void(int)> visit = [&](int mark) {
    if (!marked.insert(mark).second)
      return;
    if (auto it = edges.find(mark); it != edges.end()) {
      for (int edge : it->second)
        visit(edge);
    }
    sorted.push_back(mark);
  };
  for (int i = 0; i < count; ++i)
    visit(i);
  return sorted;
}

bool AttemptToMergeAGroup(std::vector<Group>& groups) {
  for (size_t i = 0; i < groups.size(); ++i) {
    Group& group = groups[i];
    for (const TemplateEntry* item : group) {
      std::vector<v8::Local<v8::Value>> to_ids = item->before;
      to_ids.insert(to_ids.end(), item->after.begin(), item->after.end());
      for (v8::Local<v8::Value> id : to_ids) {
        int index = IndexOfGroupContainingId(groups, id, &group);
        if (index == -1)
          continue;
        Group merged = groups[index];
        merged.insert(merged.end(), group.begin(), group.end());
        groups[index] = std::move(merged);
        groups.erase(groups.begin() + i);
        return true;
      }
    }
  }
  return false;
}

Group SortItemsInGroup(const Group& group) {
  // The last item with a given id wins.
  auto index_of = [&](v8::Local<v8::Value> id) -> int {
    for (size_t i = group.size(); i-- > 0;) {
      if (SameId(group[i]->id, id))
        return static_cast<int>(i);
    }
    return -1;
  };
  std::map<int, std::vector<int>> edges;
  for (size_t i = 0; i < group.size(); ++i) {
    for (v8::Local<v8::Value> to_id : group[i]->before) {
      if (int to = index_of(to_id); to != -1)
        edges[to].push_back(static_cast<int>(i));
    }
    for (v8::Local<v8::Value> to_id : group[i]->after) {
      if (int to = index_of(to_id); to != -1)
        edges[static_cast<int>(i)].push_back(to);
    }
  }
  Group sorted;
  for (int index : SortTopologically(static_cast<int>(group.size()), edges))
    sorted.push_back(group[index]);
  return sorted;
}

std::vector<Group> SortGroups(const std::vector<Group>& groups) {
  std::map<int, std::vector<int>> edges;
  for (size_t i = 0; i < groups.size(); ++i) {
    bool found = false;
    for (const TemplateEntry* item : groups[i]) {
      for (v8::Local<v8::Value> id : item->before_group) {
        int to = IndexOfGroupContainingId(groups, id, &groups[i]);
        if (to != -1) {
          edges[to].push_back(static_cast<int>(i));
          found = true;
          break;
        }
      }
      if (found)
        break;
      for (v8::Local<v8::Value> id : item->after_group) {
        int to = IndexOfGroupContainingId(groups, id, &groups[i]);
        if (to != -1) {
          edges[static_cast<int>(i)].push_back(to);
          found = true;
          break;
        }
      }
      if (found)
        break;
    }
  }
  std::vector<Group> sorted;
  for (int index : SortTopologically(static_cast<int>(groups.size()), edges))
    sorted.push_back(groups[index]);
  return sorted;
}

// Applies the before/after/beforeGroupContaining/afterGroupContaining
// constraints. Separators needed between groups beyond those in |entries| are
// appended to |synthesized|.
std::vector<const TemplateEntry*> SortMenuItems(
    v8::Isolate* isolate,
    const std::vector<TemplateEntry>& entries,
    std::vector<TemplateEntry>* synthesized) {
  std::vector<const TemplateEntry*> separators;
  std::vector<Group> groups(1);
  for (const TemplateEntry& e : entries) {
    if (e.is_plain_separator()) {
      separators.push_back(&e);
      if (!groups.back().empty())
        groups.emplace_back();
    } else {
      groups.back().push_back(&e);
    }
  }
  if (groups.back().empty())
    groups.pop_back();

  while (AttemptToMergeAGroup(groups)) {
  }
  for (Group& group : groups)
    group = SortItemsInGroup(group);
  groups = SortGroups(groups);

  std::vector<const TemplateEntry*> joined;
  size_t next_separator = 0;
  for (size_t i = 0; i < groups.size(); ++i) {
    if (i > 0 && !groups[i].empty()) {
      if (next_separator < separators.size()) {
        joined.push_back(separators[next_separator++]);
      } else {
        TemplateEntry e;
        e.item = MenuItem::NewSeparator(isolate);
        e.wrapper = e.item->GetWrapper(isolate).ToLocalChecked();
        synthesized->push_back(std::move(e));
        joined.push_back(&synthesized->back());
      }
    }
    joined.insert(joined.end(), groups[i].begin(), groups[i].end());
  }
  return joined;
}

std::vector<const TemplateEntry*> RemoveExtraSeparators(
    std::vector<const TemplateEntry*> items) {
  // Fold adjacent separators together.
  std::vector<const TemplateEntry*> folded;
  for (size_t i = 0; i < items.size(); ++i) {
    const TemplateEntry* e = items[i];
    bool keep = !e->item->visible() || !e->is_separator() || i == 0 ||
                !items[i - 1]->is_separator();
    if (keep)
      folded.push_back(e);
  }
  // Remove edge separators.
  std::vector<const TemplateEntry*> out;
  for (size_t i = 0; i < folded.size(); ++i) {
    const TemplateEntry* e = folded[i];
    bool keep = !e->item->visible() || !e->is_separator() ||
                (i != 0 && i != folded.size() - 1);
    if (keep)
      out.push_back(e);
  }
  return out;
}

}  // namespace

// static
v8::Local<v8::Value> Menu::BuildFromTemplate(gin_helper::ErrorThrower thrower,
                                             v8::Local<v8::Value> tmpl) {
  v8::Isolate* isolate = thrower.isolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  if (tmpl.IsEmpty() || !IsArray(tmpl)) {
    thrower.ThrowTypeError(
        "Invalid template for Menu: Menu template must be an array");
    return {};
  }
  v8::Local<v8::Object> array = tmpl.As<v8::Object>();
  const uint32_t length = ArrayLength(isolate, array);

  std::vector<TemplateEntry> entries;
  entries.reserve(length);
  for (uint32_t i = 0; i < length; ++i) {
    if (!array->HasOwnProperty(context, i).FromMaybe(false))
      continue;
    v8::Local<v8::Value> value;
    if (!array->Get(context, i).ToLocal(&value) || !value->IsObject() ||
        value->IsNull()) {
      thrower.ThrowTypeError(
          "Invalid template for MenuItem: must have at least one of label, "
          "role or type");
      return {};
    }
    v8::Local<v8::Object> object = value.As<v8::Object>();
    TemplateEntry e;
    e.item = MenuItem::FromV8(isolate, object);
    if (e.item) {
      e.wrapper = object;
    } else {
      std::string type;
      if (!object
               ->HasOwnProperty(context, gin::StringToSymbol(isolate, "label"))
               .FromMaybe(false) &&
          !object->HasOwnProperty(context, gin::StringToSymbol(isolate, "role"))
               .FromMaybe(false) &&
          !(gin_helper::Dictionary(isolate, object).Get("type", &type) &&
            type == "separator")) {
        thrower.ThrowTypeError(
            "Invalid template for MenuItem: must have at least one of label, "
            "role or type");
        return {};
      }
      v8::TryCatch try_catch(isolate);
      v8::Local<v8::Value> created = MenuItem::New(thrower, object);
      if (try_catch.HasCaught()) {
        try_catch.ReThrow();
        return {};
      }
      e.item = MenuItem::FromV8(isolate, created);
      e.wrapper = created.As<v8::Object>();
    }
    ReadConstraints(isolate, &e);
    entries.push_back(std::move(e));
  }

  // Reserve so pointers into |synthesized| stay valid.
  std::vector<TemplateEntry> synthesized;
  synthesized.reserve(entries.size() + 1);
  std::vector<const TemplateEntry*> ordered =
      RemoveExtraSeparators(SortMenuItems(isolate, entries, &synthesized));

  Menu* menu = Menu::Create(isolate);
  v8::Local<v8::Object> menu_object;
  if (!menu || !menu->GetWrapper(isolate).ToLocal(&menu_object))
    return {};
  for (const TemplateEntry* e : ordered) {
    v8::TryCatch try_catch(isolate);
    menu->InsertItem(isolate, menu->GetItemCount(), e->item, &thrower);
    if (try_catch.HasCaught()) {
      try_catch.ReThrow();
      return {};
    }
  }
  return menu_object;
}

}  // namespace electron::api
