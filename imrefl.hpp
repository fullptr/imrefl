#ifndef INCLUDED_IMREFL_H
#define INCLUDED_IMREFL_H

#include <imgui.h>
#include <imgui_internal.h>

#include <bitset>
#include <complex>
#include <concepts>
#include <format>
#include <functional>
#include <memory>
#include <meta>
#include <numeric>
#include <optional>
#include <ranges>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace ImRefl {

// ============================================================================
// LIBRARY CORE API
// ============================================================================

// Consteval-only type for passing annotation information to
// Renderer implementations.
struct Config
{
    const std::meta::info* attns     = nullptr;
    const std::size_t      num_attns = 0;

    template <typename T>
    consteval auto FetchAttn() const -> std::optional<T>
    {
        const auto attn_view = std::span<const std::meta::info>{attns, num_attns};
        for (const auto attn : attn_view) {
            if (remove_cvref(type_of(attn)) == remove_cvref(^^T)) {
                return std::meta::extract<T>(attn);
            }
        }
        return {};
    }

    template <typename T>
    consteval auto HasAttn() const -> bool
    {
        return FetchAttn<T>().has_value();
    }
};

// Specialize this struct for different types to enable them for
// rendering via ImRefl.
template <Config config, typename T>
struct Renderer
{
    static_assert(sizeof(T) == 0, "No Renderer implementation for type T");
};

// Specialize this struct for different types by giving it fields
// with names matching fields on the type T. Annotations on fields of
// this type will be used for the corresponding fields on T.
template <typename T>
struct ExternalAnnotations
{};

template <Config config, typename T>
bool Input(const char* name, T&& value)
{
    using Type = [:remove_cvref(^^T):];
    ImGui::PushID(name);
    const bool changed = Renderer<config, Type>::Render(name, std::forward<T>(value));
    ImGui::PopID();
    return changed;
}

// The main entry point for rendering.
template <typename T>
bool Input(const char* name, T&& value)
{
    constexpr auto config = Config{};
    return Input<config>(name, std::forward<T>(value));
}

// ============================================================================
// LIBRARY ANNOTATIONS
// ============================================================================

struct Ignore {};
inline static constexpr Ignore ignore {};

struct Readonly {};
inline static constexpr Readonly readonly {};

struct InLine {};
inline static constexpr InLine in_line {};

struct NonResizable {};
inline static constexpr NonResizable non_resizable {};

struct Separator { const char* title; };
consteval Separator separator(std::string_view title = "") { return {std::define_static_string(title)}; }

struct Color {};
inline static constexpr Color color {};

struct ColorWheel {};
inline static constexpr ColorWheel color_wheel {};

struct Slider { int min; int max; };
constexpr Slider slider(int min, int max) { return {min, max}; }

struct Drag { int min; int max; float speed; };
constexpr Drag drag(int min, int max, float speed = 1.0f) { return {min, max, speed}; }

struct String {};
inline static constexpr String string {};

struct Radio {};
inline static constexpr Radio radio {};

// ============================================================================
// LIBRARY UTILITY 
// ============================================================================

// A wrapper for TreeNodeEx that allows the tree to be opened and collapsed
// even in read-only mode. 
inline bool TreeNodeExNoDisable(const char* label)
{
    const int flags = ImGuiTreeNodeFlags_DefaultOpen;
    const int disabled_levels = ImGui::GetCurrentContext()->DisabledStackSize;
    for (int i = 0; i != disabled_levels; ++i) { ImGui::EndDisabled(); }
    const bool open = ImGui::TreeNodeEx(label, flags);
    for (int i = 0; i != disabled_levels; ++i) { ImGui::BeginDisabled(); }
    return open;
}

// A helper function that renders a const variable by making a mutable
// copy and calling the mutable overload for Render.
template <Config config, typename T>
bool DelegateToNonConst(const char* name, const T& value)
{
    T mutable_value = value;
    ImGui::BeginDisabled();
    Input<config>(name, mutable_value);
    ImGui::EndDisabled();
    return false;
}

