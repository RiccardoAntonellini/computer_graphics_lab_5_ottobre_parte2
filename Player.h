#pragma once
#include "PlayerShapeLoader.h"
#include "Scene.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// La logica del player e' indipendente dalla rappresentazione grafica.
// Ogni parte ha una geometria e una trasformazione locale rispetto al player.
class Player {
public:
    enum class GeometryMode { OBJ, Primitives };
    struct Part {
        MeshData mesh;
        glm::mat4 localTransform{1.0f};
    };
    // La rappresentazione e' scelta al momento della costruzione.
    // Nessuna trasformazione viene applicata ai vertici caricati dall'OBJ.
    explicit Player(std::string objFilename)
        : m_mode(GeometryMode::OBJ), m_objPath(std::move(objFilename)) {}

    // Il chiamante costruisce liberamente le primitive e le loro matrici locali.
    explicit Player(std::vector<Part> parts)
        : m_mode(GeometryMode::Primitives), m_parts(std::move(parts)) {}

    GeometryMode geometryMode() const { return m_mode; }

    // Legge esclusivamente il percorso OBJ specificato nel costruttore.
    // Le coordinate della mesh non vengono trasformate dal loader.
    bool initializeOBJ(Scene& scene) {
        if (m_mode != GeometryMode::OBJ || m_objPath.empty()) {
            std::cerr << "ERRORE: modalita' OBJ non selezionata o percorso OBJ vuoto.\n";
            return false;
        }
        MeshData mesh;
        std::string error;
        if (!PlayerShapeLoader::load(m_objPath, mesh, error)) {
            std::cerr << "ERRORE: player OBJ/MTL non caricato: " << error << '\n';
            return false;
        }
        const std::size_t triangles = mesh.positions.size() / 3;
        if (!initializeParts(scene, {{std::move(mesh), glm::mat4(1.0f)}})) return false;
        m_loadedOBJ = true;
        std::cout << "Player OBJ caricato: " << m_objPath
                  << " (" << triangles << " triangoli)\n";
        return true;
    }

    // Unico punto di inizializzazione, indipendente dalla rappresentazione.
    bool initialize(Scene& scene) {
        if (m_mode == GeometryMode::OBJ) return initializeOBJ(scene);
        if (!initializeParts(scene, std::move(m_parts))) return false;
        m_parts.clear();
        m_loadedOBJ = false;
        return true;
    }

    void setModelMatrix(Scene& scene, const glm::mat4& model) {
        m_model = model;
        for (std::size_t i = 0; i < m_instances.size(); ++i)
            scene.instance(m_instances[i]).modelMatrix = m_model * m_localTransforms[i];
    }

    bool usesOBJ() const { return m_loadedOBJ; }
    std::size_t instanceIndex() const { return m_instances.empty() ? 0 : m_instances.front(); }
    const glm::mat4& modelMatrix() const { return m_model; }

private:
    bool initializeParts(Scene& scene, std::vector<Part> parts) {
        if (!m_instances.empty() || parts.empty()) return false;
        for (auto& part : parts) {
            if (part.mesh.positions.empty()) return false;
        }
        for (auto& part : parts) {
            const auto geometry = scene.addGeometry(part.mesh);
            m_instances.push_back(scene.addInstance(geometry));
            m_localTransforms.push_back(part.localTransform);
        }
        setModelMatrix(scene, glm::mat4(1.0f));
        return true;
    }

    std::vector<std::size_t> m_instances;
    std::vector<glm::mat4> m_localTransforms;
    glm::mat4 m_model{1.0f};
    GeometryMode m_mode;
    std::string m_objPath;
    std::vector<Part> m_parts;
    bool m_loadedOBJ = false;
};
