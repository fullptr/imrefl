#ifndef INCLUDED_IMREFL_GLM_H
#define INCLUDED_IMREFL_GLM_H

#include "imrefl.hpp"

#include <glm/glm.hpp>
#include <span>

namespace ImRefl {

template <Config config, int Size, detail::scalar T, glm::qualifier Qual>
struct Renderer<config, glm::vec<Size, T, Qual>>
{
    static bool Render(const char* name, glm::vec<Size, T, Qual>& value)
    {
        return Input<config>(name, std::span{&value[0], Size});
    }

    static bool Render(const char* name, const glm::vec<Size, T, Qual>& value)
    {
        return Input<config>(name, std::span{&value[0], Size});
    }
};

template <Config config, int C, int R, detail::scalar T, glm::qualifier Qual>
struct Renderer<config, glm::mat<C, R, T, Qual>>
{
    static constexpr ImGuiTableFlags flags = ImGuiTableFlags_BordersOuter;
    static bool Render(const char* name, glm::mat<C, R, T, Qual>& value)
    {
        bool changed = false;

        ImGui::Text("%s", name);
        ImGui::BeginTable(name, C, flags, {ImGui::CalcItemWidth(), 0.f});
        for (int r = 0; r < R; ++r) {
            ImGui::PushID(r);
            for (int c = 0; c < C; ++c) {
                ImGui::TableNextColumn();

                ImGui::PushID(c);
                ImGui::SetNextItemWidth(-FLT_MIN);
                changed = ImRefl::Input<config>("##input", value[c][r]) || changed;
                ImGui::PopID();
            }
            ImGui::PopID();
        }
        ImGui::EndTable();

        return changed;
    }

    static bool Render(const char* name, const glm::mat<C, R, T, Qual>& value)
    {
        ImGui::Text("%s", name);
        ImGui::BeginTable(name, C, flags, {ImGui::CalcItemWidth(), 0.f});
        for (int r = 0; r < R; ++r) {
            ImGui::PushID(r);
            for (int c = 0; c < C; ++c) {
                ImGui::TableNextColumn();

                ImGui::PushID(c);
                ImGui::SetNextItemWidth(-FLT_MIN);
                ImRefl::Input<config>("##input", value[c][r]);
                ImGui::PopID();
            }
            ImGui::PopID();
        }
        ImGui::EndTable();

        return false;
    }
};

}  // namespace ImRefl

#endif // INCLUDED_IMREFL_GLM_H