// Internal implementation of library; using things in this namespace is
// discouraged as they may change.
namespace detail {

// INTERNAL CONCEPTS

template <typename T>
concept scalar = is_arithmetic_type(^^T) && (^^T != ^^bool);

template <typename T>
concept scoped_enum = is_scoped_enum_type(^^T);

template <typename T>
concept aggregate = is_aggregate_type(^^T);

template<typename T>
concept can_push_pop_back = requires(T t)
{
    { t.emplace_back() };
    { t.pop_back() };
};

template<typename T>
concept can_push_pop_front = requires(T t)
{
    { t.emplace_front() };
    { t.pop_front() };
};

template <typename T>
concept can_emplace = requires(T t, typename T::value_type value)
{
    { t.emplace(value) };
};

template <typename T>
concept can_erase = requires(T t, typename T::const_iterator it)
{
    { t.erase(it) } -> std::convertible_to<typename T::const_iterator>;
};

template <typename T>
concept tuple_like = requires {
    std::tuple_size<T>::value;
};

template <typename T>
concept is_mapping_type = requires {
    typename T::key_type;
    typename T::mapped_type;
};

// INTERNAL HELPERS

consteval auto nsdm_of(std::meta::info type)
{
    const auto ctx = std::meta::access_context::current();
	return std::define_static_array(nonstatic_data_members_of(type, ctx));
}

consteval std::vector<std::meta::info> external_attns(
    std::meta::info parent, std::meta::info member)
{
    const auto external = substitute(^^ExternalAnnotations, {parent});
    for (const auto m : nsdm_of(external)) {
        if (identifier_of(member) == identifier_of(m)) {
            return annotations_of(m);
        }
    }
    return {};
}

consteval auto get_all_attns(std::meta::info parent, std::meta::info member)
{
    auto attns = annotations_of(member);
    
    for (const auto attn : external_attns(parent, member)) {
        attns.push_back(attn);
    }

    return std::define_static_array(attns);
}

consteval auto integer_sequence(std::size_t max)
{
    std::vector<std::size_t> values(max);
    std::iota(values.begin(), values.end(), 0);
    return std::define_static_array(values);
}

consteval auto enums_of(std::meta::info type)
{
    return std::define_static_array(enumerators_of(type));
}

template <detail::scoped_enum T>
constexpr const char* enum_to_string(T value)
{
    template for (constexpr auto e : enums_of(^^T)) {
        if (value == [:e:]) {
            return identifier_of(e).data();
        }
    }
    return "<unnamed>";
}

template <scalar T>
consteval auto num_type()
{
    if constexpr (std::signed_integral<T>) {
        switch (sizeof(T)) {
            case 1: return ImGuiDataType_S8;
            case 2: return ImGuiDataType_S16;
            case 4: return ImGuiDataType_S32;
            case 8: return ImGuiDataType_S64;
        }
    }
    else if constexpr (std::unsigned_integral<T>) {
        switch (sizeof(T)) {
            case 1: return ImGuiDataType_U8;
            case 2: return ImGuiDataType_U16;
            case 4: return ImGuiDataType_U32;
            case 8: return ImGuiDataType_U64;
        }
    }
    else if constexpr (std::floating_point<T>) {
        switch (sizeof(T)) {
            case 4: return ImGuiDataType_Float;
            case 8: return ImGuiDataType_Double;
        }
    }
    throw "unknown scalar type";
}

bool square_button(const char* name)
{
    const float button_size = ImGui::GetFrameHeight();
    return ImGui::Button(name, {button_size, button_size});
}

// Stores an object of type T in static storage and implements a popup
// box for modifying the value. Returns a std::optional<T> containing the
// produced value when the user clicks the Add button.
template <Config config, typename T>
std::optional<T> get_new_value()
{
    static T value {};
    if (square_button("+##get_new_value")) {
        ImGui::OpenPopup("emplace_popup");
    }

    auto return_val = std::optional<T>{};
    if (ImGui::BeginPopup("emplace_popup")) {
        Input<config>("[new key]", value);
        if (ImGui::Button("Add")) {
            ImGui::CloseCurrentPopup();
            return_val = value;        
            value = {};
        }
        ImGui::EndPopup();
    }
    return return_val;
}

// Helper wrapper for std::format_to_n for small strings. Should be used carefully
// and used internally to avoid unnecessary allocations.
struct small_string
{
    char buf[32];
    operator const char*() const { return buf; }
};

template <typename... Args>
small_string fmt(std::format_string<Args...> fmt, Args&&... args)
{
    small_string ret;
    auto result = std::format_to_n(ret.buf, sizeof(ret.buf) - 1, fmt, std::forward<Args>(args)...);
    *result.out = '\0';
    return ret;
}

// INTERNAL RENDERER IMPLEMENTATIONS

template <Config config, typename T>
bool render_pointer_as_value(const char* name, T* value)
{
    if (value) {
        return Input<config>(name, *value);
    } else {
        ImGui::Text("%s: <nullptr>", name);
        return false;
    }
}

template <Config config, std::ranges::forward_range R>
bool render_forward_range(const char* name, R& range)
{
    constexpr auto is_const_range = is_const_type(remove_reference(^^std::ranges::range_reference_t<R>));
    using element_type = std::ranges::range_value_t<R>; 

    bool changed = false;
    if (TreeNodeExNoDisable(name)) {
        if constexpr (!config.HasAttn<NonResizable>() && detail::can_push_pop_front<R>) {
            if (square_button("-##front") && !range.empty()) {
                range.pop_front();
            }
            ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
            if (square_button("+##front")) {
                range.emplace_front();
            }
        }

        size_t i = 0;
        for (auto it = range.begin(); it != range.end();) {
            auto& element = *it;
            const auto index_name = fmt("[{}]", i);

            if constexpr (!is_const_range && std::ranges::random_access_range<R>) {
                changed = Input<config>(fmt("##{}", i), element) || changed;

                ImGui::SameLine();
                const float selectableWidth = ImGui::CalcTextSize(index_name).x;
                ImGui::Selectable(index_name, false, ImGuiSelectableFlags_None, {selectableWidth, 0});
                if (ImGui::BeginDragDropSource()) {
                    ImGui::SetDragDropPayload(name, &i, sizeof(size_t));
                    ImGui::EndDragDropSource();
                }
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(name)) {
                        const size_t index = *(const size_t*)payload->Data;
                        if (index != i) {
                            std::swap(range[index], element);
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
            }
            else {
                changed = Input<config>(index_name, element) || changed;
            }

            if constexpr (detail::can_erase<R>) {
                ImGui::SameLine();
                if (square_button(fmt("-##e{}", i))) {
                    it = range.erase(it);
                } else {
                    ++it;
                }
            } else {
                ++it;
            }
            ++i;
        }

        // TODO: This needs to be made a bit more general. Currently it does not support push_back
        // for types where the element type is not copy assignable, for example
        if constexpr (!config.HasAttn<NonResizable>()) {
            if constexpr (detail::can_push_pop_back<R>) {
                if (!detail::can_push_pop_front<R> || i > 0) {
                    if (square_button("-##back") && !range.empty()) {
                        range.pop_back();
                    }
                    ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
                    if (square_button("+##back")) {
                        range.emplace_back();
                    }
                }
            }

            // TODO: Clean up the type trait use here; not everything is checked
            else if constexpr (detail::can_emplace<R>) {
                if constexpr (std::is_copy_assignable_v<element_type>) {
                    if (auto new_val = get_new_value<config, element_type>()) {
                        range.emplace(*new_val);
                        changed = true; // not necessarily true if the key already exists
                    }
                }

                else if constexpr (is_mapping_type<R>) {
                    using Key = typename R::key_type;
                    using Value = typename R::mapped_type;
                    if (auto new_val = get_new_value<config, Key>()) {
                        range.emplace(*new_val, Value{});
                        changed = true; // not necessarily true if the key already exists
                    }
                }
            }
        }

        ImGui::TreePop();
    }
    return changed;
}

template <Config config, std::ranges::forward_range R>
bool render_forward_range(const char* name, const R& range)
{
    if (TreeNodeExNoDisable(name)) {
        size_t i = 0;
        for (auto& element : range) {
            Input<config>(fmt("[{}]", i), element); 
            ++i;
        }
        ImGui::TreePop();
    }
    return false; 
}

template <Config config, detail::scalar T>
bool render_scalar_n(const char* name, T* val, std::size_t count)
{
    static_assert(!(config.HasAttn<Slider>() && config.HasAttn<Drag>()), "too many visual styles given for scalar type");

    if constexpr (constexpr auto style = config.FetchAttn<Slider>()) {
        const auto min = static_cast<T>(style->min);
        const auto max = static_cast<T>(style->max);
        return ImGui::SliderScalarN(name, detail::num_type<T>(), val, count, &min, &max);
    }
    else if constexpr (constexpr auto style = config.FetchAttn<Drag>()) {
        const auto min = static_cast<T>(style->min);
        const auto max = static_cast<T>(style->max);
        const auto speed = style->speed;
        return ImGui::DragScalarN(name, detail::num_type<T>(), val, count, speed, &min, &max);
    }
    else {
        const T step = 1; // Only used for integral types
        return ImGui::InputScalarN(name, detail::num_type<T>(), val, count, &step);
    }
}

template <Config config, detail::scalar T>
bool render_scalar_n(const char* name, const T* val, std::size_t count)
{
    ImGui::BeginGroup();
    ImGui::PushMultiItemsWidths(count, ImGui::CalcItemWidth());
    for (std::size_t i = 0; i != count; ++i) {
        Input<config>(fmt("##{}", i), val[i]);
        ImGui::PopItemWidth();
        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
    }
    ImGui::Text("%s", name);
    ImGui::EndGroup();
    return false;
}

template <Config config, tuple_like T>
bool render_tuple_like(const char* name, T& value)
{
    bool changed = false;
    ImGui::Text("%s", name);
    template for (constexpr auto index : detail::integer_sequence(tuple_size(^^T))) {
        changed = Input<config>(fmt("##{}", index), std::get<index>(value)) || changed;
    }
    return changed;
}

template <Config config, tuple_like T>
bool render_tuple_like(const char* name, const T& value)
{
    ImGui::Text("%s", name);
    template for (constexpr auto index : detail::integer_sequence(tuple_size(^^T))) {
        Input<config>(fmt("##{}", index), std::get<index>(value));
    }
    return false;
}

} // namespace detail

// ============================================================================
// RENDERER IMPLEMENTATIONS 
// ============================================================================

template <Config config, detail::aggregate T>
struct Renderer<config, T>
{
    static bool Render(const char* name, T& x)
    {
        bool changed = false;
        if (TreeNodeExNoDisable(name)) {
            template for (constexpr auto member : detail::nsdm_of(^^T)) {
                constexpr auto attns = detail::get_all_attns(^^T, member);
                constexpr auto new_config = Config{attns.data(), attns.size()};

                if constexpr (!new_config.HasAttn<Ignore>()) {

                    if constexpr (constexpr auto separator = new_config.FetchAttn<Separator>()) {
                        ImGui::SeparatorText(separator->title);
                    }

                    if constexpr (new_config.HasAttn<Readonly>()) {
                        Input<new_config>(identifier_of(member).data(), std::as_const(x.[:member:]));
                    } else {
                        changed = Input<new_config>(identifier_of(member).data(), x.[:member:]) || changed;
                    }
                }
            }

            ImGui::TreePop();
        }

        return changed;
    }

