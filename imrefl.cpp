#include "imgui_internal.h"

namespace ImRefl {

  namespace detail {

    bool TreeNodeExNoDisable(const char* label, ImGuiTreeNodeFlags flags)
    {
      bool want_disabled = ImGui::GetCurrentContext()->DisabledStackSize > 0;
      if (want_disabled) { ImGui::EndDisabled(); }
      bool open = ImGui::TreeNodeEx(label, flags);
      if (want_disabled) { ImGui::BeginDisabled(); }
  
      return open;
    }
  }

}
