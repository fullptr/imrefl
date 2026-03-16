#include <imgui.h>
#include <imgui_internal.h>

#ifdef IMREFL_GLM
#include <glm/glm.hpp>
#endif

#include <concepts>
#include <format>
#include <memory>
#include <meta>
#include <optional>
#include <utility>
#include <variant>

namespace ImRefl {

struct Ignore {};
inline static constexpr Ignore ignore {};

struct Readonly {};
inline static constexpr Readonly readonly {};

struct InLine {};
inline static constexpr InLine in_line {};

struct NonResizable {};
inline static constexpr NonResizable non_resizable {};

struct Separator { const char* title; };
template <std::size_t N>
constexpr Separator separator(const char (&title)[N]) { return { std::define_static_string(title) }; }
constexpr Separator separator() { return separator(""); }

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

struct Radio {};
inline static constexpr Radio radio {};

namespace detail {

struct ImGuiID
{
    ImGuiID(const char* id) { ImGui::PushID(id); }
    ImGuiID(std::size_t id) { ImGui::PushID(id); }
    ImGuiID(const ImGuiID&) = delete;
    ImGuiID& operator=(const ImGuiID&) = delete;
    ImGuiID(ImGuiID&&) = delete;
    ImGuiID& operator=(ImGuiID&&) = delete;
    ~ImGuiID() { ImGui::PopID(); }
};

struct Config
{
    std::meta::info self = {};
};

// Magic spell to make writing a variant visitor nicer.
template <typename... Ts> struct overloaded : Ts... { using Ts::operator()...; };

template <typename T>
concept scalar = std::meta::is_arithmetic_type(^^T) && (^^T != ^^bool);

template <typename T>
concept scoped_enum = std::meta::is_scoped_enum_type(^^T);

template <typename T>
concept aggregate = std::meta::is_aggregate_type(^^T);

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

template <scoped_enum T>
consteval auto enums_of()
{
    return std::define_static_array(std::meta::enumerators_of(^^T));
}

template <typename T>
consteval auto nsdm_of()
{
    const auto ctx = std::meta::access_context::current();
	return std::define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx));
}