    static bool Render(const char* name, const T& x)
    {
        if (TreeNodeExNoDisable(name)) {
            template for (constexpr auto member : detail::nsdm_of(^^T)) {
                constexpr auto attns = detail::get_all_attns(^^T, member);
                constexpr auto new_config = Config{attns.data(), attns.size()};

                if constexpr (!new_config.HasAttn<Ignore>()) {

                    if constexpr (constexpr auto separator = new_config.FetchAttn<Separator>()) {
                        ImGui::SeparatorText(separator->title);
                    }

                    Input<new_config>(identifier_of(member).data(), x.[:member:]);
                }
            }

            ImGui::TreePop();
        }

        return false;
    }
};

template <Config config, detail::scoped_enum T>
struct Renderer<config, T>
{
    static bool Render(const char* name, T& value)
    {
        bool changed = false;
        if constexpr (config.HasAttn<Radio>()) {
            ImGui::Text("%s", name);
            template for (constexpr auto e : detail::enums_of(^^T)) {
                ImGui::SameLine();
                if (ImGui::RadioButton(identifier_of(e).data(), value == [:e:])) {
                    value = [:e:];
                    changed = true;
                }
            }
        } else {
            const auto value_name = detail::enum_to_string(value);
            if (ImGui::BeginCombo(name, value_name)) {
                template for (constexpr auto e : detail::enums_of(^^T)) {
                    if (ImGui::Selectable(identifier_of(e).data(), value == [:e:])) {
                        value = [:e:];
                        changed = true;
                    }
                }
                ImGui::EndCombo();
            }
        }
        return changed;
    }

