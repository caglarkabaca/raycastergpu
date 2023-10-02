#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cstdint>

#include "Sheesh.h"

class App
{
private:
    GLFWwindow *window;
    int w, h;
    Sheesh* sheesh;
public:
    App(int _w, int _h);
    ~App();
    int init();
    void loop();
};

App::App(int _w, int _h)
{
    w = _w;
    h = _h;
    sheesh = new Sheesh();
}

App::~App()
{
    delete sheesh;
    if (window)
        glfwDestroyWindow(window);
    glfwTerminate();
}

int App::init()
{
    if (!glfwInit())
        return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(w, h, "test", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwSwapInterval(0);
    sheesh->init();
    return 0;
}

bool should_reflesh = true;

void App::loop()
{
    double lastTime = glfwGetTime();
    int frameCount = 0;
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        frameCount++;

        if (deltaTime >= 1.0) {
            double fps = frameCount / deltaTime;
            std::cout << "FPS: " << fps << std::endl;
            frameCount = 0;
            lastTime = currentTime;
        }


        if (should_reflesh) {
            /* Render here */

            glClear(GL_COLOR_BUFFER_BIT);
            sheesh->loop();
            /* Swap front and back buffers */
            glfwSwapBuffers(window);

            should_reflesh = false;
        }


        double sleepTime = (1.f / 144.f) - (glfwGetTime() - currentTime);
        if (sleepTime > 0) {
            glfwWaitEventsTimeout(sleepTime - 0.001);
        }

        /* Poll for and process events */
        glfwPollEvents();
        double frameTime = glfwGetTime() - currentTime;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            sheesh->move(SHEESH_ILERI, frameTime);
            should_reflesh = true;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            sheesh->move(SHEESH_GERI, frameTime);
            should_reflesh = true;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            sheesh->move(SHEESH_SAG, frameTime);
            should_reflesh = true;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            sheesh->move(SHEESH_SOL, frameTime);
            should_reflesh = true;
        }
    }
}