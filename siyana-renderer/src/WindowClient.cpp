//
//  WindowClient.cpp
//  siyana-renderer
//
//  Created by Julian Villella on 2016-02-20.
//  Copyright © 2016 Julian Villella. All rights reserved.
//

#include "WindowClient.hpp"

#define __OPENCL_HOST__
    #include "Utilities.h"
    #include "Matrix4x4.h"
    #include "BuildScene.h"
    #include "OpenCLUtilities.h"
    #include <math.h>
    #include "boost/thread/thread.hpp"
#undef __OPENCL_HOST__

#include <iostream>
#include "Version.h"

using namespace std;
using namespace boost;

// TODO: Refactor into object-oriented class
// TODO: Bring back terrain generation, and infinite loading
namespace {
    // TODO: OpenCLUtilities hardcodes 4, remove this
    const int kNumChannels = 4; // RGBA
    const int2 kWindowSize(800, 800);
    const int2 kWindowOrigin(100, 100);
    const int kSamplesPerPixel = 1;
    const string kWindowTitle = "Siyana Renderer " +
        SiyanaRenderer::kBuildVersion;
    // Amount camera moves each directional key press
    const float kCameraDisplacementStep = 0.5;
    
    // TODO: Determine a better way to calculate this
    // The distance between the camera origin and the terrain boundary
    //     before generating more terrain
//    const float kGenerateTerrainDistanceThreshold = 100;
}

// Oh my god, why did I ever do so such a thing.
int spp = kSamplesPerPixel;
float* pixels = NULL;
extern Camera cam;

// TODO: Get rid of this enum
enum class CameraDirection {
    Forward, Backward, Up, Down, Left, Right, Undefined
};

void allocateImageBuffer(int width, int height, int numChannels);
void freeImageBuffer();
void resetImageBuffer(GLFWwindow* window);
void resetRender(GLFWwindow* window);

// TODO: Move this functionality into the Camera class
void moveCamera(const CameraDirection& direction, float displacement) {
    float3 axisZ = normalize(cam.focal - cam.eye);
    float3 axisX = normalize(float3(axisZ.z, axisZ.y, -axisZ.x));
    float3 axisY(0.f, 1.f, 0.f);
    
    float3 disp;
    switch(direction) {
        case CameraDirection::Right:
            disp = float3(axisX * displacement);
            break;
        case CameraDirection::Up:
            disp = float3(axisY * displacement);
            break;
        case CameraDirection::Forward:
            disp = float3(axisZ * displacement);
            break;
        case CameraDirection::Left:
            disp = float3(axisX * -displacement);
            break;
        case CameraDirection::Down:
            disp = float3(axisY * -displacement);
            break;
        case CameraDirection::Backward:
            disp = float3(axisZ * -displacement);
            break;
        default:
            break;
    }

    cam.eye   += disp;
    cam.focal += disp;
    
    CameraUpdateOrthoBasis();
}

// TODO: Move this functionality into the Camera class
void rotateCamera(float theta) {
    float3 axisZ = (cam.focal - cam.eye);
    cam.focal.z = cam.eye.z + sin(theta) * axisZ.x + cos(theta) * axisZ.z;
    cam.focal.x = cam.eye.x + cos(theta) * axisZ.x - sin(theta) * axisZ.z;
    
    CameraUpdateOrthoBasis();
}

void errorCallback(int error, const char* description) {
    cerr<<description<<endl;
}

//void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
//    freeImageBuffer();
//    allocateImageBuffer(width, height, kNumChannels);
//    
//    // TODO: Rename this function, it should also be a member function of Camera
//    CameraUpdateOrthoBasis();
//    OpenCLResetRender();
//    
//    glViewport(0, 0, width, height);
//    // TODO: Find a `glutPostRedisplay()` for GLFW3
//}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) {
        return;
    }
    
    // Controls
    //
    // w: forwards
    // s: backwards
    // a: left
    // d: right
    // q: down (-z)
    // e: up (z)
    // esc: exit
    
    CameraDirection direction;
    switch(key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GL_TRUE);
            break;
            
        case GLFW_KEY_UP:
        case GLFW_KEY_W:
            direction = CameraDirection::Forward;
            break;
        
        case GLFW_KEY_DOWN:
        case GLFW_KEY_S:
            direction = CameraDirection::Backward;
            break;
            
        case GLFW_KEY_LEFT:
        case GLFW_KEY_A:
            direction = CameraDirection::Left;
            break;
            
        case GLFW_KEY_RIGHT:
        case GLFW_KEY_D:
            direction = CameraDirection::Right;
            break;
            
        case GLFW_KEY_Q:
            direction = CameraDirection::Down;
            break;
            
        case GLFW_KEY_E:
            direction = CameraDirection::Up;
            break;
    
        default:
            direction = CameraDirection::Undefined;
            break;
    }

    if (direction != CameraDirection::Undefined) {
        moveCamera(direction, kCameraDisplacementStep);
        resetRender(window);
    }
}