    static bool Render(const char* name, const T& value)
    {
        return DelegateToNonConst<config>(name, value);
    }
};

template <Config config, detail::scalar T>
struct Renderer<config, T>
{
    static bool Render(const char* name, T& value)
    {
        // Treat char as a single character string, rather than an integral
        if constexpr (^^T == ^^char) {
            char buffer[2] = {value, '\0'};
            if (ImGui::InputText(name, buffer, sizeof(buffer))) {
                value = buffer[0];
                return true;
            }
            return false;
        }

        // Treat long double as a simple double as ImGui does not support it natively
        else if constexpr (^^T == ^^long double) {
            double temp = static_cast<double>(value);
            if (Input<config>(name, temp)) {
                value = temp;
                return true;
            }
            return false;
        }

        else {
            return detail::render_scalar_n<config, T>(name, &value, 1);
        }
    }

    static bool Render(const char* name, const T& value)
    {
        return DelegateToNonConst<config>(name, value);
    }
};

template <Config config>
struct Renderer<config, bool>
{
    static bool Render(const char* name, bool& value)
    {
        return ImGui::Checkbox(name, &value);
    }

    static bool Render(const char* name, const bool& value)
    {
        return DelegateToNonConst<config>(name, value);
    }
};

template <Config config, typename T>
struct Renderer<config, T*>
{
    static bool Render(const char* name, T* value)
    {
        return detail::render_pointer_as_value<config, T>(name, value);
    }
};

template <Config config, typename T, std::size_t N> requires (N > 0) 
struct Renderer<config, T[N]>
{
    using Type = T[N];

