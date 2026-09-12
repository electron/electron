// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

// The ordering part of Menu.buildFromTemplate().

#include <map>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "base/functional/function_ref.h"
#include "base/memory/raw_ptr.h"
#include "gin/converter.h"
#include "gin/dictionary.h"
#include "shell/browser/api/electron_api_menu.h"
#include "shell/common/gin_helper/dictionary.h"
#include "v8/include/v8.h"

namespace electron::api {

namespace {

// Every V8 value read from the template lives in one v8::LocalVector; the
// sorting data refers to values and entries by index.
using ValueIndex = size_t;
using EntryIndex = size_t;

struct TemplateEntry {
  // A separator with no ordering constraints splits groups.
  bool is_plain_separator() const { return separator && !constrained; }

  ValueIndex value = 0;
  bool separator = false;
  bool hidden = false;  // visible === false
  // Ids are compared with ===.
  std::optional<ValueIndex> id;
  std::vector<ValueIndex> before, after, before_group, after_group;
  bool constrained = false;  // any of before/after/...GroupContaining set
};

using Group = std::vector<EntryIndex>;

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

// Sort nodes topologically, depth first; cycles are broken.
std::vector<size_t> SortTopologically(
    size_t count,
    const std::map<size_t, std::vector<size_t>>& edges) {
  std::vector<size_t> sorted;
  std::vector<bool> marked(count, false);
  // (node, index of the next edge to visit)
  std::vector<std::pair<size_t, size_t>> stack;
  for (size_t start = 0; start < count; ++start) {
    if (marked[start])
      continue;
    marked[start] = true;
    stack.emplace_back(start, 0);
    while (!stack.empty()) {
      auto& [node, next] = stack.back();
      auto it = edges.find(node);
      if (it != edges.end() && next < it->second.size()) {
        size_t to = it->second[next++];
        if (!marked[to]) {
          marked[to] = true;
          stack.emplace_back(to, 0);
        }
        continue;
      }
      sorted.push_back(node);
      stack.pop_back();
    }
  }
  return sorted;
}

class TemplateSorter {
 public:
  explicit TemplateSorter(v8::Isolate* isolate)
      : isolate_(isolate), values_(isolate) {}

  // Adds a template object, reading its ordering fields.
  void Add(v8::Local<v8::Object> object, bool separator, bool hidden) {
    gin_helper::Dictionary dict(isolate_, object);
    TemplateEntry e;
    e.value = Push(object);
    e.separator = separator;
    e.hidden = hidden;
    v8::Local<v8::Value> id;
    if (dict.Get("id", &id) && !id->IsUndefined())
      e.id = Push(id);
    e.before = IdList(dict, "before", &e.constrained);
    e.after = IdList(dict, "after", &e.constrained);
    e.before_group = IdList(dict, "beforeGroupContaining", &e.constrained);
    e.after_group = IdList(dict, "afterGroupContaining", &e.constrained);
    entries_.push_back(std::move(e));
  }

  // Applies the before/after/beforeGroupContaining/afterGroupContaining
  // constraints and folds separators; returns the values in display order.
  // |make_separator| supplies separators needed between groups beyond those
  // in the template.
  v8::LocalVector<v8::Value> Sort(
      base::FunctionRef<v8::Local<v8::Object>()> make_separator) {
    std::vector<EntryIndex> ordered =
        RemoveExtraSeparators(SortMenuItems(make_separator));
    v8::LocalVector<v8::Value> out(isolate_);
    for (EntryIndex i : ordered)
      out.push_back(values_[entries_[i].value]);
    return out;
  }

 private:
  ValueIndex Push(v8::Local<v8::Value> value) {
    values_.push_back(value);
    return values_.size() - 1;
  }

  std::vector<ValueIndex> IdList(const gin_helper::Dictionary& dict,
                                 std::string_view key,
                                 bool* constrained) {
    std::vector<ValueIndex> out;
    v8::Local<v8::Value> value;
    if (!dict.Get(key, &value))
      return out;
    if (value->BooleanValue(isolate_))
      *constrained = true;
    if (!IsArray(value))
      return out;
    v8::Local<v8::Object> array = value.As<v8::Object>();
    v8::Local<v8::Context> context = isolate_->GetCurrentContext();
    for (uint32_t i = 0, n = ArrayLength(isolate_, array); i < n; ++i) {
      v8::Local<v8::Value> element;
      if (array->Get(context, i).ToLocal(&element))
        out.push_back(Push(element));
    }
    return out;
  }

  bool HasId(EntryIndex entry, ValueIndex id) const {
    const std::optional<ValueIndex>& entry_id = entries_[entry].id;
    return entry_id && values_[*entry_id]->StrictEquals(values_[id]);
  }

  std::optional<size_t> IndexOfGroupContainingId(
      const std::vector<Group>& groups,
      ValueIndex id,
      size_t ignore_group) const {
    for (size_t i = 0; i < groups.size(); ++i) {
      if (i == ignore_group)
        continue;
      for (EntryIndex e : groups[i]) {
        if (HasId(e, id))
          return i;
      }
    }
    return std::nullopt;
  }

  bool AttemptToMergeAGroup(std::vector<Group>& groups) const {
    for (size_t i = 0; i < groups.size(); ++i) {
      for (EntryIndex item : groups[i]) {
        const TemplateEntry& e = entries_[item];
        for (const std::vector<ValueIndex>* ids : {&e.before, &e.after}) {
          for (ValueIndex id : *ids) {
            std::optional<size_t> index =
                IndexOfGroupContainingId(groups, id, i);
            if (!index)
              continue;
            groups[*index].insert(groups[*index].end(), groups[i].begin(),
                                  groups[i].end());
            groups.erase(groups.begin() + i);
            return true;
          }
        }
      }
    }
    return false;
  }

