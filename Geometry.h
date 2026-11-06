#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <vector>

// ============================================================================
// Geometry
// ----------------------------------------------------------------------------
// Rappresenta UNA geometria OpenGL.
//
// La classe contiene:
//   - i dati CPU: vector<vec3> positions e vector<vec4> colors
//   - le risorse GPU: VAO, VBO delle posizioni, VBO dei colori
//   - il tipo di primitiva da disegnare
//
// L'idea è:
//      vector sulla CPU  --->  upload()  --->  VBO sulla GPU
//
// Il VAO ricorda:
//      location 0 -> VBO delle posizioni
//      location 1 -> VBO dei colori
//
// La classe è volutamente semplice da utilizzare.
// ============================================================================

class Geometry
{
public:

    Geometry() = default;

    // Una Geometry possiede risorse OpenGL.
    // Evitiamo quindi copie accidentali di VAO/VBO.
    Geometry(const Geometry&) = delete;
    Geometry& operator=(const Geometry&) = delete;

    // Consentiamo invece lo spostamento.
    Geometry(Geometry&& other) noexcept;
    Geometry& operator=(Geometry&& other) noexcept;

    ~Geometry();


    // ------------------------------------------------------------------------
    // DATI CPU
    // ------------------------------------------------------------------------

    void setData(
        const std::vector<glm::vec3>& positions,
        const std::vector<glm::vec4>& colors,
        GLenum primitiveType
    );


    // ------------------------------------------------------------------------
    // UPLOAD CPU -> GPU
    // ------------------------------------------------------------------------
    //
    // dynamic = false  -> GL_STATIC_DRAW
    // dynamic = true   -> GL_DYNAMIC_DRAW
    //
    void upload(bool dynamic = false);


    // Aggiorna solo il VBO delle posizioni.
    // Utile per una geometria animata, come il cuore.
    void updatePositions(
        const std::vector<glm::vec3>& newPositions
    );


    // Aggiorna solo il VBO dei colori.
    void updateColors(
        const std::vector<glm::vec4>& newColors
    );


    // ------------------------------------------------------------------------
    // RENDERING
    // ------------------------------------------------------------------------

    // Ogni oggetto Geometry possiede il proprio metodo di disegno.
    //
    // L'oggetto sa:
    //      - quale VAO attivare;
    //      - quale primitiveType usare;
    //      - quanti vertici disegnare.
    //
    // Il Renderer può quindi semplicemente chiamare geometry.draw().
    void draw() const;


    // ------------------------------------------------------------------------
    // ACCESSO AI DATI CPU
    // ------------------------------------------------------------------------

    std::vector<glm::vec3>& positions()
    {
        return m_positions;
    }

    std::vector<glm::vec4>& colors()
    {
        return m_colors;
    }

    const std::vector<glm::vec3>& positions() const
    {
        return m_positions;
    }

    const std::vector<glm::vec4>& colors() const
    {
        return m_colors;
    }


    int vertexCount() const
    {
        return static_cast<int>(m_positions.size());
    }


    // Matrice di modellazione: trasforma le coordinate locali in coordinate mondo.
    void setModelMatrix(const glm::mat4& model);
    const glm::mat4& modelMatrix() const;

    void destroy();


private:

    std::vector<glm::vec3> m_positions;
    std::vector<glm::vec4> m_colors;

    GLuint m_VAO = 0;
    GLuint m_VBO_positions = 0;
    GLuint m_VBO_colors = 0;

    GLenum m_primitiveType = GL_TRIANGLES;

    bool m_uploaded = false;

    glm::mat4 m_model = glm::mat4(1.0f);
};
