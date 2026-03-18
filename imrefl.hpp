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

struct Config
{
    std::meta::info self = {};
};

template <Config config, typename T>
struct Renderer;

template <Config config, typename T>
concept renderable = requires(const char* name, T&& val)
{
    { ImRefl::Renderer<config, T>::Render(name, std::forward<T>(val)) }
        -> std::convertible_to<bool>;
};

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

template <Config config, typename T>
bool RenderPointerAsValue(const char* name, T* value)
{
    if (value) {
        return Renderer<config, T>::Render(name, *value);
    } else {
        ImGui::Text("%s: <nullptr>", name);
        return false;
    }
}

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
    constexpr auto is_const_range = is_const_type(remove_reference(^^std::ranges::range_reference_t<R>));
    using element_type = std::ranges::range_value_t<R>; 

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

            if constexpr (!is_const_range && std::ranges::random_access_range<R>) {
                changed = Renderer<config, element_type>::Render("", element) || changed;
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
                changed = Renderer<config, element_type>::Render(index_name.c_str(), element) || changed;
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
    using element_type = std::ranges::range_value_t<R>; 

    ImGuiID id{name};
    if (TreeNodeExNoDisable(name)) {
        size_t i = 0;
        for (auto& element : range) {
            Renderer<config, element_type>::Render(std::format("[{}]", i).c_str(), element); 
            ++i;
        }
        ImGui::TreePop();
    }
    return false; 
}

consteval bool CheckScalarStyle(std::meta::info info)
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
    static_assert(CheckScalarStyle(config.self), "too many visual styles given for scalar type");

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

template <Config config, scalar T>
bool RenderScalarN(const char* name, const T* val, std::size_t count)
{
    ImGui::BeginGroup();
    ImGui::PushMultiItemsWidths(count, ImGui::CalcItemWidth());
    for (std::size_t i = 0; i != count; ++i) {
        ImGuiID id{i};
        Renderer<config, T>::Render("", val[i]);
        ImGui::PopItemWidth();
        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
    }
    ImGui::Text("%s", name);
    ImGui::EndGroup();
    return false;
}

// A helper function that renders a const variable by making a mutable
// copy and calling the mutable overload for Render.
template <Config config, typename T>
bool DelegateToNonConst(const char* name, const T& value)
{
    T mutable_value = value;
    ImGui::BeginDisabled();
    Renderer<config, T>::Render(name, mutable_value);
    ImGui::EndDisabled();
    return false;
}

// Renderer Implementations

template <Config config, typename T>
struct Renderer<config, std::span<T>>
{
    static bool Render(const char* name, std::span<T> arr)
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
};

template <Config config, typename T>
struct Renderer<config, std::span<const T>>
{
    static bool Render(const char* name, std::span<const T> arr)
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
                Renderer<config, std::span<T>>::Render(name, std::span{copy, arr.size()});
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
};

template <Config config, typename T, std::size_t N> requires (N > 0) 
struct Renderer<config, T[N]>
{
    using Type = T[N];
    static bool Render(const char* name, Type& arr)
    {
        return Renderer<config, std::span<T>>::Render(name, arr);
    }

    static bool Render(const char* name, const Type& arr)
    {
        return Renderer<config, std::span<const T>>::Render(name, arr);
    }
};

template <Config config, typename T, std::size_t N> requires (N > 0) 
struct Renderer<config, std::array<T, N>>
{
    static bool Render(const char* name, std::array<T, N>& arr)
    {
        return Renderer<config, std::span<T>>::Render(name, arr);
    }

    static bool Render(const char* name, const std::array<T, N>& arr)
    {
        return Renderer<config, std::span<const T>>::Render(name, arr);
    }
};

template <Config config, std::ranges::forward_range R>
struct Renderer<config, R>
{
    static bool Render(const char* name, R& range)
    {
        return RenderForwardRange<config>(name, range);
    }

