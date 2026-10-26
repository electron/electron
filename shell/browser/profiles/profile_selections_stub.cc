// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile_selections.h"

// See profile_keyed_service_factory_stub.cc: the selections are built but
// never applied, so only construction is implemented.

ProfileSelections::Builder::Builder() : selections_(new ProfileSelections()) {}

ProfileSelections::Builder::~Builder() = default;

ProfileSelections::Builder& ProfileSelections::Builder::WithRegular(
    ProfileSelection selection) {
  selections_->regular_profile_selection_ = selection;
  return *this;
}

ProfileSelections::Builder& ProfileSelections::Builder::WithGuest(
    ProfileSelection selection) {
  selections_->guest_profile_selection_ = selection;
  return *this;
}

ProfileSelections::Builder& ProfileSelections::Builder::WithSystem(
    ProfileSelection selection) {
  selections_->system_profile_selection_ = selection;
  return *this;
}

ProfileSelections::Builder& ProfileSelections::Builder::WithIsolatedMode(
    ProfileSelection selection) {
  selections_->isolated_mode_profile_selection_ = selection;
  return *this;
}

ProfileSelections::Builder& ProfileSelections::Builder::WithAshInternals(
    ProfileSelection selection) {
  selections_->ash_internals_profile_selection_ = selection;
  return *this;
}

ProfileSelections ProfileSelections::Builder::Build() {
  return *selections_;
}

ProfileSelections::ProfileSelections() = default;
ProfileSelections::ProfileSelections(const ProfileSelections& other) = default;
ProfileSelections::~ProfileSelections() = default;
