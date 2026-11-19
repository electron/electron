// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/menu_bar.h"

#include <memory>

#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "shell/browser/native_window.h"
#include "shell/browser/ui/electron_menu_model.h"
#include "shell/browser/ui/views/root_view.h"
#include "shell/browser/ui/views/submenu_button.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/mojom/menu_source_type.mojom.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/background.h"
#include "ui/views/layout/box_layout.h"

#if BUILDFLAG(IS_LINUX)
#include "shell/browser/ui/gtk_util.h"
#include "ui/gtk/gtk_util.h"  // nogncheck
#endif

#if BUILDFLAG(IS_WIN)
#include "shell/common/color_util.h"
#endif

namespace electron {

namespace {

// Default color of the menu bar.
const SkColor kDefaultColor = SkColorSetARGB(255, 233, 233, 233);

}  // namespace

MenuBar::MenuBar(NativeWindow* window, RootView* root_view)
    : background_color_(kDefaultColor), window_(window), root_view_(root_view) {
  const ui::NativeTheme* theme = root_view_->GetNativeTheme();
#if BUILDFLAG(IS_WIN)
  SetBackground(views::CreateSolidBackground(ui::kColorMenuBackground));
#endif
  RefreshColorCache(theme);
  UpdateViewColors();
  SetFocusBehavior(FocusBehavior::ALWAYS);
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal));
  window_->AddObserver(this);

  auto& view_a11y = GetViewAccessibility();
  view_a11y.SetName(std::u16string(),
                    ax::mojom::NameFrom::kAttributeExplicitlyEmpty);
  view_a11y.SetRole(ax::mojom::Role::kMenuBar);
}

MenuBar::~MenuBar() {
  window_->RemoveObserver(this);
}

void MenuBar::SetMenu(ElectronMenuModel* model) {
  menu_model_ = model;
  RebuildChildren();
}

void MenuBar::SetAcceleratorVisibility(bool visible) {
  for (views::View* child : GetChildrenInZOrder())
    static_cast<SubmenuButton*>(child)->SetAcceleratorVisibility(visible);
}

MenuBar::View* MenuBar::FindAccelChild(char16_t key) {
  if (key == 0)
    return nullptr;
  for (views::View* child : GetChildrenInZOrder()) {
    if (child->GetVisible() && child->GetEnabled() &&
        static_cast<SubmenuButton*>(child)->accelerator() == key)
      return child;
  }
  return nullptr;
}

bool MenuBar::HasAccelerator(char16_t key) {
  return FindAccelChild(key) != nullptr;
}

void MenuBar::ActivateAccelerator(char16_t key) {
  auto* child = FindAccelChild(key);
  if (child)
    static_cast<SubmenuButton*>(child)->Activate(nullptr);
}

size_t MenuBar::GetItemCount() const {
  return menu_model_ ? menu_model_->GetItemCount() : 0;
}

bool MenuBar::GetMenuButtonFromScreenPoint(const gfx::Point& screenPoint,
                                           ElectronMenuModel** menu_model,
                                           views::MenuButton** button) {
  if (!GetBoundsInScreen().Contains(screenPoint))
    return false;

  auto children = GetChildrenInZOrder();
  for (size_t i = 0, n = children.size(); i < n; ++i) {
    if (children[i]->GetBoundsInScreen().Contains(screenPoint) &&
        (menu_model_->GetTypeAt(i) == ElectronMenuModel::TYPE_SUBMENU)) {
      *menu_model = menu_model_->GetSubmenuModelAt(i);
      *button = static_cast<views::MenuButton*>(children[i]);
      return true;
    }
  }

  return false;
}

bool MenuBar::GetSiblingMenuButtonByDirection(
    const int current_id,
    const bool next,
    views::MenuButton** const button) const {
  if (!menu_model_)
    return false;

  const int count = static_cast<int>(GetItemCount());
  if (count == 0)
    return false;

  // Walk adjacent menu-bar items, wrapping around and skipping separators,
  // hidden items, and disabled items.
  const int step = next ? 1 : -1;
  for (int offset = 1; offset <= count; ++offset) {
    const int index = ((current_id + step * offset) % count + count) % count;
    if (index == current_id)
      break;
    if (menu_model_->GetTypeAt(index) == ElectronMenuModel::TYPE_SEPARATOR ||
        !menu_model_->IsVisibleAt(index) || !menu_model_->IsEnabledAt(index)) {
      continue;
    }
    // GetViewByID(0) would return the menu bar itself.
    for (views::View* child : children()) {
      if (child->GetID() == index) {
        *button = static_cast<views::MenuButton*>(child);
        return true;
      }
    }
  }

  return false;
}

void MenuBar::SelectMenuButton(views::MenuButton* button, bool open_submenu) {
  if (open_submenu && menu_model_->GetTypeAt(button->GetID()) ==
                          ElectronMenuModel::TYPE_SUBMENU) {
    if (pane_has_focus()) {
      focus_manager()->SetFocusedViewWithReason(
          button, views::FocusManager::FocusChangeReason::kFocusTraversal);
    }
    button->Activate(nullptr);
    return;
  }

  if (!pane_has_focus() && !SetPaneFocusAndFocusDefault()) {
    return;
  }
  focus_manager()->SetFocusedViewWithReason(
      button, views::FocusManager::FocusChangeReason::kFocusTraversal);
  // Remember whether subsequent arrow navigation should open submenus.
  open_menu_on_navigation_ = open_submenu;
}

