#include "ShapeGenerator.h"
#include <random>
#include <cmath>

namespace
{
    constexpr float PI = 3.14159265358979323846f;
}


MeshData ShapeGenerator::createTriangle(
    const glm::vec4& color1,
    const glm::vec4& color2,
    const glm::vec4& color3)
{
    MeshData data;

    data.positions.push_back(glm::vec3(-0.5f, -0.5f, 0.0f));
    data.positions.push_back(glm::vec3(0.5f, -0.5f, 0.0f));
    data.positions.push_back(glm::vec3(0.0f, 0.5f, 0.0f));
   

    data.colors.push_back(color1);
    data.colors.push_back(color2);
    data.colors.push_back(color3);

    data.primitiveType = GL_TRIANGLES;

    return data;
}


MeshData ShapeGenerator::createCircle(
    float cx,
    float cy,
    float radiusX,
    float radiusY,
    int segments,
    const glm::vec4& centerColor,
    const glm::vec4& borderColor)
{
    // cx e cy sono mantenuti per compatibilita: la posizione viene data dalla Model Matrix.
    (void)cx; (void)cy;
    MeshData data;

    data.positions.reserve(
        segments + 2
    );

    data.colors.reserve(
        segments + 2
    );

    data.positions.push_back(
        glm::vec3(0.0f, 0.0f, 0.0f)
    );

    data.colors.push_back(centerColor);

    const float step =
        2.0f * PI / static_cast<float>(segments);

    for (int i = 0; i <= segments; ++i)
    {
        float t =
            static_cast<float>(i) * step;

        float x =
            radiusX * std::cos(t);

        float y =
            radiusY * std::sin(t);

        data.positions.push_back(
            glm::vec3(x, y, 0.0f)
        );

        data.colors.push_back(borderColor);
    }

    data.primitiveType = GL_TRIANGLE_FAN;

    return data;
}


MeshData ShapeGenerator::createButterfly(
    float cx,
    float cy,
    float radiusX,
    float radiusY,
    int segments,
    const glm::vec4& centerColor,
    const glm::vec4& borderColor)
{
    // cx e cy sono mantenuti per compatibilita: la posizione viene data dalla Model Matrix.
    (void)cx; (void)cy;
    MeshData data;

    data.positions.reserve(
        segments + 2
    );

    data.colors.reserve(
        segments + 2
    );

    data.positions.push_back(
        glm::vec3(0.0f, 0.0f, 0.0f)
    );

    data.colors.push_back(centerColor);

    const float step =
        2.0f * PI / static_cast<float>(segments);

    for (int i = 0; i <= segments; ++i)
    {
        float t =
            static_cast<float>(i) * step;

        float common =
            std::exp(std::cos(t))
            - 2.0f * std::cos(4.0f * t)
            + std::pow(std::sin(t / 12.0f), 5.0f);

        float x =
            radiusX * std::sin(t) * common;

        float y =
            radiusY * std::cos(t) * common;

        data.positions.push_back(
            glm::vec3(x, y, 0.0f)
        );

        data.colors.push_back(borderColor);
    }

    data.primitiveType = GL_TRIANGLE_FAN;

    return data;
}

MeshData ShapeGenerator::createMoon(
    float cx,
    float cy,
    float scaleX,
    float scaleY,
    int segments,
    const glm::vec4& color)
{
    MeshData data;

    (void)cx; (void)cy;
    const float PI = 3.14159265358979323846f;

    for (int i = 0; i <= segments; ++i)
    {
        float t =
            2.0f * PI *
            static_cast<float>(i) /
            static_cast<float>(segments);

        // Equazione parametrica della luna
        float x =
            3.0f * std::sin(t);

        float y =
            0.5f * std::cos(2.0f * t)
            - std::cos(t);

        // Scala e traslazione
        x = scaleX * x;
        y = scaleY * y;

        data.positions.push_back(
            glm::vec3(x, y, 0.0f)
        );

        data.colors.push_back(
            color
        );
    }

    data.primitiveType = GL_TRIANGLE_FAN;

    return data;
}
MeshData ShapeGenerator::createHeart(
    float cx,
    float cy,
    float radiusX,
    float radiusY,
    int segments,
    const glm::vec4& centerColor,
    const glm::vec4& borderColor)
{
    // cx e cy sono mantenuti per compatibilita: la posizione viene data dalla Model Matrix.
    (void)cx; (void)cy;
    MeshData data;

    data.positions.reserve(
        segments + 2
    );

    data.colors.reserve(
        segments + 2
    );

    data.positions.push_back(
        glm::vec3(0.0f, 0.0f, 0.0f)
    );

    data.colors.push_back(centerColor);

    const float step =
        2.0f * PI / static_cast<float>(segments);

    for (int i = 0; i <= segments; ++i)
    {
        float t =
            static_cast<float>(i) * step;

        const float s = std::sin(t);

        float x =
            cx
            + radiusX
            * 16.0f
            * s * s * s;

        float y =
            cy
            + radiusY
            * (
                13.0f * std::cos(t)
                - 5.0f * std::cos(2.0f * t)
                - 2.0f * std::cos(3.0f * t)
                - std::cos(4.0f * t)
              );

        data.positions.push_back(
            glm::vec3(x, y, 0.0f)
        );

        data.colors.push_back(borderColor);
    }

    data.primitiveType = GL_TRIANGLE_FAN;

    return data;
}






MeshData ShapeGenerator::createSierpinski(
    const glm::vec3& T0,
    const glm::vec3& T1,
    const glm::vec3& T2,
    int nPoints,
    const glm::vec4& color)
{
    MeshData data;

    // ------------------------------------------------------------------------
    // 1. I TRE VERTICI DEL TRIANGOLO
    // ------------------------------------------------------------------------

    glm::vec3 vertices[3] =
    {
        T0,
        T1,
        T2
    };


    // ------------------------------------------------------------------------
    // 2. GENERATORE DI NUMERI CASUALI
    // ------------------------------------------------------------------------

    std::random_device rd;

    std::mt19937 generator(
        rd()
    );

    std::uniform_int_distribution<int> distribution(
        0,
        2
    );


    // ------------------------------------------------------------------------
    // 3. PUNTO INIZIALE
    // ------------------------------------------------------------------------
    //
    // Scegliamo inizialmente uno dei tre vertici.
    //

    glm::vec3 currentPoint =
        vertices[
            distribution(generator)
        ];


    // ------------------------------------------------------------------------
    // 4. COSTRUZIONE PROCEDURALE DEI PUNTI
    // ------------------------------------------------------------------------

    for (int i = 0; i < nPoints; ++i)
    {
        // Scegliamo casualmente uno dei tre vertici.
        int vertexIndex =
            distribution(generator);

        glm::vec3 selectedVertex =
            vertices[vertexIndex];


        // ------------------------------------------------------------
        // Calcoliamo il punto medio tra:
        //
        // currentPoint
        //
        // e
        //
        // selectedVertex
        //
        // p_k = (p_{k-1} + T) / 2
        // ------------------------------------------------------------

        currentPoint =
            0.5f *
            (
                currentPoint +
                selectedVertex
                );


        // ------------------------------------------------------------
        // Memorizziamo il nuovo punto.
        // ------------------------------------------------------------

        data.positions.push_back(
            currentPoint
        );

        data.colors.push_back(
            color
        );
    }


    // ------------------------------------------------------------------------
    // 5. TIPO DI PRIMITIVA OPENGL
    // ------------------------------------------------------------------------
    //
    // Il Sierpinski gasket viene visualizzato come una costellazione
    // di punti indipendenti.
    //

    data.primitiveType =
        GL_POINTS;


    return data;
}
