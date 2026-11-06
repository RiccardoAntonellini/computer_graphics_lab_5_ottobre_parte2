TEMPLATE OPENGL 3.3 - CLASSI + TRIANGOLO + DEAR IMGUI
======================================================

Il template include fisicamente nel nuovo progetto:
  dependencies_GL_GLFW\include\glad\glad.h
  dependencies_GL_GLFW\include\KHR\khrplatform.h
  dependencies_GL_GLFW\include\GLFW\glfw3.h
  dependencies_GL_GLFW\lib\glfw3.lib
  dependencies_GL_GLFW\ImGui\...
  dependencies_GL_GLFW\glad.c

Dear ImGui appare in Esplora soluzioni sotto:
  Dipendenze esterne\ImGui\Header Files
  Dipendenze esterne\ImGui\Source Files

GLAD e GLFW sono visibili sotto Dipendenze esterne\GLAD e Dipendenze esterne\GLFW.

GLM NON e' incluso: installarlo tramite NuGet nel progetto appena creato.

Dopo l'installazione di GLM, compilando il progetto viene mostrato:
  - un triangolo colorato;
  - una GUI Dear ImGui;
  - selettore del colore dello sfondo;
  - checkbox Wireframe/Fill.

Configurazione prevista: Visual Studio 2022, x64, C++17, OpenGL 3.3 Core.
