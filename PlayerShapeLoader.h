#pragma once
// Loader OBJ/MTL 2D: triangola le facce poligonali a ventaglio (OBJ triangolati consigliati).
// MeshData usa triangoli non indicizzati e colori per vertice, come Scene/Geometry.
#include "MeshData.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace PlayerShapeLoader {
inline std::string directoryOf(const std::string& path) {
    const auto pos = path.find_last_of("/\\");
    return pos == std::string::npos ? std::string() : path.substr(0, pos + 1);
}
inline bool readMTL(const std::string& filename, std::unordered_map<std::string, glm::vec4>& materials) {
    std::ifstream file(filename);
    if (!file) return false;
    std::string line, current;
    while (std::getline(file, line)) {
        std::istringstream in(line); std::string tag; in >> tag;
        if (tag == "newmtl") { in >> current; materials[current] = glm::vec4(1.0f); }
        else if (tag == "Kd" && !current.empty()) {
            float r, g, b;
            if (in >> r >> g >> b) materials[current] = glm::vec4(r, g, b, 1.0f);
        }
        else if (tag == "d" && !current.empty()) { float a; if (in >> a) materials[current].a = a; }
    }
    return true;
}
inline bool load(const std::string& filename, MeshData& mesh, std::string& error) {
    std::ifstream file(filename);
    if (!file) { error = "Impossibile aprire OBJ: " + filename; return false; }
    std::vector<glm::vec3> vertices;
    std::unordered_map<std::string, glm::vec4> materials;
    glm::vec4 currentColor(0.3f, 0.7f, 0.95f, 1.0f);
    MeshData result; result.primitiveType = GL_TRIANGLES;
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream in(line); std::string tag; in >> tag;
        if (tag == "v") {
            glm::vec3 v(0.0f);
            if (in >> v.x >> v.y) { in >> v.z; vertices.push_back(v); }
        } else if (tag == "mtllib") {
            std::string name; while (in >> name) readMTL(directoryOf(filename) + name, materials);
        } else if (tag == "usemtl") {
            std::string name; in >> name;
            auto it = materials.find(name);
            currentColor = it != materials.end() ? it->second : glm::vec4(0.3f, 0.7f, 0.95f, 1.0f);
        } else if (tag == "f") {
            std::vector<int> face; std::string token;
            while (in >> token) {
                try {
                    int index = std::stoi(token.substr(0, token.find('/')));
                    if (index == 0) { face.clear(); break; }
                    index = index > 0 ? index - 1 : static_cast<int>(vertices.size()) + index;
                    if (index < 0 || index >= static_cast<int>(vertices.size())) { face.clear(); break; }
                    face.push_back(index);
                } catch (...) { face.clear(); break; }
            }
            for (std::size_t k = 1; k + 1 < face.size(); ++k) {
                const glm::vec3 a = vertices[face[0]], b = vertices[face[k]], c = vertices[face[k + 1]];
                const float cross = (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);
                if (std::abs(cross) < 1e-10f) continue;
                result.positions.push_back(a); result.positions.push_back(b); result.positions.push_back(c);
                result.colors.insert(result.colors.end(), 3, currentColor);
            }
        }
    }
    if (result.positions.empty()) { error = "OBJ senza facce triangolari valide: " + filename; return false; }
    // Nessuna centratura, scalatura o modifica delle coordinate OBJ.
    mesh = std::move(result);
    error.clear();
    return true;
}
}
