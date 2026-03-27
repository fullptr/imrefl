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

}  // namespace ImRefl

#endif // INCLUDED_IMREFL_GLM_H
