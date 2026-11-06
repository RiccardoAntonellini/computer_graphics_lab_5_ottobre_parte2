#pragma once
#include "Geometry.h"
#include "ShapeGenerator.h"
#include <vector>
#include <memory>
#include <cstddef>

// Una Geometry contiene VAO e VBO: la mesh GPU e' caricata una sola volta.
// Una GeometryInstance contiene soltanto un indice di mesh, una Model Matrix
// e un flag di visibilita'. Piu' istanze possono usare la stessa mesh.
struct GeometryInstance {
    std::size_t geometryIndex = 0;
    glm::mat4 modelMatrix{1.0f};
    bool visible = true;
};

class Scene {
public:
    // Registra e carica una sola volta una nuova mesh nella GPU.
    std::size_t addGeometry(const MeshData& data, bool dynamic = false);
    // Crea una nuova occorrenza di una mesh gia' registrata.
    std::size_t addInstance(std::size_t geometryIndex);
    GeometryInstance& instance(std::size_t index) { return m_instances.at(index); }
    const GeometryInstance& instance(std::size_t index) const { return m_instances.at(index); }
    const Geometry& geometry(std::size_t index) const { return *m_geometries.at(index); }
    const std::vector<GeometryInstance>& instances() const { return m_instances; }
    std::size_t geometryCount() const { return m_geometries.size(); }
    std::size_t instanceCount() const { return m_instances.size(); }
    void clear();
private:
    // unique_ptr mantiene stabile la Geometry anche quando il vector cresce.
    std::vector<std::unique_ptr<Geometry>> m_geometries;
    std::vector<GeometryInstance> m_instances;
};