    static bool Render(const char* name, Type& arr)
    {
        return Input<config, std::span<T>>(name, arr);
    }

    static bool Render(const char* name, const Type& arr)
    {
        return Input<config, std::span<const T>>(name, arr);
    }
};

template <Config config, typename T, std::size_t N> requires (N > 0) 
struct Renderer<config, std::array<T, N>>
{
    static bool Render(const char* name, std::array<T, N>& arr)
    {
        return Input<config, std::span<T>>(name, arr);
    }

    static bool Render(const char* name, const std::array<T, N>& arr)
    {
        return Input<config, std::span<const T>>(name, arr);
    }
};

template <Config config, typename T>
struct Renderer<config, std::span<T>>
{
    static bool Render(const char* name, std::span<T> arr)
    {
        // Chars arrays to be treated as string buffers.
        if constexpr ((^^T == ^^char) && config.HasAttn<String>()) {
            return ImGui::InputText(name, arr.data(), arr.size());
        }
        
        // Float arrays of size 3 and 4 can be treated as colors 
        if constexpr (^^T == ^^float) {
            switch (arr.size()) {
                case 3: {
                    if constexpr (config.HasAttn<ColorWheel>()) {
                        return ImGui::ColorPicker3(name, arr.data());
                    } else if constexpr (config.HasAttn<Color>()) {
                        return ImGui::ColorEdit3(name, arr.data());
                    }
                } break;
                case 4: {
                    if constexpr (config.HasAttn<ColorWheel>()) {
                        return ImGui::ColorPicker4(name, arr.data());
                    } else if constexpr (config.HasAttn<Color>()) {
                        return ImGui::ColorEdit4(name, arr.data());
                    }
                } break;
            }
        }

        // scalar spans can be rendered in a single line if specified.
        if constexpr (detail::scalar<T> && config.HasAttn<InLine>()) {
            return detail::render_scalar_n<config>(name, arr.data(), arr.size());
        }

        return detail::render_forward_range<config>(name, arr);
    }
};

template <Config config, typename T>
struct Renderer<config, std::span<const T>>
{
    static bool Render(const char* name, std::span<const T> arr)
    {
        // Chars arrays to be treated as string buffers.
        if constexpr ((^^T == ^^char) && config.HasAttn<String>()) {
            ImGui::Text("%s: ", name);
            ImGui::SameLine();
            ImGui::TextUnformatted(arr.data(), arr.data() + arr.size());
            return false;
        }
        
        // Float arrays of size 3 and 4 can be treated as colors 
        if constexpr (^^T == ^^float) {
            if (arr.size() == 3 || arr.size() == 4) {
                float copy[4] = {};
                std::copy(arr.begin(), arr.end(), std::begin(copy));
                ImGui::BeginDisabled();
                Input<config>(name, std::span{copy, arr.size()});
                ImGui::EndDisabled();
                return false;
            }
        }

        // scalar spans can be rendered in a single line if specified.
        if constexpr (detail::scalar<T> && config.HasAttn<InLine>()) {
            return detail::render_scalar_n<config>(name, arr.data(), arr.size());
        }

        return detail::render_forward_range<config>(name, arr);
    }
};

template <Config config>
struct Renderer<config, std::string>
{
    static bool Render(const char* name, std::string& value)
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

