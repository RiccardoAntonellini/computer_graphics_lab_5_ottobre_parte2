#pragma once

#include <glad/glad.h>

#include <string>
#include <glm/glm.hpp>

// ============================================================================
// Shader
// ----------------------------------------------------------------------------
// Piccolo wrapper intorno a ShaderMaker.
//
// I path degli shader vengono passati dal main.
// In questo progetto gli shader sono memorizzati nella cartella:
//
//      shader/
//
// Esempio:
//
//      shader.load(
//          "shader/vertexshaderC.glsl",
//          "shader/fragmentshaderC.glsl"
//      );
// ============================================================================

class Shader
{
public:

    Shader() = default;

    ~Shader();


    bool load(
        const char* vertexShaderFile,
        const char* fragmentShaderFile
    );


    void use() const;

    // Imposta una uniform mat4 sul programma shader attualmente attivo.
    void setMat4(const char* name, const glm::mat4& matrix) const;


    GLuint id() const
    {
        return m_programId;
    }


    void destroy();


private:

    GLuint m_programId = 0;
};
