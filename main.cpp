#include <iostream>
#include <memory>

#include "application.h"
#include "base/glfw_context.h"
#include "base/timer.h"
#include "global.h"
#include "pbr.h"
#include "pcss.h"
#include "prt.h"
#include "ssr.h"

int main(int argc, char* argv[]) {
  Timer timer;
  GlfwContext wnd;

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  wnd.window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
  glfwSetWindowUserPointer(wnd.window, &wnd);
  glfwSetFramebufferSizeCallback(wnd.window,
                                 GlfwContext::framebufferResizeCallback);
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  wnd.glfwExtensions.assign(glfwExtensions,
                            glfwExtensions + glfwExtensionCount);

  std::unique_ptr<Application> app;
  if (argc == 1)
    app = std::make_unique<SsrApplication>(&wnd);
  else {
    std::string name(argv[1]);
    if (name == "pcss")
      app = std::make_unique<PCSSApplication>(&wnd);
    else if (name == "pbr")
      app = std::make_unique<PBRApplication>(&wnd);
    else if (name == "prt")
      app = std::make_unique<PrtApplication>(&wnd);
    else if (name == "ssr")
      app = std::make_unique<SsrApplication>(&wnd);
  }
  app->prepare();
  app->mainLoop(timer);
  app->cleanup();
}