    static bool Render(const char* name, const std::string& value)
    {
        ImGui::Text("%s: %s", name, value.c_str());
        return false;
    }
};

template <Config config>
struct Renderer<config, std::string_view>
{
    static bool Render(const char* name, std::string_view value)
    {
        ImGui::Text("%s: %.*s", name, static_cast<int>(value.size()), value.data());
        return false;
    }
};

template <Config config, typename L, typename R>
struct Renderer<config, std::pair<L, R>>
{
    static bool Render(const char* name, std::pair<L, R>& value)
    {
        return detail::render_tuple_like<config>(name, value);
    }

    static bool Render(const char* name, const std::pair<L, R>& value)
    {
        return detail::render_tuple_like<config>(name, value);
    }
};

template <Config config, typename... Ts>
struct Renderer<config, std::tuple<Ts...>>
{
    static bool Render(const char* name, std::tuple<Ts...>& value)
    {
        return detail::render_tuple_like<config>(name, value);
    }

    static bool Render(const char* name, const std::tuple<Ts...>& value)
    {
        return detail::render_tuple_like<config>(name, value);
    }
};

template <Config config, typename T>
struct Renderer<config, std::optional<T>>
{
    static bool Render(const char* name, std::optional<T>& value)
    {
        bool changed = false;
        const ImGuiStyle& style = ImGui::GetStyle();
        if (value.has_value()) {
            bool should_remove = false;
            if (ImGui::Button("Remove")) {
                should_remove = true;
            }

            ImGui::SameLine(0, style.ItemInnerSpacing.x);
            ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - (ImGui::GetItemRectSize().x + style.ItemInnerSpacing.x));
            changed = Input<config>(name, *value) || should_remove;

            if (should_remove) {
                value = {};     // Delay this so as not to pass invalid memory to Render
            }
        } else {
            if (ImGui::Button("Add", {ImGui::CalcItemWidth(), ImGui::GetFrameHeight()})) {
                value.emplace();
                changed = true;
            }
            ImGui::SameLine(0, style.ItemInnerSpacing.x);
            ImGui::Text("%s", name);
        }

        return changed;
    }

    static bool Render(const char* name, const std::optional<T>& value)
    {
        const ImGuiStyle& style = ImGui::GetStyle();
        if (value.has_value()) {
            ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - (ImGui::GetItemRectSize().x + style.ItemInnerSpacing.x));
            Input<config>(name, *value);
        } else {
            ImGui::Text("%s: <empty>", name);
        }
        return false;
    }
};

