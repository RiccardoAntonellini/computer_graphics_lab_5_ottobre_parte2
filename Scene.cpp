#include "Scene.h"
#include <stdexcept>

std::size_t Scene::addGeometry(const MeshData& data, bool dynamic) {
    auto g = std::make_unique<Geometry>();
    g->setData(data.positions, data.colors, data.primitiveType);
    g->upload(dynamic);
    m_geometries.push_back(std::move(g));
    return m_geometries.size() - 1;
}

std::size_t Scene::addInstance(std::size_t geometryIndex) {
    if (geometryIndex >= m_geometries.size())
        throw std::out_of_range("Indice Geometry non valido");
    GeometryInstance i;
    i.geometryIndex = geometryIndex;
    m_instances.push_back(i);
    return m_instances.size() - 1;
}

void Scene::clear() {
    m_instances.clear();
    m_geometries.clear(); // i distruttori liberano VAO e VBO
}