  Group SortItemsInGroup(const Group& group) const {
    // The last item with a given id wins.
    auto index_of = [&](ValueIndex id) -> std::optional<size_t> {
      for (size_t i = group.size(); i-- > 0;) {
        if (HasId(group[i], id))
          return i;
      }
      return std::nullopt;
    };
    std::map<size_t, std::vector<size_t>> edges;
    for (size_t i = 0; i < group.size(); ++i) {
      for (ValueIndex to_id : entries_[group[i]].before) {
        if (std::optional<size_t> to = index_of(to_id))
          edges[*to].push_back(i);
      }
      for (ValueIndex to_id : entries_[group[i]].after) {
        if (std::optional<size_t> to = index_of(to_id))
          edges[i].push_back(*to);
      }
    }
    Group sorted;
    for (size_t index : SortTopologically(group.size(), edges))
      sorted.push_back(group[index]);
    return sorted;
  }

  std::vector<Group> SortGroups(const std::vector<Group>& groups) const {
    std::map<size_t, std::vector<size_t>> edges;
    for (size_t i = 0; i < groups.size(); ++i) {
      [&] {
        for (EntryIndex item : groups[i]) {
          for (ValueIndex id : entries_[item].before_group) {
            if (std::optional<size_t> to =
                    IndexOfGroupContainingId(groups, id, i)) {
              edges[*to].push_back(i);
              return;
            }
          }
          for (ValueIndex id : entries_[item].after_group) {
            if (std::optional<size_t> to =
                    IndexOfGroupContainingId(groups, id, i)) {
              edges[i].push_back(*to);
              return;
            }
          }
        }
      }();
    }
    std::vector<Group> sorted;
    for (size_t index : SortTopologically(groups.size(), edges))
      sorted.push_back(groups[index]);
    return sorted;
  }

  std::vector<EntryIndex> SortMenuItems(
      base::FunctionRef<v8::Local<v8::Object>()> make_separator) {
    std::vector<EntryIndex> separators;
    std::vector<Group> groups(1);
    for (EntryIndex i = 0; i < entries_.size(); ++i) {
      if (entries_[i].is_plain_separator()) {
        separators.push_back(i);
        if (!groups.back().empty())
          groups.emplace_back();
      } else {
        groups.back().push_back(i);
      }
    }
    if (groups.back().empty())
      groups.pop_back();

    while (AttemptToMergeAGroup(groups)) {
    }
    for (Group& group : groups)
      group = SortItemsInGroup(group);
    groups = SortGroups(groups);

    std::vector<EntryIndex> joined;
    size_t next_separator = 0;
    for (size_t i = 0; i < groups.size(); ++i) {
      if (i > 0 && !groups[i].empty()) {
        if (next_separator < separators.size()) {
          joined.push_back(separators[next_separator++]);
        } else {
          TemplateEntry e;
          e.value = Push(make_separator());
          e.separator = true;
          entries_.push_back(std::move(e));
          joined.push_back(entries_.size() - 1);
        }
      }
      joined.insert(joined.end(), groups[i].begin(), groups[i].end());
    }
    return joined;
  }

  std::vector<EntryIndex> RemoveExtraSeparators(
      const std::vector<EntryIndex>& items) const {
    // Fold adjacent separators together.
    std::vector<EntryIndex> folded;
    for (size_t i = 0; i < items.size(); ++i) {
      const TemplateEntry& e = entries_[items[i]];
      bool keep = e.hidden || !e.separator || i == 0 ||
                  !entries_[items[i - 1]].separator;
      if (keep)
        folded.push_back(items[i]);
    }
    // Remove edge separators.
    std::vector<EntryIndex> out;
    for (size_t i = 0; i < folded.size(); ++i) {
      const TemplateEntry& e = entries_[folded[i]];
      bool keep =
          e.hidden || !e.separator || (i != 0 && i != folded.size() - 1);
      if (keep)
        out.push_back(folded[i]);
    }
    return out;
  }

  const raw_ptr<v8::Isolate> isolate_;
  v8::LocalVector<v8::Value> values_;
  std::vector<TemplateEntry> entries_;
};

}  // namespace

// static
v8::Local<v8::Value> Menu::SortTemplate(v8::Isolate* isolate,
                                        v8::Local<v8::Value> tmpl) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  if (!IsArray(tmpl))
    return tmpl;
  v8::Local<v8::Object> array = tmpl.As<v8::Object>();
  const uint32_t length = ArrayLength(isolate, array);

  TemplateSorter sorter(isolate);
  for (uint32_t i = 0; i < length; ++i) {
    v8::Local<v8::Value> value;
    if (array->HasOwnProperty(context, i).FromMaybe(false) &&
        array->Get(context, i).ToLocal(&value) && value->IsObject()) {
      gin_helper::Dictionary dict(isolate, value.As<v8::Object>());
      std::string type;
      bool separator = dict.Get("type", &type) && type == "separator";
      v8::Local<v8::Value> visible;
      bool hidden = dict.Get("visible", &visible) && visible->IsFalse();
      sorter.Add(value.As<v8::Object>(), separator, hidden);
    }
  }

  v8::LocalVector<v8::Value> sorted = sorter.Sort([&] {
    gin_helper::Dictionary separator = gin::Dictionary::CreateEmpty(isolate);
    separator.Set("type", std::string_view("separator"));
    return gin::ConvertToV8(isolate, separator).As<v8::Object>();
  });
  return v8::Array::New(isolate, sorted.data(), sorted.size());
}

}  // namespace electron::api
