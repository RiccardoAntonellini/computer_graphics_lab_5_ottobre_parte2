#include "Geometry.h"

#include <utility>


Geometry::Geometry(Geometry&& other) noexcept
{
    *this = std::move(other);
}


Geometry& Geometry::operator=(Geometry&& other) noexcept
{
    if (this != &other)
    {
        destroy();

        m_positions = std::move(other.m_positions);
        m_colors = std::move(other.m_colors);

        m_VAO = other.m_VAO;
        m_VBO_positions = other.m_VBO_positions;
        m_VBO_colors = other.m_VBO_colors;

        m_primitiveType = other.m_primitiveType;
        m_uploaded = other.m_uploaded;
        m_model = other.m_model;

        // L'oggetto sorgente non deve più possedere le risorse OpenGL.
        other.m_VAO = 0;
        other.m_VBO_positions = 0;
        other.m_VBO_colors = 0;
        other.m_uploaded = false;
    }

    return *this;
}


Geometry::~Geometry()
{
    destroy();
}


void Geometry::setData(
    const std::vector<glm::vec3>& positions,
    const std::vector<glm::vec4>& colors,
    GLenum primitiveType)
{
    m_positions = positions;
    m_colors = colors;
    m_primitiveType = primitiveType;
}


void Geometry::upload(bool dynamic)
{
    // Se la geometria era già stata caricata, eliminiamo le vecchie risorse.
    destroy();

    const GLenum usage =
        dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;


    // ========================================================================
    // 1. VAO
    // ========================================================================

    glGenVertexArrays(
        1,
        &m_VAO
    );

    glBindVertexArray(m_VAO);


    // ========================================================================
    // 2. VBO POSIZIONI
    // ========================================================================

    glGenBuffers(
        1,
        &m_VBO_positions
    );

    glBindBuffer(
        GL_ARRAY_BUFFER,
        m_VBO_positions
    );

    glBufferData(
        GL_ARRAY_BUFFER,
        m_positions.size() * sizeof(glm::vec3),
        m_positions.data(),
        usage
    );

    // location = 0 -> vec3 position
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(glm::vec3),
        (void*)0
    );

    glEnableVertexAttribArray(0);


    // ========================================================================
    // 3. VBO COLORI
    // ========================================================================

    glGenBuffers(
        1,
        &m_VBO_colors
    );

    glBindBuffer(
        GL_ARRAY_BUFFER,
        m_VBO_colors
    );

    glBufferData(
        GL_ARRAY_BUFFER,
        m_colors.size() * sizeof(glm::vec4),
        m_colors.data(),
        usage
    );

    // location = 1 -> vec4 color
    glVertexAttribPointer(
        1,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(glm::vec4),
        (void*)0
    );

    glEnableVertexAttribArray(1);


    glBindVertexArray(0);

    m_uploaded = true;
}


void Geometry::updatePositions(
    const std::vector<glm::vec3>& newPositions)
{
    m_positions = newPositions;

    if (!m_uploaded)
        return;

    glBindBuffer(
        GL_ARRAY_BUFFER,
        m_VBO_positions
    );

    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        m_positions.size() * sizeof(glm::vec3),
        m_positions.data()
    );
}


void Geometry::updateColors(
    const std::vector<glm::vec4>& newColors)
{
    m_colors = newColors;

    if (!m_uploaded)
        return;

    glBindBuffer(
        GL_ARRAY_BUFFER,
        m_VBO_colors
    );

    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        m_colors.size() * sizeof(glm::vec4),
        m_colors.data()
    );
}


void Geometry::draw() const
{
    if (!m_uploaded)
        return;

    glBindVertexArray(m_VAO);

    glDrawArrays(
        m_primitiveType,
        0,
        vertexCount()
    );
}


void Geometry::destroy()
{
    if (m_VBO_positions != 0)
    {
        glDeleteBuffers(
            1,
            &m_VBO_positions
        );

        m_VBO_positions = 0;
    }

    if (m_VBO_colors != 0)
    {
        glDeleteBuffers(
            1,
            &m_VBO_colors
        );

        m_VBO_colors = 0;
    }

    if (m_VAO != 0)
    {
        glDeleteVertexArrays(
            1,
            &m_VAO
        );

        m_VAO = 0;
    }

    m_uploaded = false;
}

// Ogni istanza conserva la propria trasformazione Object -> World.
void Geometry::setModelMatrix(const glm::mat4& model)
{
    m_model = model;
}

const glm::mat4& Geometry::modelMatrix() const
{
    return m_model;
}
