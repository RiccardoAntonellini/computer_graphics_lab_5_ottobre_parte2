#include "Gui.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <../ImGui/imgui.h>
#include <../ImGui/imgui_impl_glfw.h>
#include <../ImGui/imgui_impl_opengl3.h>

void Gui::initialize(GLFWwindow* window)
{
    if (m_initialized) return;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    m_initialized = true;
}
void Gui::beginFrame()
{
    if (!m_initialized) return;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}
void Gui::draw()
{
    ImGui::SetNextWindowPos(ImVec2(12, 12), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(380, 370), ImGuiCond_FirstUseEver);
    ImGui::Begin("Cannone 2D - Farfalle, cuori e proiettili");
    ImGui::TextWrapped("Il cannone ruota verso il cursore del mouse.");
    ImGui::Separator();
    ImGui::Text("Farfalle e proiettili");
    ImGui::SliderInt("Numero farfalle", &m_butterflyCount, 0, 15);
    ImGui::SliderInt("Numero cuori", &m_heartCount, 0, 15);
    ImGui::SliderFloat("Velocita' farfalle", &m_butterflySpeed, 4.0f, 45.0f);
    ImGui::SliderFloat("Velocita' proiettili", &m_bulletSpeed, 30.0f, 200.0f);
    ImGui::Text("Punteggio: %d", m_score);
    ImGui::Text("Proiettili attivi: %d", m_activeProjectiles);
    ImGui::TextWrapped("Mira con il mouse. Spara con click sinistro o SPAZIO.");
    if (ImGui::Button("Ripristina scena")) m_resetRequested = true;
    ImGui::Separator();
    ImGui::ColorEdit3("Sfondo", &m_clearColor.r);
    ImGui::Checkbox("Wireframe", &m_wireframe);
    ImGui::End();
}
bool Gui::resetRequested()
{
    const bool result = m_resetRequested;
    m_resetRequested = false;
    return result;
}
void Gui::render()
{
    if (!m_initialized) return;
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
void Gui::shutdown()
{
    if (!m_initialized) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}
