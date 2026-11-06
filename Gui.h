#pragma once
#include <glm/glm.hpp>
struct GLFWwindow;

// Esercitazione 7: parametri controllabili dalla GUI.
class Gui
{
public:
    void initialize(GLFWwindow* window);
    void beginFrame();
    void draw();
    void render();
    void shutdown();

    const glm::vec4& clearColor() const { return m_clearColor; }
    bool wireframe() const { return m_wireframe; }
    float bulletSpeed() const { return m_bulletSpeed; }
    float butterflySpeed() const { return m_butterflySpeed; }
    int butterflyCount() const { return m_butterflyCount; }
    int heartCount() const { return m_heartCount; }
    bool resetRequested();
    void setScore(int score) { m_score = score; }
    void setActiveProjectiles(int n) { m_activeProjectiles = n; }

private:
    glm::vec4 m_clearColor{0.035f, 0.09f, 0.18f, 1.0f};
    bool m_initialized = false;
    bool m_wireframe = false;
    float m_bulletSpeed = 105.0f;
    float m_butterflySpeed = 18.0f;
    int m_butterflyCount = 7;
    int m_heartCount = 5;
    int m_score = 0;
    int m_activeProjectiles = 0;
    bool m_resetRequested = false;
};
