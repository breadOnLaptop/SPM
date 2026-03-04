#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include <string>
#include <GLFW/glfw3.h>

class WindowManager {
public:
    WindowManager();
    ~WindowManager();

    bool init(int width, int height, const std::string& title);
    bool shouldClose() const;
    void startFrame();
    void endFrame();
    void shutdown();

    GLFWwindow* getWindow() const { return window_; }

private:
    GLFWwindow* window_;
    std::string glsl_version_;
};

#endif // WINDOW_MANAGER_H
