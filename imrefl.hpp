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

template <typename T>
concept arithmetic = std::integral<T> || std::floating_point<T>;

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

template <typename E> requires std::is_scoped_enum_v<E>
constexpr const char* enum_to_string(E value)
{
    template for (constexpr auto e : enums_of<E>()) { 
        if (value == [:e:]) {
            return std::meta::identifier_of(e).data();
        }
    }
    return "<unnamed>";
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

template <arithmetic T>
struct minmax { T min, max; };

template <arithmetic T>
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

// Forward decls

template <typename T>
bool Render(const char* name, T& val);

template <typename T> requires std::is_scoped_enum_v<T>
bool Render(const char* name, T& value);

template <arithmetic T>
bool Render(const char* name, T& val);

bool Render(const char* name, char& c);
bool Render(const char* name, long double& x);
bool Render(const char* name, bool& value);

template <arithmetic T>
bool Render(const char* name, std::span<T> arr);

template <arithmetic T, std::size_t N> requires (N > 0)
bool Render(const char* name, T (&arr)[N]);

template <arithmetic T, std::size_t N> requires (N > 0)
bool Render(const char* name, std::array<T, N>& arr);

bool Render(const char* name, std::string& value);

#ifdef IMREFL_GLM
template <int Size, arithmetic T, glm::qualifier Qual>
bool Render(const char* name, glm::vec<Size, T, Qual>& value);
#endif

template <typename L, typename R>
bool Render(const char* name, std::pair<L, R>& value);

template <typename T>
bool Render(const char* name, std::optional<T>& value);

template <typename T> requires (std::meta::is_aggregate_type(^^T))
bool Render(const char* name, T& x);

// End of forward decls

template <typename T> requires std::is_scoped_enum_v<T>
bool Render(const char* name, T& value)
{
    const auto valueName = enum_to_string(value);
    bool changed = false;
    if (ImGui::BeginCombo(name, valueName)) {
        template for (constexpr auto e : enums_of<T>()) {
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

template <arithmetic T>
bool Render(const char* name, T& val)
{
    ImGuiStorage* storage = ImGui::GetStateStorage();
    if (const auto limits = slider_limits<T>()) {
        return ImGui::SliderScalar(name, num_type<T>(), &val, &limits->min, &limits->max);
    } else {
        const T step = 1; // Only used for integral types
        return ImGui::InputScalar(name, num_type<T>(), &val, std::integral<T> ? &step : nullptr);
    }
}

// Treat char as a single character string, rather than an integral
bool Render(const char* name, char& c)
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
bool Render(const char* name, long double& x)
{
    double temp = static_cast<double>(x);
    if (Render(name, temp)) {
        x = temp;
        return true;
    }
    return false;
}

bool Render(const char* name, bool& value)
{
    return ImGui::Checkbox(name, &value);
}

template <arithmetic T>
bool Render(const char* name, std::span<T> arr)
{
    if (arr.empty()) {
        ImGui::Text("span '%s' is of length 0", name);
        return false;
    }

    // Only floating point types permit the color options
    if constexpr (std::floating_point<T>) {
        ImGuiStorage* storage = ImGui::GetStateStorage();
        switch (arr.size()) {
            case 3: {
                if (storage->GetBool(ImGui::GetID("color_wheel"), false)) {
                    return ImGui::ColorPicker3(name, arr.data());
                } else if (storage->GetBool(ImGui::GetID("color"), false)) {
                    return ImGui::ColorEdit3(name, arr.data());
                }
            } break;
            case 4: {
                if (storage->GetBool(ImGui::GetID("color_wheel"), false)) {
                    return ImGui::ColorPicker4(name, arr.data());
                } else if (storage->GetBool(ImGui::GetID("color"), false)) {
                    return ImGui::ColorEdit4(name, arr.data());
                }
            } break;
        }
    }

    return ImGui::InputScalarN(name, num_type<T>(), arr.data(), arr.size());
}

template <arithmetic T, std::size_t N>
    requires (N > 0)
bool Render(const char* name, T (&arr)[N])
{
    return Render(name, std::span<T>{arr, N});
}

template <arithmetic T, std::size_t N>
    requires (N > 0)
bool Render(const char* name, std::array<T, N>& arr)
{
    return Render(name, std::span<T>{arr.begin(), arr.end()});
}

bool Render(const char* name, std::string& value)
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
template <int Size, arithmetic T, glm::qualifier Qual>
bool Render(const char* name, glm::vec<Size, T, Qual>& value)
{
    return Render(name, std::span{&value[0], Size});
}
#endif

template <typename L, typename R>
bool Render(const char* name, std::pair<L, R>& value)
{
    ImGui::Text("%s", name);

    const auto width = ImGui::GetContentRegionAvail().x
                     - ImGui::GetStyle().ItemSpacing.x;
    ImGui::PushItemWidth(width / 2.0f);

    bool changed = false;
    changed = changed || Render("first", value.first);
    changed = changed || Render("second", value.second);

    ImGui::PopItemWidth();
    return changed;
}

template <typename T>
bool Render(const char* name, std::optional<T>& value)
{
    bool changed = false;
    ImGui::PushID(name);
    if (value.has_value()) {
        if (ImGui::Button("Delete")) {
            value = {};
            changed = true;
        } else {
            ImGui::SameLine();
            Render(name, *value);
        }
    }
    else {
        if (ImGui::Button("New")) {
            value.emplace();
            changed = true;
        } else {
            ImGui::SameLine();
            ImGui::Text("%s", name);
        }
    }
    ImGui::PopID();
    return changed;
}

template <typename T> requires (std::meta::is_aggregate_type(^^T))
bool Render(const char* name, T& x)
{
    ImGuiStorage* storage = ImGui::GetStateStorage();
    bool changed = false;

    ImGui::Text("%s", name);
    template for (constexpr auto member : nsdm_of<T>()) {
        if constexpr (!has_annotation<ImReflIgnore>(member)) {
            if constexpr (has_annotation<ImReflReadonly>(member)) { ImGui::BeginDisabled(); }

            // TODO: Generalise this
            if constexpr (constexpr auto slider_info = fetch_annotation<ImReflSlider>(member)) {
                static_assert(is_arithmetic_type(type_of(member)));
                storage->SetBool(ImGui::GetID("has_slider"), true);
                storage->SetInt(ImGui::GetID("slider_min"), slider_info->min);
                storage->SetInt(ImGui::GetID("slider_max"), slider_info->max);
            }
            if constexpr (has_annotation<ImReflColorWheel>(member)) {
                storage->SetBool(ImGui::GetID("color_wheel"), true);
            }
            if constexpr (has_annotation<ImReflColor>(member)) {
                storage->SetBool(ImGui::GetID("color"), true);
            }

            changed = changed || Render(std::meta::identifier_of(member).data(), x.[:member:]);
            
            if constexpr (has_annotation<ImReflColor>(member)) {
                storage->SetBool(ImGui::GetID("color"), true);
            }
            if constexpr (has_annotation<ImReflColorWheel>(member)) {
                storage->SetBool(ImGui::GetID("color_wheel"), false);
            }
            if constexpr (constexpr auto slider_info = fetch_annotation<ImReflSlider>(member)) {
                storage->SetBool(ImGui::GetID("has_slider"), false);
            }

            if constexpr (has_annotation<ImReflReadonly>(member)) { ImGui::EndDisabled(); }
        }
    }

    return changed;
}

template <typename T>
bool Render(const char* name, T& val)
{
    static_assert(false && "not implemented for this type"); 
}

}  // namespace detail

bool Input(const char* name, auto& value)
{
    return detail::Render(name, value);
}

}  // namespace ImRefl