void cursorPosCallback(GLFWwindow* window, double xPos, double yPos) {
    // Mouse-camera motion algorithm influenced by Ronny AndrÈ Reierstad
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    
    int xWindowMidpoint = width  >> 1;
    int yWindowMidpoint = height >> 1;
    if(xPos == xWindowMidpoint && yPos == yWindowMidpoint) {
        // TODO: Can we remove this?
        return; // otherwise inifinite loop from `glfwSetCursorPos()`
    }
    glfwSetCursorPos(window, xWindowMidpoint, yWindowMidpoint);
    
    // TODO: Move magic numbers to constants
    float yAngle = (xWindowMidpoint - xPos) / 1000.f; // constant proportional to speed
    float zAngle = (yWindowMidpoint - yPos) / 500.f;
    
    cam.focal.y += zAngle * 2.f; //greater constant = faster look around
    
    // TODO: Move magic numbers to constants
    // Constrain rotation around x-axis
    if (cam.focal.y - cam.eye.y > 8.f) {
        cam.focal.y = cam.eye.y + 8.f;
    } else if (cam.focal.y - cam.eye.y < -8.f) {
        cam.focal.y = cam.eye.y - 8.f;
    }
    
    rotateCamera(yAngle);
    resetRender(window);
}

// TODO: Refactor this method
void drawBuffer(int width, int height, const float* imageBuffer) {
    glEnable(GL_TEXTURE_2D);
    
    glColor3f(1.f, 1.f, 1.f);
    glBindTexture(GL_TEXTURE_2D, 0);
    glPixelStoref(GL_UNPACK_ALIGNMENT, 4);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_FLOAT, imageBuffer);
    
    glBegin(GL_QUADS);
        glTexCoord2f(0.f, 0.f);
        glVertex3f(0.f, 0.f, 0.f);
        
        glTexCoord2f(1.f, 0.f);
        glVertex3f(width, 0.f, 0.f);
        
        glTexCoord2f(1.f, 1.f);
        glVertex3f(width, height, 0.f);
        
        glTexCoord2f(0.f, 1.f);
        glVertex3f(0.f, height, 0.f);
    glEnd();
    
    glDisable(GL_TEXTURE_2D);
}

void drawText(string text, int x, int y) {
    // TODO: Implement me
}

void renderImage(int width, int height) {
    cout<<"Rendering..."<<endl;
    OpenCLRender();
    drawBuffer(width, height, pixels);
    // TODO: Find a `glutPostRedisplay()` for GLFW3
}

void allocateImageBuffer(int width, int height, int numChannels) {
    const int bufferSize = width * height * numChannels * sizeof(float);
    pixels = (float*)malloc(bufferSize);
    memset(pixels, 0, bufferSize);
}

void freeImageBuffer() {
    if (pixels != NULL) {
        free(pixels);
        pixels = NULL;
    }
}

void resetImageBuffer(GLFWwindow* window) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    
    freeImageBuffer();
    allocateImageBuffer(width, height, kSamplesPerPixel);
}

void resetRender(GLFWwindow* window) {
//    resetImageBuffer(window);
    OpenCLResetRender();
}

bool reachedGenerateTerrainThreshold() {
    // TODO: Implement me, and likely requires a different implementation
    return false;
}

GLFWwindow* setupWindow(int *width, int *height) {
    glfwSetErrorCallback(errorCallback);
    
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }
    
    GLFWwindow* window = glfwCreateWindow(kWindowSize.x, kWindowSize.y,
                                          kWindowTitle.c_str(), NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    //    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    glfwSetWindowPos(window, kWindowOrigin.x, kWindowOrigin.y);
    
    glfwGetFramebufferSize(window, width, height);
    allocateImageBuffer(*width, *height, kNumChannels);
    
    return window;
}

void startGui(GLFWwindow* window) {
    double elapsedTime = 0.0;
    while (!glfwWindowShouldClose(window)) {
        double fps = 1.0 / ((glfwGetTime() - elapsedTime) / 1000.0);
        
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        
        glViewport(0, 0, width, height);
        glLoadIdentity();
        glOrtho(0.f, width - 1.f, 0.f, height - 1.f, -1.f, 1.f);
        
        glRasterPos2i(0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        renderImage(width, height);
        
        // TODO: Move on-screen drawaing to method
        drawText("FPS: " + to_string(fps), 10, 10);
        drawText("SPP: " + to_string(spp), 10, 20);
        drawText("Elapsed: " + to_string(elapsedTime), 10, 30);
        
        // TODO: Show camera location
        drawText("Camera -----", 10, 40);
        drawText("Origin: ???", 10, 50);
        drawText("Focal: ???", 10, 60);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
        
        elapsedTime = glfwGetTime();
    }
    
    freeImageBuffer();
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}