template <Config config, typename... Ts>
struct Renderer<config, std::variant<Ts...>>
{
    static bool Render(const char* name, std::variant<Ts...>& value)
    {
        const ImGuiStyle& style = ImGui::GetStyle();

        static const char* type_names[] = { display_string_of(^^Ts).data()... };
        bool changed = false;

        ImGui::SetNextItemWidth(ImGui::CalcItemWidth() / 3 - style.ItemInnerSpacing.x);
        if (ImGui::BeginCombo("##combo_box", type_names[value.index()])) {
            template for (constexpr auto index : detail::integer_sequence(sizeof...(Ts))) {
                ImGui::PushID(index);
                if (ImGui::Selectable(type_names[index])) {
                    value.template emplace<index>();
                    changed = true;
                }
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        
        ImGui::SameLine(0, style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - (ImGui::GetItemRectSize().x + style.ItemInnerSpacing.x));
        template for (constexpr auto index : detail::integer_sequence(sizeof...(Ts))) {
            if (index == value.index()) {
                changed = Input<config>(name, std::get<index>(value)) || changed;
            }
        }

        return changed;
    }

    static bool Render(const char* name, const std::variant<Ts...>& value)
    {
        ImGui::BeginDisabled();
        const ImGuiStyle& style = ImGui::GetStyle();

        static const char* type_names[] = { display_string_of(^^Ts).data()... };

        ImGui::SetNextItemWidth(ImGui::CalcItemWidth() / 3 - style.ItemInnerSpacing.x);
        if (ImGui::BeginCombo("##combo_box", type_names[value.index()])) {
            template for (constexpr auto index : detail::integer_sequence(sizeof...(Ts))) {
                ImGui::PushID(index);
                ImGui::Selectable(type_names[index]);
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        
        ImGui::SameLine(0, style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - (ImGui::GetItemRectSize().x + style.ItemInnerSpacing.x));
        template for (constexpr auto index : detail::integer_sequence(sizeof...(Ts))) {
            if (index == value.index()) {
                Input<config>(name, std::get<index>(value));
            }
        }

        ImGui::EndDisabled();
        return false;
    }
};

template <Config config, std::ranges::forward_range R>
struct Renderer<config, R>
{
    static bool Render(const char* name, R& range)
    {
        return detail::render_forward_range<config>(name, range);
    }

    static bool Render(const char* name, const R& range)
    {
        return detail::render_forward_range<config>(name, range);
    }
};

template <Config config, typename T, typename Deleter>
struct Renderer<config, std::unique_ptr<T, Deleter>>
{
    static bool Render(const char* name, const std::unique_ptr<T, Deleter>& value)
    {
        return detail::render_pointer_as_value<config, T>(name, value.get());
    }
};

template <Config config, typename T>
struct Renderer<config, std::shared_ptr<T>>
{
    static bool Render(const char* name, const std::shared_ptr<T>& value)
    {
        return detail::render_pointer_as_value<config, T>(name, value.get());
    }
};

template <Config config, typename T>
struct Renderer<config, std::weak_ptr<T>>
{
    static bool Render(const char* name, const std::weak_ptr<T>& value)
    {
        if (value.expired()) {
            ImGui::Text("%s: <expired>", name);
            return false;
        }
        return Input<config>(name, value.lock());
    }
};

template <Config config, typename Return>
struct Renderer<config, std::function<Return()>>
{
    static bool Render(const char* name, const std::function<Return()>& fn)
    {
        if (ImGui::Button(name)) {
            if (fn) { fn(); }
        }
        return false;
    }
};

template <Config config, std::size_t N>
struct Renderer<config, std::bitset<N>>
{
    static bool Render(const char* name, std::bitset<N>& value)
    {
        ImGui::Text("%s", name);
        bool changed = false;
        template for (constexpr auto i : detail::integer_sequence(N)) {
            bool proxy = value[i];
            if constexpr (config.HasAttn<InLine>()) { ImGui::SameLine(); }
            changed = Input<config>(fmt("[{}]", i), proxy) || changed;
            value[i] = proxy;
        }
        return changed;
    }

    static bool Render(const char* name, const std::bitset<N>& value)
    {
        ImGui::Text("%s", name);
        template for (constexpr auto i : detail::integer_sequence(N)) {
            if constexpr (config.HasAttn<InLine>()) { ImGui::SameLine(); }
            Input<config>(fmt("[{}]", i), value[i]);
        }
        return false;
    }
};

template <Config config>
struct Renderer<config, std::source_location>
{
    static bool Render(const char* name, const std::source_location& value)
    {
        ImGui::Text(
            "%s: %s(%u:%u): '%s'",
            name,
            value.file_name(),
            value.line(),
            value.column(),
            value.function_name());
        return false;
    }
};

template <Config config, std::floating_point T>
struct Renderer<config, std::complex<T>>
{
    static bool Render(const char* name, std::complex<T>& value)
    {
        ImGui::Text("%s", name);
        bool changed = false;
        T real = value.real();
        if (Input<config>("Real", real)) {
            value.real(real);
            changed = true;
        }
        T imag = value.imag();
        if (Input<config>("Imag", imag)) {
            value.imag(imag);
            changed = true;
        }
        return changed;
    }

    static bool Render(const char* name, const std::complex<T>& value)
    {
        return DelegateToNonConst<config>(name, value);
    }
};

}  // namespace ImRefl

#endif // INCLUDED_IMREFL_H
