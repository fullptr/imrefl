#include <imgui.h>

#include <experimental/meta>
#include <print>
#include <format>
#include <concepts>
#include <type_traits>

namespace ImRefl {

struct ImReflHidden {};
inline static constexpr ImReflHidden hidden {};

struct ImReflReadonly {};
inline static constexpr ImReflReadonly readonly {};

struct ImReflSlider { int min; int max; };
constexpr ImReflSlider slider(int min, int max) { return {min, max}; }

namespace detail {

template <typename E> requires std::is_scoped_enum_v<E>
constexpr const char* enum_to_string(E value)
{
    template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E))) {
        if (value == [:e:]) {
            return std::meta::identifier_of(e).data();
        }
    }
    return "<unnamed>";
}

template <typename E> requires std::is_scoped_enum_v<E>
consteval auto enums_of()
{
    return std::define_static_array(std::meta::enumerators_of(^^E));
}

template <typename T>
consteval auto nsdm_of()
{
    const auto ctx = std::meta::access_context::current();
	return std::define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx));
}

consteval bool is_hidden(std::meta::info info)
{
    for (const auto a : std::meta::annotations_of(info)) {
        if (std::meta::type_of(a) == ^^ImReflHidden) {
            return true;
        }
    }
    return false;
}

consteval bool is_readonly(std::meta::info info)
{
    for (const auto a : std::meta::annotations_of(info)) {
        if (std::meta::type_of(a) == ^^ImReflReadonly) {
            return true;
        }
    }
    return false;
}

// TODO: Implement this for other types, currently only supported
// by integral types
consteval std::optional<ImReflSlider> has_slider(std::meta::info info)
{
    for (const auto a : std::meta::annotations_of(info)) {
        if (std::meta::type_of(a) == ^^ImReflSlider) {
            return std::meta::extract<ImReflSlider>(a);
        }
    }
    return {};
}

}

template <typename T> void Input(const char* name, T& val);

template <typename T> requires std::is_scoped_enum_v<T>
void Input(const char* name, T& value)
{
    const auto valueName = detail::enum_to_string(value);
    if (ImGui::BeginCombo(name, valueName)) {
        template for (constexpr auto e : detail::enums_of<T>()) {
            constexpr auto enumName = std::meta::identifier_of(e);
            if (ImGui::Selectable(enumName.data(), value == [:e:])) {
                value = [:e:];
            }
        }
        ImGui::EndCombo();
    }
}

template <std::signed_integral T>
void Input(const char* name, T& x)
{
    ImGuiStorage* storage = ImGui::GetStateStorage();
    if (storage->GetBool(ImGui::GetID("has_slider"), false)) {
        const int min = storage->GetInt(ImGui::GetID("slider_min"));
        const int max = storage->GetInt(ImGui::GetID("slider_max"));
        ImGui::SliderInt(name, &x, min, max);
    } else {
        ImGui::InputInt(name, &x);
    }
}

void Input(const char* name, float& x)
{
    ImGui::InputFloat(name, &x);
}

void Input(const char* name, double& x)
{
    ImGui::InputDouble(name, &x);
}

void Input(const char* name, bool& value)
{
    ImGui::Checkbox(name, &value);
}

void Input(const char* name, std::string& value)
{
    auto callback = [](ImGuiInputTextCallbackData* data) -> int {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
            auto* str = static_cast<std::string*>(data->UserData);
            str->resize(data->BufTextLen);
            data->Buf = str->data();
        }
        return 0;
    };
    
    ImGui::InputText(
        name,
        value.data(),
        value.size() + 1,
        ImGuiInputTextFlags_CallbackResize,
        callback,
        static_cast<void*>(&value)
    );
}

template <typename T> requires std::is_aggregate_v<T>
void Input(const char* name, T& x)
{
    ImGuiStorage* storage = ImGui::GetStateStorage();

    ImGui::Text("%s", name);
    template for (constexpr auto member : detail::nsdm_of<T>()) {
        if constexpr (detail::is_hidden(member)) { continue; }
        if constexpr (detail::is_readonly(member)) { ImGui::BeginDisabled(); }

        // TODO: Generalise this
        if constexpr (constexpr auto slider_info = detail::has_slider(member)) {
            static_assert(std::meta::type_of(member) == ^^int);
            storage->SetBool(ImGui::GetID("has_slider"), true);
            storage->SetInt(ImGui::GetID("slider_min"), slider_info->min);
            storage->SetInt(ImGui::GetID("slider_max"), slider_info->max);
        }

        Input(std::meta::identifier_of(member).data(), x.[:member:]);
        
        if constexpr (constexpr auto slider_info = detail::has_slider(member)) {
            storage->SetBool(ImGui::GetID("has_slider"), false);
        }

        if constexpr (detail::is_readonly(member)) { ImGui::EndDisabled(); }
    }
}

template <typename T>
void Input(const char* name, T& val)
{
    static_assert(false && "not implemented for this type"); 
}

}