void MenuBar::OnBeforeExecuteCommand() {
  open_menu_on_navigation_ = false;
  if (GetPaneFocusTraversable() != nullptr) {
    RemovePaneFocus();
  }
  root_view_->RestoreFocus();
}

void MenuBar::OnMenuClosed() {
  SetAcceleratorVisibility(pane_has_focus());
}

void MenuBar::OnWindowBlur() {
  open_menu_on_navigation_ = false;
  UpdateViewColors();
  SetAcceleratorVisibility(pane_has_focus());
}

void MenuBar::OnWindowFocus() {
  UpdateViewColors();
  SetAcceleratorVisibility(pane_has_focus());
}

bool MenuBar::AcceleratorPressed(const ui::Accelerator& accelerator) {
  // Treat pressing Alt the same as pressing Esc.
  const ui::Accelerator& translated =
      accelerator.key_code() == ui::VKEY_MENU
          ? ui::Accelerator(ui::VKEY_ESCAPE, accelerator.modifiers(),
                            accelerator.key_state(), accelerator.time_stamp())
          : accelerator;
  if (pane_has_focus() && (translated.key_code() == ui::VKEY_LEFT ||
                           translated.key_code() == ui::VKEY_RIGHT)) {
    auto* focused = focus_manager()->GetFocusedView();
    if (!focused || focused->parent() != this) {
      return false;
    }
    const bool next =
        (translated.key_code() == ui::VKEY_RIGHT) != base::i18n::IsRTL();
    views::MenuButton* button = nullptr;
    if (GetSiblingMenuButtonByDirection(focused->GetID(), next, &button)) {
      SelectMenuButton(button, open_menu_on_navigation_);
    }
    return true;
  }

  bool result = views::AccessiblePaneView::AcceleratorPressed(translated);
  if (result && !pane_has_focus()) {
    open_menu_on_navigation_ = false;
    root_view_->RestoreFocus();
  }
  return result;
}

bool MenuBar::SetPaneFocusAndFocusDefault() {
  bool result = views::AccessiblePaneView::SetPaneFocusAndFocusDefault();
  if (result && !accelerator_installed_) {
    // Listen to Alt key events.
    // Note that there is no need to unregister the accelerator.
    accelerator_installed_ = true;
    focus_manager()->RegisterAccelerator(
        ui::Accelerator(ui::VKEY_MENU, ui::EF_ALT_DOWN),
        ui::AcceleratorManager::kNormalPriority, this);
  }
  return result;
}

bool MenuBar::OnKeyPressed(const ui::KeyEvent& event) {
  if (pane_has_focus() && !event.IsControlDown() && !event.IsCommandDown() &&
      event.key_code() >= ui::VKEY_A && event.key_code() <= ui::VKEY_Z &&
      HasAccelerator(event.key_code())) {
    ActivateAccelerator(event.key_code());
    return true;
  }
  return views::AccessiblePaneView::OnKeyPressed(event);
}

void MenuBar::OnThemeChanged() {
  views::AccessiblePaneView::OnThemeChanged();
  const ui::NativeTheme* theme = root_view_->GetNativeTheme();
  RefreshColorCache(theme);
  UpdateViewColors();
}

void MenuBar::OnDidChangeFocus(View* focused_before, View* focused_now) {
  views::AccessiblePaneView::OnDidChangeFocus(focused_before, focused_now);
  SetAcceleratorVisibility(pane_has_focus());
  if (!pane_has_focus()) {
    open_menu_on_navigation_ = false;
    root_view_->RestoreFocus();
  }
}

void MenuBar::ButtonPressed(size_t id, const ui::Event& event) {
  // Keep mnemonic underlines visible only while the menu bar has pane focus.
  SetAcceleratorVisibility(pane_has_focus());

  if (!menu_model_ || !menu_model_->IsEnabledAt(id) ||
      !menu_model_->IsVisibleAt(id)) {
    return;
  }

  ElectronMenuModel::ItemType type = menu_model_->GetTypeAt(id);
  if (type == ElectronMenuModel::TYPE_SEPARATOR) {
    return;
  }
  if (type != ElectronMenuModel::TYPE_SUBMENU && event.IsKeyEvent() &&
      (event.AsKeyEvent()->key_code() == ui::VKEY_UP ||
       event.AsKeyEvent()->key_code() == ui::VKEY_DOWN)) {
    return;
  }

  if (!root_view_->HasFocus())
    root_view_->RequestFocus();

  if (type != ElectronMenuModel::TYPE_SUBMENU) {
    OnBeforeExecuteCommand();
    menu_model_->ActivatedAt(id, 0);
    return;
  }

  SubmenuButton* source = nullptr;
  for (views::View* child : children()) {
    auto* button = static_cast<SubmenuButton*>(child);
    int button_id = button->GetID();
    if (button_id >= 0 && static_cast<size_t>(button_id) == id) {
      source = button;
      break;
    }
  }
  DCHECK(source);

  // Deleted in MenuDelegate::OnMenuClosed
  auto* menu_delegate = new MenuDelegate(this);
  menu_delegate->RunMenu(menu_model_->GetSubmenuModelAt(id), source,
                         event.IsKeyEvent()
                             ? ui::mojom::MenuSourceType::kKeyboard
                             : ui::mojom::MenuSourceType::kMouse);
  menu_delegate->AddObserver(this);
}

