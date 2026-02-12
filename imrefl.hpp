#include <imgui.h>

#ifdef IMREFL_GLM
#include <glm/glm.hpp>
#endif

#include <experimental/meta>
#include <print>
#include <format>
#include <concepts>
#include <type_traits>
#include <optional>

namespace ImRefl {

struct ImReflIgnore {};
inline static constexpr ImReflIgnore ignore {};

struct ImReflReadonly {};
inline static constexpr ImReflReadonly readonly {};

struct ImReflColor {};
inline static constexpr ImReflColor color {};

struct ImReflColorWheel {};
inline static constexpr ImReflColorWheel color_wheel {};

// TODO: Give this another think, it would be great for this to hold the
// correct type, but the interface should be as simple as slider(min, max) for
// every type.
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

template <typename T>
consteval std::optional<T> fetch_annotation(std::meta::info info)
{
    for (const auto a : std::meta::annotations_of(info)) {
        if (std::meta::type_of(a) == ^^T) {
            return std::meta::extract<T>(a);
        }
    }
    return {};
}

template <typename T>
consteval bool has_annotation(std::meta::info info)
{
    return fetch_annotation<T>(info).has_value();
}

template <std::signed_integral T>
consteval auto num_type()
{
    switch (sizeof(T)) {
        case 1: return ImGuiDataType_S8;
        case 2: return ImGuiDataType_S16;
        case 4: return ImGuiDataType_S32;
        case 8: return ImGuiDataType_S64;
    }
    throw "unknown signed integral size";
}

template <std::unsigned_integral T>
consteval auto num_type()
{
    switch (sizeof(T)) {
        case 1: return ImGuiDataType_U8;
        case 2: return ImGuiDataType_U16;
        case 4: return ImGuiDataType_U32;
        case 8: return ImGuiDataType_U64;
    }
    throw "unknown unsigned integral size";
}

template <std::floating_point T>
consteval auto num_type()
{
    switch (sizeof(T)) {
        case 4: return ImGuiDataType_Float;
        case 8: return ImGuiDataType_Double;
    }
    throw "unknown floating point size";
}

template <typename T>
struct minmax { T min, max; };

template <typename T>
    requires std::integral<T> || std::floating_point<T>
constexpr std::optional<minmax<T>> slider_limits()
{
    ImGuiStorage* storage = ImGui::GetStateStorage();
    if (storage->GetBool(ImGui::GetID("has_slider"), false)) {
        const auto min = static_cast<T>(storage->GetInt(ImGui::GetID("slider_min")));
        const auto max = static_cast<T>(storage->GetInt(ImGui::GetID("slider_max")));
        return minmax{min, max};
    }
    return {};
}

}

template <typename T> bool Input(const char* name, T& val);

