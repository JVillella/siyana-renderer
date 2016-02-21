//
//  WindowClient.hpp
//  siyana-renderer
//
//  Created by Julian Villella on 2016-02-20.
//  Copyright © 2016 Julian Villella. All rights reserved.
//

#ifndef WindowClient_hpp
#define WindowClient_hpp

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

GLFWwindow* setupWindow(int *width, int *height);
void startGui(GLFWwindow* window);

#endif /* WindowClient_hpp */
