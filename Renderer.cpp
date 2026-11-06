#include "Renderer.h"

#include <glad/glad.h>

#include "Geometry.h"
#include "Scene.h"
#include "Shader.h"

Renderer::Renderer(const Shader& shader) : m_shader(shader) {}


void Renderer::beginFrame(
    const glm::vec4& clearColor) const
{
    glClearColor(
        clearColor.r,
        clearColor.g,
        clearColor.b,
        clearColor.a
    );

    glClear(
        GL_COLOR_BUFFER_BIT
    );
}


void Renderer::setWireframe(
    bool enabled) const
{
    if (enabled)
    {
        glPolygonMode(
            GL_FRONT_AND_BACK,
            GL_LINE
        );
    }
    else
    {
        glPolygonMode(
            GL_FRONT_AND_BACK,
            GL_FILL
        );
    }
}


void Renderer::setProjectionMatrix(const glm::mat4& projection)
{
    m_projection = projection;
}

void Renderer::draw(const Geometry& geometry) const {
    draw(geometry, geometry.modelMatrix());
}

void Renderer::draw(const Geometry& geometry, const glm::mat4& model) const {
    m_shader.use();
    m_shader.setMat4("model", model);
    m_shader.setMat4("projection", m_projection);
    geometry.draw();
}

void Renderer::drawScene(const Scene& scene) const {
    for (const GeometryInstance& i : scene.instances()) {
        if (!i.visible) continue;
        draw(scene.geometry(i.geometryIndex), i.modelMatrix);
    }
}
