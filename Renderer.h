#pragma once

#include <glm/glm.hpp>

class Geometry;
class Scene;
class Shader;

// ============================================================================
// Renderer
// ----------------------------------------------------------------------------
// Questa classe si occupa esclusivamente del rendering.
//
// Responsabilità:
//   - pulire il framebuffer;
//   - impostare wireframe/fill;
//   - disegnare una Geometry;
//   - disegnare tutte le Geometry presenti in una Scene.
//
// La Geometry mantiene comunque il proprio metodo draw().
// Il Renderer lo utilizza, ma non possiede VAO o VBO.
// ============================================================================

class Renderer
{
public:

    // Il Renderer utilizza lo Shader, senza possederne la risorsa OpenGL.
    explicit Renderer(const Shader& shader);

    // Inizia un nuovo frame pulendo il framebuffer.
    void beginFrame(
        const glm::vec4& clearColor
    ) const;


    // Imposta la modalità di rasterizzazione.
    void setWireframe(
        bool enabled
    ) const;


    // Imposta la proiezione ortografica comune a tutta la scena.
    void setProjectionMatrix(const glm::mat4& projection);

    // Disegna una singola geometria.
    void draw(
        const Geometry& geometry
    ) const;

    // Disegna una mesh condivisa con la matrice della singola istanza.
    void draw(const Geometry& geometry, const glm::mat4& model) const;


    // Disegna tutte le geometrie presenti nella scena.
    void drawScene(
        const Scene& scene
    ) const;

private:
    const Shader& m_shader;
    glm::mat4 m_projection = glm::mat4(1.0f);
};
