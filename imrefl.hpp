#include <imgui.h>

#include <experimental/meta>
#include <print>
#include <concepts>

namespace ImRefl {

template <std::signed_integral T>
void InputImpl(const char* name, T& x) {
    ImGui::InputInt(name, &x);
}

void InputImpl(const char* name, float& x) {
    ImGui::InputFloat(name, &x);
}

void InputImpl(const char* name, double& x) {
    ImGui::InputDouble(name, &x);
}

template <typename T>
void Input(T& x)
{
    template for (
            constexpr auto member :
	    std::define_static_array(
		std::meta::nonstatic_data_members_of(
		    ^^T,
		    std::meta::access_context::current()
		)
	    )
    ) {
	InputImpl(std::meta::identifier_of(member).data(), x.[:member:]);
    }
}

}
