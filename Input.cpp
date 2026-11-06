#include <glad/glad.h>
#include "Input.h"


namespace Input
{
    double mouseX = 0.0;
    double mouseY = 0.0;


    void keyCallback(
        GLFWwindow* window,
        int key,
        int scancode,
        int action,
        int mods)
    {
        if (
            key == GLFW_KEY_ESCAPE
            && action == GLFW_PRESS
        )
        {
            glfwSetWindowShouldClose(
                window,
                GLFW_TRUE
            );
        }
    }


    void cursorPositionCallback(
        GLFWwindow* window,
        double xpos,
        double ypos)
    {
        int width;
        int height;

        glfwGetFramebufferSize(
            window,
            &width,
            &height
        );

        mouseX = xpos;

        // GLFW misura y dall'alto verso il basso.
        // La convertiamo nel riferimento usato nella scena.
        mouseY = height - ypos;
    }


    void framebufferSizeCallback(
        GLFWwindow* window,
        int width,
        int height)
    {
        glViewport(
            0,
            0,
            width,
            height
        );
    }
}
