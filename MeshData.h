#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

// ============================================================================
// MeshData
// Contenitore dei dati geometrici sulla CPU.
// ============================================================================
struct MeshData
{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec4> colors;
    GLenum primitiveType = GL_TRIANGLES;
};
