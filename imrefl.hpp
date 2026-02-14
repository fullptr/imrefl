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
#include <variant>

namespace ImRefl {

struct Ignore {};
inline static constexpr Ignore ignore {};

struct Readonly {};
inline static constexpr Readonly readonly {};

struct Color {};
inline static constexpr Color color {};

struct ColorWheel {};
inline static constexpr ColorWheel color_wheel {};

// The default "input" way to display a scalar. Not really
// useful in exposing publicly but done so for consistency.
// I would call this Input but that name is taken.
struct Normal {};
inline static constexpr Normal normal {};

// TODO: Give this another think; should these be floats?
// Or should we have slider() and sliderf(), which I don't really like.
struct Slider { int min; int max; };
constexpr Slider slider(int min, int max) { return {min, max}; }

// TODO: Same todo as the above, but speed is always a float
// since that is what ImGui supports.
struct Drag { int min; int max; float speed; };
constexpr Drag drag(int min, int max, float speed = 1.0f) { return {min, max, speed}; }

struct String {};
inline static constexpr String string {};

namespace detail {

struct Config
{
    bool color       = false;
    bool color_wheel = false;
    bool is_string   = false; // used for formatting char buffers

    std::variant<Normal, Slider, Drag> scalar_style = Normal{};
};

// Magic spell to make writing a variant visitor nicer.
template <typename... Ts> struct overloaded : Ts... { using Ts::operator()...; };

template <typename T>
concept arithmetic = std::integral<T> || std::floating_point<T>;

template <typename E>
    requires std::is_scoped_enum_v<E>
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

// Ensures that the given meta::info has exactly one of the arithmetic
// visual styles, or none.
consteval bool check_arithmetic_style(std::meta::info info)
{
    std::size_t style_count = 0;
    if (has_annotation<Normal>(info)) { ++style_count; }
    if (has_annotation<Slider>(info)) { ++style_count; }
    if (has_annotation<Drag>(info))   { ++style_count; }
    return style_count < 2;
}

template <std::meta::info info>
constexpr Config get_config()
{
    static_assert(check_arithmetic_style(info), "too many visual styles given for arithmetic type");

    Config config;
    if constexpr (constexpr auto style = fetch_annotation<Normal>(info)) {
        config.scalar_style = *style;
    }
    if constexpr (constexpr auto style = fetch_annotation<Slider>(info)) {
        config.scalar_style = *style;
    }
    if constexpr (constexpr auto style = fetch_annotation<Drag>(info)) {
        config.scalar_style = *style;
    }
    if constexpr (has_annotation<ColorWheel>(info)) {
        config.color_wheel = true;
    }
    if constexpr (has_annotation<Color>(info)) {
        config.color = true;
    }
    if constexpr (has_annotation<String>(info)) {
        config.is_string = true;
    }

    return config;
}

// Forward decls

template <typename T>
bool Render(const char* name, T& val, const Config& config);

template <typename T> requires std::is_scoped_enum_v<T>
bool Render(const char* name, T& value, const Config& config);

template <arithmetic T>
bool Render(const char* name, T& val, const Config& config);

bool Render(const char* name, char& c, const Config& config);
bool Render(const char* name, long double& x, const Config& config);
bool Render(const char* name, bool& value, const Config& config);

template <arithmetic T>
bool Render(const char* name, std::span<T> arr, const Config& config);

template <arithmetic T, std::size_t N> requires (N > 0)
bool Render(const char* name, T (&arr)[N], const Config& config);

template <arithmetic T, std::size_t N> requires (N > 0)
bool Render(const char* name, std::array<T, N>& arr, const Config& config);

bool Render(const char* name, std::string& value, const Config& config);

#ifdef IMREFL_GLM
template <int Size, arithmetic T, glm::qualifier Qual>
bool Render(const char* name, glm::vec<Size, T, Qual>& value, const Config& config);
#endif

template <typename L, typename R>
bool Render(const char* name, std::pair<L, R>& value, const Config& config);

template <typename T>
bool Render(const char* name, std::optional<T>& value, const Config& config);

template <typename T> requires (std::meta::is_aggregate_type(^^T))
bool Render(const char* name, T& x, const Config& config);

// End of forward decls

template <typename T> requires std::is_scoped_enum_v<T>
bool Render(const char* name, T& value, const Config& config)
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
bool Render(const char* name, T& val, const Config& config)
{
    return Render(name, std::span<T>{&val, 1}, config);
}

// Treat char as a single character string, rather than an integral
bool Render(const char* name, char& c, const Config& config)
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
bool Render(const char* name, long double& x, const Config& config)
{
    double temp = static_cast<double>(x);
    if (Render(name, temp, config)) {
        x = temp;
        return true;
    }
    return false;
}

bool Render(const char* name, bool& value, const Config& config)
{
    return ImGui::Checkbox(name, &value);
}

template <arithmetic T>
bool Render(const char* name, std::span<T> arr, const Config& config)
{
    if (arr.empty()) {
        ImGui::Text("span '%s' is of length 0", name);
        return false;
    }

    if constexpr (^^T == ^^char) {
        if (config.is_string) {
            return ImGui::InputText(name, arr.data(), arr.size());
        }
    }

    // Only float permits the color options
    if constexpr (^^T == ^^float) {
        switch (arr.size()) {
            case 3: {
                if (config.color_wheel) {
                    return ImGui::ColorPicker3(name, arr.data());
                } else if (config.color) {
                    return ImGui::ColorEdit3(name, arr.data());
                }
            } break;
            case 4: {
                if (config.color_wheel) {
                    return ImGui::ColorPicker4(name, arr.data());
                } else if (config.color) {
                    return ImGui::ColorEdit4(name, arr.data());
                }
            } break;
        }
    }

    const auto visitor = overloaded{
        [&](Normal) {
            const T step = 1; // Only used for integral types
            return ImGui::InputScalarN(name, num_type<T>(), arr.data(), arr.size(), std::integral<T> ? &step : nullptr);
        },
        [&](Slider slider) {
            const auto min = static_cast<T>(slider.min);
            const auto max = static_cast<T>(slider.max);
            return ImGui::SliderScalarN(name, num_type<T>(), arr.data(), arr.size(), &min, &max);
        },
        [&](Drag drag) {
            const auto min = static_cast<T>(drag.min);
            const auto max = static_cast<T>(drag.max);
            const auto speed = drag.speed;
            return ImGui::DragScalarN(name, num_type<T>(), arr.data(), arr.size(), speed, &min, &max);
        }
    };
    return std::visit(visitor, config.scalar_style);
}

template <arithmetic T, std::size_t N>
    requires (N > 0)
bool Render(const char* name, T (&arr)[N], const Config& config)
{
    return Render(name, std::span<T>{arr, N}, config);
}

template <arithmetic T, std::size_t N>
    requires (N > 0)
bool Render(const char* name, std::array<T, N>& arr, const Config& config)
{
    return Render(name, std::span<T>{arr.begin(), arr.end()}, config);
}

bool Render(const char* name, std::string& value, const Config& config)
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
bool Render(const char* name, glm::vec<Size, T, Qual>& value, const Config& config)
{
    return Render(name, std::span{&value[0], Size}, config);
}
#endif

template <typename L, typename R>
bool Render(const char* name, std::pair<L, R>& value, const Config& config)
{
    ImGui::Text("%s", name);

    const auto width = ImGui::GetContentRegionAvail().x
                     - ImGui::GetStyle().ItemSpacing.x;
    ImGui::PushItemWidth(width / 2.0f);

    bool changed = false;
    changed = changed || Render("first", value.first, config);
    changed = changed || Render("second", value.second, config);

    ImGui::PopItemWidth();
    return changed;
}

template <typename T>
bool Render(const char* name, std::optional<T>& value, const Config& config)
{
    bool changed = false;
    ImGui::PushID(name);
    if (value.has_value()) {
        if (ImGui::Button("Delete")) {
            value = {};
            changed = true;
        } else {
            ImGui::SameLine();
            Render(name, *value, config);
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

template <typename T>
    requires (std::meta::is_aggregate_type(^^T))
bool Render(const char* name, T& x, [[maybe_unused]] const Config& config)
{
    bool changed = false;

    ImGui::Text("%s", name);
    template for (constexpr auto member : nsdm_of<T>()) {
        if constexpr (!has_annotation<Ignore>(member)) {
            // Previous config does not propagate down to the current struct
            const Config new_config = get_config<member>();

            if constexpr (has_annotation<Readonly>(member)) { ImGui::BeginDisabled(); }
            changed = changed || Render(std::meta::identifier_of(member).data(), x.[:member:], new_config);
            if constexpr (has_annotation<Readonly>(member)) { ImGui::EndDisabled(); }
        }
    }

    return changed;
}

template <typename T>
bool Render(const char* name, T& val, const Config& config)
{
    static_assert(false && "not implemented for this type"); 
}

}  // namespace detail

bool Input(const char* name, auto& value)
{
    detail::Config config;
    return detail::Render(name, value, config);
}

}  // namespace ImRefl