template <scoped_enum T>
constexpr const char* enum_to_string(T value)
{
    template for (constexpr auto e : enums_of<T>()) {
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
        if (std::meta::remove_cvref(std::meta::type_of(a)) == std::meta::remove_cvref(^^T)) {
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

// Forward decls

template <Config config, typename T>
bool Render(const char* name, T& val);

template <Config config, typename T>
bool Render(const char* name, const T& val);

template <Config config, scoped_enum T>
bool Render(const char* name, T& value);

template <Config config, scoped_enum T>
bool Render(const char* name, const T& value);

template <Config config, scalar T>
bool Render(const char* name, T& val);

template <Config config, scalar T>
bool Render(const char* name, const T& val);

// Don't need const versions of these since the const scalar
// overload will delegate to them instead.
template <Config config>
bool Render(const char* name, char& c);

template <Config config>
bool Render(const char* name, long double& x);

template <Config config>
bool Render(const char* name, bool& value);

template <Config config, typename T>
bool Render(const char* name, std::span<T> arr);

template <Config config, typename T>
bool Render(const char* name, std::span<const T> arr);

template <Config config, typename T, std::size_t N> requires (N > 0)
bool Render(const char* name, T (&arr)[N]);

template <Config config, typename T, std::size_t N> requires (N > 0)
bool Render(const char* name, const T (&arr)[N]);

template <Config config, typename T, std::size_t N> requires (N > 0)
bool Render(const char* name, std::array<T, N>& arr);

template <Config config, typename T, std::size_t N> requires (N > 0)
bool Render(const char* name, const std::array<T, N>& arr);

template <Config config, std::ranges::forward_range R>
bool Render(const char* name, R& range);

template <Config config, std::ranges::forward_range R>
bool Render(const char* name, const R& range);

template <Config config>
bool Render(const char* name, std::string& value);

template <Config config>
bool Render(const char* name, const std::string& value);

#ifdef IMREFL_GLM
template <Config config, int Size, scalar T, glm::qualifier Qual>
bool Render(const char* name, glm::vec<Size, T, Qual>& value);

template <Config config, int Size, scalar T, glm::qualifier Qual>
bool Render(const char* name, const glm::vec<Size, T, Qual>& value);
#endif

template <Config config, typename L, typename R>
bool Render(const char* name, std::pair<L, R>& value);

template <Config config, typename L, typename R>
bool Render(const char* name, const std::pair<L, R>& value);

template <Config config, typename T>
bool Render(const char* name, std::optional<T>& value);

template <Config config, typename T>
bool Render(const char* name, const std::optional<T>& value);

template <Config config, typename... Ts>
bool Render(const char* name, std::variant<Ts...>& value);

template <Config config, typename... Ts>
bool Render(const char* name, const std::variant<Ts...>& value);

template <Config config, typename T, typename Deleter>
bool Render(const char* name, std::unique_ptr<T, Deleter>& value);

template <Config config, typename T, typename Deleter>
bool Render(const char* name, const std::unique_ptr<T, Deleter>& value);

template <Config config, typename T>
bool Render(const char* name, std::shared_ptr<T>& value);

template <Config config, typename T>
bool Render(const char* name, const std::shared_ptr<T>& value);

template <Config config, typename T>
bool Render(const char* name, std::weak_ptr<T>& value);

template <Config config, typename T>
bool Render(const char* name, const std::weak_ptr<T>& value);

template <Config config, aggregate T>
bool Render(const char* name, T& x);

template <Config config, aggregate T>
bool Render(const char* name, const T& x);

// End of forward decls

inline bool TreeNodeExNoDisable(const char* label)
{
    const int flags = ImGuiTreeNodeFlags_DefaultOpen;
    const int disabled_levels = ImGui::GetCurrentContext()->DisabledStackSize;
    for (int i = 0; i != disabled_levels; ++i) { ImGui::EndDisabled(); }
    const bool open = ImGui::TreeNodeEx(label, flags);
    for (int i = 0; i != disabled_levels; ++i) { ImGui::BeginDisabled(); }
    return open;
}

template <Config config, std::ranges::forward_range R>
bool RenderForwardRange(const char* name, R& range)
{
    ImGuiID id{name};
    bool changed = false;
    if (TreeNodeExNoDisable(name)) {
        if constexpr (!has_annotation<NonResizable>(config.self) && can_push_pop_front<R>) {
            ImGuiID id{"front"};
            const float button_size = ImGui::GetFrameHeight();
            const ImGuiStyle& style = ImGui::GetStyle();
            if (ImGui::Button("-", {button_size, button_size}) && !range.empty()) {
                range.pop_front();
            }
            ImGui::SameLine(0, style.ItemInnerSpacing.x);
            if (ImGui::Button("+", {button_size, button_size})) {
                range.emplace_front();
            }
        }

        size_t i = 0;
        for (auto& element : range) {
            ImGuiID id(i);
            const std::string index_name = std::format("[{}]", i);
            using element_type = std::remove_reference_t<std::ranges::range_reference_t<R>>;
            if constexpr (!std::is_const_v<element_type> && std::ranges::random_access_range<R>) {
                changed = Render<config>("", element) || changed;
                ImGui::SameLine();
                ImGui::Selectable(index_name.c_str());
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
                changed = Render<config>(index_name.c_str(), element) || changed;
            }
            ++i;
        }

        if constexpr (!has_annotation<NonResizable>(config.self) && can_push_pop_back<R>) {
            if (!can_push_pop_front<R> || i > 0) {
                ImGuiID id{"back"};
                const float button_size = ImGui::GetFrameHeight();
                const ImGuiStyle& style = ImGui::GetStyle();
                if (ImGui::Button("-", {button_size, button_size}) && !range.empty()) {
                    range.pop_back();
                }
                ImGui::SameLine(0, style.ItemInnerSpacing.x);
                if (ImGui::Button("+", {button_size, button_size})) {
                    range.emplace_back();
                }
            }
        }

        ImGui::TreePop();
    }
    return changed;
}

template <Config config, std::ranges::forward_range R>
bool RenderForwardRange(const char* name, const R& range)
{
    ImGuiID id{name};
    if (TreeNodeExNoDisable(name)) {
        size_t i = 0;
        for (auto& element : range) {
            Render<config>(std::format("[{}]", i).c_str(), element); 
            ++i;
        }
        ImGui::TreePop();
    }
    return false; 
}

consteval bool check_scalar_style(std::meta::info info)
{
    std::size_t count = 0;
    if (has_annotation<Normal>(info)) { ++count; }
    if (has_annotation<Slider>(info)) { ++count; }
    if (has_annotation<Drag>(info))   { ++count; }
    return count < 2;
}

template <Config config, scalar T>
bool RenderScalarN(const char* name, T* val, std::size_t count)
{
    static_assert(check_scalar_style(config.self), "too many visual styles given for scalar type");

    if constexpr (constexpr auto style = fetch_annotation<Slider>(config.self)) {
        const auto min = static_cast<T>(style->min);
        const auto max = static_cast<T>(style->max);
        return ImGui::SliderScalarN(name, num_type<T>(), val, count, &min, &max);
    }
    else if constexpr (constexpr auto style = fetch_annotation<Drag>(config.self)) {
        const auto min = static_cast<T>(style->min);
        const auto max = static_cast<T>(style->max);
        const auto speed = style->speed;
        return ImGui::DragScalarN(name, num_type<T>(), val, count, speed, &min, &max);
    }
    else {
        const T step = 1; // Only used for integral types
        return ImGui::InputScalarN(name, num_type<T>(), val, count, &step);
    }
}

// A helper function that renders a const variable by making a mutable
// copy and calling the mutable overload for Render.
template <Config config, typename T>
bool DelegateToNonConst(const char* name, const T& value)
{
    T mutable_value = value;
    ImGui::BeginDisabled();
    Render<config>(name, mutable_value);
    ImGui::EndDisabled();
    return false;
}

template <Config config, scalar T>
bool RenderScalarN(const char* name, const T* val, std::size_t count)
{
    ImGui::BeginGroup();
    ImGui::PushMultiItemsWidths(count, ImGui::CalcItemWidth());
    for (std::size_t i = 0; i != count; ++i) {
        ImGuiID id{i};
        Render<config>("", val[i]);
        ImGui::PopItemWidth();
        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
    }
    ImGui::Text("%s", name);
    ImGui::EndGroup();
    return false;
}

template <Config config, scoped_enum T>
bool Render(const char* name, T& value)
{
    ImGuiID guard{name};
    bool changed = false;
    if constexpr (has_annotation<Radio>(config.self)) {
        ImGui::Text("%s", name);
        template for (constexpr auto e : enums_of<T>()) {
            constexpr auto enum_name = std::meta::identifier_of(e);
            ImGui::SameLine();
            if (ImGui::RadioButton(enum_name.data(), value == [:e:])) {
                value = [:e:];
                changed = true;
            }
        }
    } else {
        const auto value_name = enum_to_string(value);
        if (ImGui::BeginCombo(name, value_name)) {
            template for (constexpr auto e : enums_of<T>()) {
                constexpr auto enum_name = std::meta::identifier_of(e);
                if (ImGui::Selectable(enum_name.data(), value == [:e:])) {
                    value = [:e:];
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }
    }
    return changed;
}

template <Config config, scoped_enum T>
bool Render(const char* name, const T& value)
{
    return DelegateToNonConst<config>(name, value);
}

template <Config config, scalar T>
bool Render(const char* name, T& val)
{
    return RenderScalarN<config>(name, &val, 1);
}

template <Config config, scalar T>
bool Render(const char* name, const T& val)
{
    return DelegateToNonConst<config>(name, val);
}

// Treat char as a single character string, rather than an integral
template <Config config>
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
template <Config config>
bool Render(const char* name, long double& x)
{
    double temp = static_cast<double>(x);
    if (Render<config>(name, temp)) {
        x = temp;
        return true;
    }
    return false;
}

template <Config config>
bool Render(const char* name, bool& value)
{
    return ImGui::Checkbox(name, &value);
}

template <Config config>
bool Render(const char* name, const bool& value)
{
    return DelegateToNonConst<config>(name, value);
}

template <Config config, typename T>
bool Render(const char* name, std::span<T> arr)
{
    // Chars arrays to be treated as string buffers.
    if constexpr ((^^T == ^^char) && has_annotation<String>(config.self)) {
        return ImGui::InputText(name, arr.data(), arr.size());
    }
    
    // Float arrays of size 3 and 4 can be treated as colors 
    if constexpr (^^T == ^^float) {
        switch (arr.size()) {
            case 3: {
                if constexpr (has_annotation<ColorWheel>(config.self)) {
                    return ImGui::ColorPicker3(name, arr.data());
                } else if constexpr (has_annotation<Color>(config.self)) {
                    return ImGui::ColorEdit3(name, arr.data());
                }
            } break;
            case 4: {
                if constexpr (has_annotation<ColorWheel>(config.self)) {
                    return ImGui::ColorPicker4(name, arr.data());
                } else if constexpr (has_annotation<Color>(config.self)) {
                    return ImGui::ColorEdit4(name, arr.data());
                }
            } break;
        }
    }

    // scalar spans can be rendered in a single line if specified.
    if constexpr (scalar<T> && has_annotation<InLine>(config.self)) {
        return RenderScalarN<config>(name, arr.data(), arr.size());
    }

    return RenderForwardRange<config>(name, arr);
}

template <Config config, typename T>
bool Render(const char* name, std::span<const T> arr)
{
    // Chars arrays to be treated as string buffers.
    if constexpr ((^^T == ^^char) && has_annotation<String>(config.self)) {
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
            Render<config>(name, std::span{copy, arr.size()});
            ImGui::EndDisabled();
            return false;
        }
    }

    // scalar spans can be rendered in a single line if specified.
    if constexpr (scalar<T> && has_annotation<InLine>(config.self)) {
        return RenderScalarN<config>(name, arr.data(), arr.size());
    }

    return RenderForwardRange<config>(name, arr);
}

template <Config config, typename T, std::size_t N>
    requires (N > 0)
bool Render(const char* name, T (&arr)[N])
{
    return Render<config>(name, std::span<T>{arr, N});
}

template <Config config, typename T, std::size_t N>
    requires (N > 0)
bool Render(const char* name, const T (&arr)[N])
{
    return Render<config>(name, std::span<const T>{arr, N});
}

template <Config config, typename T, std::size_t N>
    requires (N > 0)
bool Render(const char* name, std::array<T, N>& arr)
{
    return Render<config>(name, std::span<T>{arr});
}

template <Config config, typename T, std::size_t N>
    requires (N > 0)
bool Render(const char* name, const std::array<T, N>& arr)
{
    return Render<config>(name, std::span<const T>{arr});
}

template <Config config, std::ranges::forward_range R>
bool Render(const char* name, R& range)
{
    return RenderForwardRange<config>(name, range);
}

template <Config config, std::ranges::forward_range R>
bool Render(const char* name, const R& range)
{
    return RenderForwardRange<config>(name, range);
}

template <Config config>
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

template <Config config>
bool Render(const char* name, const std::string& value)
{
    ImGui::Text("%s: %s", name, value.c_str());
    return false;
}

#ifdef IMREFL_GLM
template <Config config, int Size, scalar T, glm::qualifier Qual>
bool Render(const char* name, glm::vec<Size, T, Qual>& value)
{
    return Render<config>(name, std::span{&value[0], Size});
}

template <Config config, int Size, scalar T, glm::qualifier Qual>
bool Render(const char* name, const glm::vec<Size, T, Qual>& value)
{
    return Render<config>(name, std::span{&value[0], Size});
}
#endif

template <Config config, typename L, typename R>
bool Render(const char* name, std::pair<L, R>& value)
{
    ImGuiID guard{name};
    ImGui::Text("%s", name);
    const bool first_changed = Render<config>("first", value.first);
    const bool second_changed = Render<config>("second", value.second);
    return first_changed || second_changed;
}

template <Config config, typename L, typename R>
bool Render(const char* name, const std::pair<L, R>& value)
{
    ImGuiID guard{name};
    ImGui::Text("%s", name);
    Render<config>("first", value.first);
    Render<config>("second", value.second);
    return false;
}

template <Config config, typename T>
bool Render(const char* name, std::optional<T>& value)
{
    ImGuiID guard{name};
    bool changed = false;

    const ImGuiStyle& style = ImGui::GetStyle();
    if (value.has_value()) {
        bool should_remove = false;
        if (ImGui::Button("Remove")) {
            should_remove = true;
        }

        ImGui::SameLine(0, style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - (ImGui::GetItemRectSize().x + style.ItemInnerSpacing.x));
        changed = Render<config>(name, *value) || should_remove;

        if (should_remove) {
            value = {};     // Delay this so as not to pass invalid memory to Render
        }
    } else {
        if (ImGui::Button("Add", ImVec2(ImGui::CalcItemWidth(), ImGui::GetFrameHeight()))) {
            value.emplace();
            changed = true;
        }
        ImGui::SameLine(0, style.ItemInnerSpacing.x);
        ImGui::Text("%s", name);
    }

    return changed;
}

template <Config config, typename T>
bool Render(const char* name, const std::optional<T>& value)
{
    ImGuiID guard{name};

    const ImGuiStyle& style = ImGui::GetStyle();
    if (value.has_value()) {
        ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - (ImGui::GetItemRectSize().x + style.ItemInnerSpacing.x));
        Render<config>(name, *value);
    } else {
        ImGui::Text("%s: <empty>", name);
    }
    return false;
}

template <typename... Ts>
struct Tag {};

consteval auto integer_sequence(std::size_t max)
{
    std::vector<std::size_t> values;
    for (std::size_t i = 0; i != max; ++i) {
        values.push_back(i);
    }
    return std::define_static_array(values);
}

template <Config config, typename... Ts>
bool Render(const char* name, std::variant<Ts...>& value)
{
    ImGuiID guard{name};
    const ImGuiStyle& style = ImGui::GetStyle();

    static const char* type_names[] = { std::meta::display_string_of(^^Ts).data()... };
    bool changed = false;

    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() / 3 - style.ItemInnerSpacing.x);
    if (ImGui::BeginCombo("##combo_box", type_names[value.index()])) {
        template for (constexpr auto index : integer_sequence(sizeof...(Ts))) {
            ImGuiID id{index};
            if (ImGui::Selectable(type_names[index])) {
                value.template emplace<index>();
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    
    ImGui::SameLine(0, style.ItemInnerSpacing.x);
    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - (ImGui::GetItemRectSize().x + style.ItemInnerSpacing.x));
    template for (constexpr auto index : integer_sequence(sizeof...(Ts))) {
        if (index == value.index()) {
            changed = Render<config>(name, std::get<index>(value)) || changed;
        }
    }

    return changed;
}

template <Config config, typename... Ts>
bool Render(const char* name, const std::variant<Ts...>& value)
{
    ImGuiID guard{name};
    ImGui::BeginDisabled();
    const ImGuiStyle& style = ImGui::GetStyle();

    static const char* type_names[] = { std::meta::display_string_of(^^Ts).data()... };

    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() / 3 - style.ItemInnerSpacing.x);
    if (ImGui::BeginCombo("##combo_box", type_names[value.index()])) {
        template for (constexpr auto index : integer_sequence(sizeof...(Ts))) {
            ImGuiID id{index};
            ImGui::Selectable(type_names[index]);
        }
        ImGui::EndCombo();
    }
    
    ImGui::SameLine(0, style.ItemInnerSpacing.x);
    ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - (ImGui::GetItemRectSize().x + style.ItemInnerSpacing.x));
    template for (constexpr auto index : integer_sequence(sizeof...(Ts))) {
        if (index == value.index()) {
            Render<config>(name, std::get<index>(value));
        }
    }

    ImGui::EndDisabled();
    return false;
}

template <Config config, typename T, typename Deleter>
bool Render(const char* name, std::unique_ptr<T, Deleter>& value)
{
    if (value) {
        return Render<config>(name, *value);
    } else {
        ImGui::Text("%s: <nullopt>", name);
        return false;
    }
}

template <Config config, typename T, typename Deleter>
bool Render(const char* name, const std::unique_ptr<T, Deleter>& value)
{
    if (value) {
        return Render<config>(name, *value);
    } else {
        ImGui::Text("%s: <nullopt>", name);
        return false;
    }
}

template <Config config, typename T>
bool Render(const char* name, std::shared_ptr<T>& value)
{
    if (value) {
        return Render<config>(name, *value);
    } else {
        ImGui::Text("%s: <nullopt>", name);
        return false;
    }
}

template <Config config, typename T>
bool Render(const char* name, const std::shared_ptr<T>& value)
{
    if (value) {
        return Render<config>(name, *value);
    } else {
        ImGui::Text("%s: <nullopt>", name);
        return false;
    }
}

template <Config config, typename T>
bool Render(const char* name, std::weak_ptr<T>& value)
{
    if (value.expired()) {
        ImGui::Text("%s: <expired>", name);
        return false;
    }
    return Render<config>(name, value.lock());
}

template <Config config, typename T>
bool Render(const char* name, const std::weak_ptr<T>& value)
{
    if (value.expired()) {
        ImGui::Text("%s: <expired>", name);
        return false;
    }
    return Render<config>(name, value.lock());
}

template <Config config, aggregate T>
bool Render(const char* name, T& x)
{
    ImGuiID guard{name};
    bool changed = false;

    if (TreeNodeExNoDisable(name)) {
        template for (constexpr auto member : nsdm_of<T>()) {
            if constexpr (!has_annotation<Ignore>(member)) {
                constexpr auto new_config = Config{ .self=member };

                if constexpr (constexpr auto separator = fetch_annotation<Separator>(member)) {
                    ImGui::SeparatorText(separator->title);
                }

                if constexpr (has_annotation<Readonly>(member)) {
                    Render<new_config>(std::meta::identifier_of(member).data(), std::as_const(x.[:member:]));
                } else {
                    changed = Render<new_config>(std::meta::identifier_of(member).data(), x.[:member:]) || changed;
                }
            }
        }

        ImGui::TreePop();
    }

    return changed;
}

template <Config config, aggregate T>
bool Render(const char* name, const T& x)
{
    ImGuiID guard{name};

    if (TreeNodeExNoDisable(name)) {
        template for (constexpr auto member : nsdm_of<T>()) {
            if constexpr (!has_annotation<Ignore>(member)) {
                constexpr auto new_config = Config{ .self=member };

                if constexpr (constexpr auto separator = fetch_annotation<Separator>(member)) {
                    ImGui::SeparatorText(separator->title);
                }

                Render<new_config>(std::meta::identifier_of(member).data(), x.[:member:]);
            }
        }

        ImGui::TreePop();
    }

    return false;
}

template <Config config, typename T>
bool Render(const char* name, T& val)
{
    static_assert(false && "not implemented for this type"); 
}

}  // namespace detail

bool Input(const char* name, auto& value)
{
    constexpr auto config = detail::Config{ .self=^^value };

    return detail::Render<config>(name, value);
}

}  // namespace ImRefl