void MenuBar::ViewHierarchyChanged(
    const views::ViewHierarchyChangedDetails& details) {
  views::AccessiblePaneView::ViewHierarchyChanged(details);
#if BUILDFLAG(IS_WIN)
  background_color_ =
      GetBackground()->color().ResolveToSkColor(root_view_->GetColorProvider());
#endif
}

void MenuBar::RefreshColorCache(const ui::NativeTheme* theme) {
  if (theme) {
#if BUILDFLAG(IS_LINUX)
    if (gtk_util::IsGtkAvailable()) {
      background_color_ = gtk::GetBgColor("GtkMenuBar#menubar");
      enabled_color_ =
          gtk::GetFgColor("GtkMenuBar#menubar GtkMenuItem#menuitem GtkLabel");
      disabled_color_ = gtk::GetFgColor(
          "GtkMenuBar#menubar GtkMenuItem#menuitem:disabled GtkLabel");
    } else {
      LOG(WARNING)
          << "No GTK UI theme is available; using default menu colors.";
      const auto* color_provider = root_view_->GetColorProvider();
      background_color_ = color_provider->GetColor(ui::kColorMenuBackground);
      enabled_color_ = color_provider->GetColor(ui::kColorMenuItemForeground);
      disabled_color_ =
          color_provider->GetColor(ui::kColorMenuItemForegroundDisabled);
    }
#elif BUILDFLAG(IS_WIN)
    background_color_ = GetBackground()->color().ResolveToSkColor(
        root_view_->GetColorProvider());
#endif
  }
}

void MenuBar::RebuildChildren() {
  open_menu_on_navigation_ = false;
  RemoveAllChildViews();
  for (size_t i = 0, n = GetItemCount(); i < n; ++i) {
    auto button = std::make_unique<SubmenuButton>(
        base::BindRepeating(&MenuBar::ButtonPressed, base::Unretained(this), i),
        menu_model_->GetLabelAt(i), background_color_);
    button->SetID(i);
    button->SetEnabled(menu_model_->IsEnabledAt(i) &&
                       menu_model_->GetTypeAt(i) !=
                           ElectronMenuModel::TYPE_SEPARATOR);
    button->SetVisible(menu_model_->IsVisibleAt(i));
    if (menu_model_->GetTypeAt(i) == ElectronMenuModel::TYPE_SEPARATOR) {
      button->SetFocusBehavior(FocusBehavior::NEVER);
      button->GetViewAccessibility().SetRole(
          ax::mojom::Role::kMenuItemSeparator);
      button->GetViewAccessibility().SetName(
          std::u16string(), ax::mojom::NameFrom::kAttributeExplicitlyEmpty);
    } else if (menu_model_->GetTypeAt(i) != ElectronMenuModel::TYPE_SUBMENU) {
      button->GetViewAccessibility().SetRole(ax::mojom::Role::kMenuItem);
      button->GetViewAccessibility().SetHasPopup(ax::mojom::HasPopup::kFalse);
      if (button->GetEnabled()) {
        button->GetViewAccessibility().SetDefaultActionVerb(
            ax::mojom::DefaultActionVerb::kClick);
      }
    }
    AddChildView(std::move(button));
  }
  UpdateViewColors();
}

void MenuBar::UpdateViewColors() {
#if BUILDFLAG(IS_LINUX)
  // set menubar background color
  SetBackground(views::CreateSolidBackground(background_color_));
#endif

  // set child colors
  if (menu_model_ == nullptr)
    return;
#if BUILDFLAG(IS_LINUX)
  const auto& textColor =
      window_->IsFocused() ? enabled_color_ : disabled_color_;
  for (views::View* child : GetChildrenInZOrder()) {
    auto* button = static_cast<SubmenuButton*>(child);
    button->SetTextColor(views::Button::STATE_NORMAL, textColor);
    button->SetTextColor(views::Button::STATE_DISABLED, disabled_color_);
    button->SetTextColor(views::Button::STATE_PRESSED, enabled_color_);
    button->SetTextColor(views::Button::STATE_HOVERED, textColor);
    button->SetUnderlineColor(textColor);
  }
#elif BUILDFLAG(IS_WIN)
  for (views::View* child : GetChildrenInZOrder()) {
    auto* button = static_cast<SubmenuButton*>(child);
    button->SetUnderlineColor(GetSysSkColor(COLOR_MENUTEXT));
  }
#endif
}

BEGIN_METADATA(MenuBar)
END_METADATA

}  // namespace electron
