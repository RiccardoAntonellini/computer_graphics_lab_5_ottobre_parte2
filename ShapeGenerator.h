#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

// ============================================================================
// MeshData
// ----------------------------------------------------------------------------
// Contiene SOLO dati CPU.
//
// ShapeGenerator costruisce i vector.
// Geometry si occupa invece di trasferirli sulla GPU.
// ============================================================================

#include "MeshData.h"

// ============================================================================
// ShapeGenerator
// ----------------------------------------------------------------------------
// Funzioni che costruiscono forme geometriche sulla CPU.
// Non contengono chiamate OpenGL.
// ============================================================================

class ShapeGenerator
{
public:

    static MeshData createTriangle(
        const glm::vec4& color1,
        const glm::vec4& color2,
        const glm::vec4& color3
    );


    static MeshData createCircle(
        float cx,
        float cy,
        float radiusX,
        float radiusY,
        int segments,
        const glm::vec4& centerColor,
        const glm::vec4& borderColor
    );


    static MeshData createButterfly(
        float cx,
        float cy,
        float radiusX,
        float radiusY,
        int segments,
        const glm::vec4& centerColor,
        const glm::vec4& borderColor
    );


    static MeshData createHeart(
        float cx,
        float cy,
        float radiusX,
        float radiusY,
        int segments,
        const glm::vec4& centerColor,
        const glm::vec4& borderColor
    );

    static MeshData createMoon(
        float cx,
        float cy,
        float scaleX,
        float scaleY,
        int segments,
        const glm::vec4& color
    );

    static MeshData createSierpinski(
        const glm::vec3& T0,
        const glm::vec3& T1,
        const glm::vec3& T2,
        int nPoints,
        const glm::vec4& color
    );
};