template <typename T> requires std::is_scoped_enum_v<T>
bool Input(const char* name, T& value)
{
    const auto valueName = detail::enum_to_string(value);
    bool changed = false;
    if (ImGui::BeginCombo(name, valueName)) {
        template for (constexpr auto e : detail::enums_of<T>()) {
            constexpr auto enumName = std::meta::identifier_of(e);
            if (ImGui::Selectable(enumName.data(), value == [:e:])) {
                value = [:e:];
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

template <typename T>
    requires std::integral<T> || std::floating_point<T>
bool Input(const char* name, T& val)
{
    ImGuiStorage* storage = ImGui::GetStateStorage();
    if (const auto limits = detail::slider_limits<T>()) {
        return ImGui::SliderScalar(name, detail::num_type<T>(), &val, &limits->min, &limits->max);
    } else {
        const T step = 1; // Only used for integral types
        return ImGui::InputScalar(name, detail::num_type<T>(), &val, std::integral<T> ? &step : nullptr);
    }
}

// Treat char as a single character string, rather than an integral
bool Input(const char* name, char& c)
{
    char buffer[2] = {c, '\0'};
    if (ImGui::InputText(name, buffer, sizeof(buffer))) {
        c = buffer[0];
        return true;
    }
    return false;
}

// ImGui does not support long double out of the box, but double
// precision is almost certainly fine for UI debugging
bool Input(const char* name, long double& x)
{
    double temp = static_cast<double>(x);
    if (Input(name, temp)) {
        x = temp;
        return true;
    }
    return false;
}

bool Input(const char* name, bool& value)
{
    return ImGui::Checkbox(name, &value);
}

bool Input(const char* name, std::string& value)
{
    auto callback = [](ImGuiInputTextCallbackData* data) -> int {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
            auto* str = static_cast<std::string*>(data->UserData);
            str->resize(data->BufTextLen);
            data->Buf = str->data();
        }
        return 0;
    };
    return ImGui::InputText(
        name,
        value.data(),
        value.size() + 1,
        ImGuiInputTextFlags_CallbackResize,
        callback,
        static_cast<void*>(&value)
    );
}

#ifdef IMREFL_GLM
bool Input(const char* name, glm::vec2& value)
{
    return ImGui::InputFloat2(name, &value[0]);
}

bool Input(const char* name, glm::vec3& value)
{
    ImGuiStorage* storage = ImGui::GetStateStorage();
    if (storage->GetBool(ImGui::GetID("color_wheel"), false)) {
        return ImGui::ColorPicker3(name, &value[0]);
    } else if (storage->GetBool(ImGui::GetID("color"), false)) {
        return ImGui::ColorEdit3(name, &value[0]);
    } else {
        return ImGui::InputFloat3(name, &value[0]);
    }
}

bool Input(const char* name, glm::vec4& value)
{
    ImGuiStorage* storage = ImGui::GetStateStorage();
    if (storage->GetBool(ImGui::GetID("color_wheel"), false)) {
        return ImGui::ColorPicker4(name, &value[0]);
    } else if (storage->GetBool(ImGui::GetID("color"), false)) {
        return ImGui::ColorEdit4(name, &value[0]);
    } else {
        return ImGui::InputFloat4(name, &value[0]);
    }
}
#endif

template <typename L, typename R>
bool Input(const char* name, std::pair<L, R>& value)
{
    ImGui::Text("%s", name);
    char buffer[256] = {}; // This should be long enough for variable name
    const auto name_length = std::strlen(name); 
    if (name_length > sizeof(buffer) - 1 - 3) { // -1 for null terminator, -3 for ##L prefix
        ImGui::Text("Cannot display variable '%s', name too long", name);
        return false;
    }
    bool changed = false;
    buffer[0] = '#';
    buffer[1] = '#';

    const auto half_width = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2.0f;
    ImGui::PushItemWidth(half_width);

    buffer[2] = 'L';
    std::memcpy(&buffer[3], name, std::strlen(name));
    changed = changed || Input(buffer, value.first);

    ImGui::SameLine();  // Both elements are on the same line

    buffer[2] = 'R';
    std::memcpy(&buffer[3], name, std::strlen(name));
    changed = changed || Input(buffer, value.second);

    ImGui::PopItemWidth();
    return changed;
}

template <typename T>
bool Input(const char* name, std::optional<T>& value)
{
    bool changed = false;
    if (value.has_value()) {
        if (ImGui::Button("Delete")) {
            value = {};
            changed = true;
        } else {
            ImGui::SameLine();
            Input(name, *value);
        }
    }
    else {
        if (ImGui::Button("Emplace")) {
            value.emplace();
            changed = true;
        } else {
            ImGui::SameLine();
            ImGui::Text("%s", name);
        }
    }
    return changed;
}

template <typename T> requires std::is_aggregate_v<T>
bool Input(const char* name, T& x)
{
    ImGuiStorage* storage = ImGui::GetStateStorage();
    bool changed = false;

    ImGui::Text("%s", name);
    template for (constexpr auto member : detail::nsdm_of<T>()) {
        if constexpr (!detail::has_annotation<ImReflIgnore>(member)) {
            if constexpr (detail::has_annotation<ImReflReadonly>(member)) { ImGui::BeginDisabled(); }

            // TODO: Generalise this
            if constexpr (constexpr auto slider_info = detail::fetch_annotation<ImReflSlider>(member)) {
                static_assert(is_arithmetic_type(type_of(member)));
                storage->SetBool(ImGui::GetID("has_slider"), true);
                storage->SetInt(ImGui::GetID("slider_min"), slider_info->min);
                storage->SetInt(ImGui::GetID("slider_max"), slider_info->max);
            }
            if constexpr (detail::has_annotation<ImReflColorWheel>(member)) {
                storage->SetBool(ImGui::GetID("color_wheel"), true);
            }
            if constexpr (detail::has_annotation<ImReflColor>(member)) {
                storage->SetBool(ImGui::GetID("color"), true);
            }

            changed = changed || Input(std::meta::identifier_of(member).data(), x.[:member:]);
            
            if constexpr (detail::has_annotation<ImReflColor>(member)) {
                storage->SetBool(ImGui::GetID("color"), true);
            }
            if constexpr (detail::has_annotation<ImReflColorWheel>(member)) {
                storage->SetBool(ImGui::GetID("color_wheel"), false);
            }
            if constexpr (constexpr auto slider_info = detail::fetch_annotation<ImReflSlider>(member)) {
                storage->SetBool(ImGui::GetID("has_slider"), false);
            }

            if constexpr (detail::has_annotation<ImReflReadonly>(member)) { ImGui::EndDisabled(); }
        }
    }

    return changed;
}

template <typename T>
bool Input(const char* name, T& val)
{
    static_assert(false && "not implemented for this type"); 
}

}
