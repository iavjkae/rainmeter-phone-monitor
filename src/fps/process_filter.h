// SPDX-License-Identifier: MIT
#pragma once

#include "fps/config.h"

#include <string_view>

namespace phone_monitor::fps {

class ProcessFilter {
  public:
    explicit ProcessFilter(SelectionPolicy policy);
    bool allows(std::string_view name) const;

  private:
    SelectionPolicy policy_;
};

} // namespace phone_monitor::fps