    static bool Render(const char* name, const R& range)
    {
        return RenderForwardRange<config>(name, range);
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

#ifdef IMREFL_GLM
template <Config config, int Size, scalar T, glm::qualifier Qual>
struct Renderer<config, glm::vec<Size, T, Qual>>
{
    static bool Render(const char* name, glm::vec<Size, T, Qual>& value)
    {
        return Renderer<config, std::span<T>>::Render(name, std::span{&value[0], Size});
    }

    static bool Render(const char* name, const glm::vec<Size, T, Qual>& value)
    {
        return Renderer<config, std::span<const T>>::Render(name, std::span{&value[0], Size});
    }
};
#endif

template <Config config, typename L, typename R>
struct Renderer<config, std::pair<L, R>>
{
    static bool Render(const char* name, std::pair<L, R>& value)
    {
        ImGuiID guard{name};
        ImGui::Text("%s", name);
        const bool first_changed = Renderer<config, L>::Render("first", value.first);
        const bool second_changed = Renderer<config, R>::Render("second", value.second);
        return first_changed || second_changed;
    }

    static bool Render(const char* name, const std::pair<L, R>& value)
    {
        ImGuiID guard{name};
        ImGui::Text("%s", name);
        Renderer<config, L>::Render("first", value.first);
        Renderer<config, R>::Render("second", value.second);
        return false;
    }
};

template <Config config, typename T>
struct Renderer<config, std::optional<T>>
{
    static bool Render(const char* name, std::optional<T>& value)
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
            changed = Renderer<config, T>::Render(name, *value) || should_remove;

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

    static bool Render(const char* name, const std::optional<T>& value)
    {
        ImGuiID guard{name};

        const ImGuiStyle& style = ImGui::GetStyle();
        if (value.has_value()) {
            ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - (ImGui::GetItemRectSize().x + style.ItemInnerSpacing.x));
            Renderer<config, T>::Render(name, *value);
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
                changed = Renderer<config, Ts...[index]>::Render(name, std::get<index>(value)) || changed;
            }
        }

        return changed;
    }

    static bool Render(const char* name, const std::variant<Ts...>& value)
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
                Renderer<config, Ts...[index]>::Render(name, std::get<index>(value));
            }
        }

        ImGui::EndDisabled();
        return false;
    }
};

template <Config config, typename T, typename Deleter>
struct Renderer<config, std::unique_ptr<T, Deleter>>
{
    static bool Render(const char* name, std::unique_ptr<T, Deleter>& value)
    {
        return RenderPointerAsValue<config, T>(name, value.get());
    }

    static bool Render(const char* name, const std::unique_ptr<T, Deleter>& value)
    {
        return RenderPointerAsValue<config, T>(name, value.get());
    }
};

template <Config config, typename T>
struct Renderer<config, std::shared_ptr<T>>
{
    static bool Render(const char* name, std::shared_ptr<T>& value)
    {
        return RenderPointerAsValue<config, T>(name, value.get());
    }

    static bool Render(const char* name, const std::shared_ptr<T>& value)
    {
        return RenderPointerAsValue<config, T>(name, value.get());
    }
};

template <Config config, typename T>
struct Renderer<config, std::weak_ptr<T>>
{
    static bool Render(const char* name, std::weak_ptr<T>& value)
    {
        if (value.expired()) {
            ImGui::Text("%s: <expired>", name);
            return false;
        }
        return Renderer<config, std::shared_ptr<T>>::Render(name, value.lock());
    }

    static bool Render(const char* name, const std::weak_ptr<T>& value)
    {
        return DelegateToNonConst<config>(name, value);
    }
};

template <Config config, aggregate T>
struct Renderer<config, T>
{
    static bool Render(const char* name, T& x)
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

                    using element_type = [:remove_const(type_of(member)):];
                    if constexpr (has_annotation<Readonly>(member)) {
                        Renderer<new_config, element_type>::Render(std::meta::identifier_of(member).data(), std::as_const(x.[:member:]));
                    } else {
                        changed = Renderer<new_config, element_type>::Render(std::meta::identifier_of(member).data(), x.[:member:]) || changed;
                    }
                }
            }

            ImGui::TreePop();
        }

        return changed;
    }

    static bool Render(const char* name, const T& x)
    {
        ImGuiID guard{name};

        if (TreeNodeExNoDisable(name)) {
            template for (constexpr auto member : nsdm_of<T>()) {
                if constexpr (!has_annotation<Ignore>(member)) {
                    constexpr auto new_config = Config{ .self=member };

                    if constexpr (constexpr auto separator = fetch_annotation<Separator>(member)) {
                        ImGui::SeparatorText(separator->title);
                    }

                    using element_type = [:remove_const(type_of(member)):];
                    Renderer<new_config, element_type>::Render(std::meta::identifier_of(member).data(), x.[:member:]);
                }
            }

            ImGui::TreePop();
        }

        return false;
    }
};

template <Config config, scalar T>
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
            if (Renderer<config, double>::Render(name, temp)) {
                value = temp;
                return true;
            }
            return false;
        }

        else {
            return RenderScalarN<config, T>(name, &value, 1);
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

template <Config config, scoped_enum T>
struct Renderer<config, T>
{
    static bool Render(const char* name, T& value)
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

    static bool Render(const char* name, const T& value)
    {
        return DelegateToNonConst<config>(name, value);
    }
};

template <typename T>
bool Input(const char* name, T&& value)
{
    using Type = [:remove_cvref(^^T):];
    constexpr auto config = Config{ .self=^^value };
    if constexpr (renderable<config, Type>) {
        return Renderer<config, Type>::Render(name, std::forward<T>(value));
    } else {
        static_assert(false && "not implemented for this type"); 
    }
}

}  // namespace ImRefl
