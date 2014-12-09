/************************************************************************
* Copyright (C) 2013  Julian Villella                                   *
*                                                                       *
* This file is part of Siyana Renderer                                  *
*                                                                       *
* This program is free software: you can redistribute it and/or modify  *
* it under the terms of the GNU General Public License as published by  *
* the Free Software Foundation, either version 3 of the License, or     *
* (at your option) any later version.                                   *
*                                                                       *
* This program is distributed in the hope that it will be useful,       *
* but WITHOUT ANY WARRANTY; without even the implied warranty of        *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
* GNU General Public License for more details.                          *
*                                                                       *
* You should have received a copy of the GNU General Public License     *
* along with this program.  If not, see <http://www.gnu.org/licenses/>. *
*                                                                       *
* Website: http://jvillella.com                                         *
*************************************************************************/

#define __OPENCL_HOST__
    #include "Utilities.h"
    #include "Matrix4x4.h"
    #include "GLUTUtilities.h"
    #include "BuildScene.h"
    #include <math.h>
    #include <boost/thread/thread.hpp>
#undef __OPENCL_HOST__

#include <iostream>
using namespace std;
using namespace boost;

int width = 645;
int height = 365;
int spp = 0;
float* pixels = NULL;
extern Camera cam;

namespace {
    //***For Rendering***
    float time_elapsed = 0.f; //for fps count
    bool camera_moved = false;
    bool resize = false;
    bool rendered_low_pass = false;
    const int LOW_PASS_DIV = 1;

    //***For Terrain Generation***
    //if eye moved this much from previous position...rebuild terrain
    float MAX_TERRAIN_EYE_DISPLACEMENT;
    float3 old_cam_eye;
    thread terrain_thread;
    bool geometry_updating;
    bool request_scene_reset;

    //***For Movement***
    const float WALK_SPEED = 0.3f;
    //FOR DEBUGGING// const float WALK_SPEED = 3.f;
};

//------------------------------------------Camera Data Movement
static void Move(int axis) { //1 = x-axis forward, -1 = x-axis backwards, etc.
    float3 z_vec = normalize(cam.focal - cam.eye);
    float3 x_vec = normalize(float3(z_vec.z, z_vec.y, -z_vec.x));
    float3 y_vec(0.f, 1.f, 0.f);

    float3 disp; //displacement vec
    switch(axis) {
        case 1:
            disp = float3(x_vec * WALK_SPEED);
            break;
        case 2:
            disp = float3(y_vec * WALK_SPEED);
            break;
        case 3:
            disp = float3(z_vec * WALK_SPEED);
            break;
        case -1:
            disp = float3(x_vec * -WALK_SPEED);
            break;
        case -2:
            disp = float3(y_vec * -WALK_SPEED);
            break;
        case -3:
            disp = float3(z_vec * -WALK_SPEED);
            break;
        default:
            break;
    }
    //add displacement
    cam.eye += disp;
    cam.focal += disp;
}
static void Rotate(float theta) {
    float3 z_vec = (cam.focal - cam.eye);
    cam.focal.z = (float)(cam.eye.z + sin(theta)*z_vec.x + cos(theta)*z_vec.z);
    cam.focal.x = (float)(cam.eye.x + cos(theta)*z_vec.x - sin(theta)*z_vec.z);
}
//------------------------------------------Utilities
static void AllocateLocalImageMem(void) {
    pixels = (float*)malloc(width * height * 4 * sizeof(float));
    int bytes = (width * height * 4 * sizeof(float));
    memset(pixels, 0, bytes); //set pixel colour
}
static void FreeLocalImageMem(void) {
    if(pixels != NULL) {
        printf("GLUT: Local pixel array freed\n");
        free(pixels);
        pixels = NULL;
    }
}

static void DrawString(const char* string) {
    int c = 0; //char counter
    while(string[c] != '\0') { //null terminator
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_10, string[c]);
        c++;
    }
}
static void SetupViewport(void) {
    glViewport(0, 0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
    glLoadIdentity(); //reset matrix
    //multiply current matrix by orthographic matrix, set clipping planes
    glOrtho(0.f, glutGet(GLUT_WINDOW_WIDTH) - 1.f, 0.f, glutGet(GLUT_WINDOW_HEIGHT) - 1.f, -1.f, 1.f);
}
//------------------------------------------GLUT Callbacks
static void Resize_cb(int new_width, int new_height) {
    resize = true;

    //modify globals
    width = new_width;
    height = new_height;

    printf("GLUT: resize_cb called.\n");
    printf("\tnew res: %ix%i\n", width, height);

    //reset viewport with new dimensions
    SetupViewport();

    glutPostRedisplay(); //multiple calls generate only single
}
static void Keyboard_cb(unsigned char key, int, int) {
    /* --- CONTROLS ---
     * w = forwards
     * s = backwards
     * a = left
     * d = right
     * q = down (-z)
     * e = up (z)
     * ------------- */
    int dir = 0;

    switch(key) {
        case 27: //esc key
            cout<<"GLUT: Esc pressed - program exited"<<endl;
            exit(0); break;
        case 'w':
            dir = 3; break;
        case 's':
            dir = -3; break;
        case 'a':
            dir = -1; break;
        case 'd':
            dir = 1; break;
        case 'q':
            dir = -2; break;
        case 'e':
            dir = 2; break;
        default:
            break;
    }

    if(dir != 0) {
        Move(dir);
        camera_moved = true;
        glutPostRedisplay(); //multiple calls generate only single
    }
}

//Mouse-camera motion algorithm influenced by Ronny André Reierstad
static void Mouse_cb(int x, int y) {
    //cout<<"GLUT: Passive Motion Callback (mouse_cb) invoked"<<endl;
    int window_mid_x = glutGet(GLUT_WINDOW_WIDTH)  >> 1; //>> 1 = div 2
    int window_mid_y = glutGet(GLUT_WINDOW_HEIGHT) >> 1;
    if(x == window_mid_x && y == window_mid_y) {
        return; //otherwise inifinite loop from glutWarp...
    }
    glutWarpPointer(window_mid_x, window_mid_y);
    float angle_y = (window_mid_x - x) / 1000.f; //constant proport. to speed
    float angle_z = (window_mid_y - y) / 500.f;

    cam.focal.y += angle_z * 2.f; //greater constant = faster look around

    //limit rotation around x-axis
    if((cam.focal.y - cam.eye.y) > 8.f) {
        cam.focal.y = cam.eye.y + 8.f;
    }
    if((cam.focal.y - cam.eye.y) < -8.f) {
        cam.focal.y = cam.eye.y - 8.f;
    }

    Rotate(angle_y);

    camera_moved = true;
    glutPostRedisplay(); //multiple calls generate only single
}

static void StartSceneUpdate() {
    geometry_updating = true; //start generation
    UpdateScene();
    geometry_updating = false; //done terrain generation
    request_scene_reset = true;
}

static void BackgroundProcess_cb(void) {
    if(resize) { //full reset needed
        cout<<"Resizing now..."<<endl;
        cam.view_dist = CAMERA_VIEW_DIST; //put back to default
        FreeLocalImageMem();
        AllocateLocalImageMem();
        CameraUpdateOrthoBasis();
        OpenCLResetRender();
        resize = false; //redraw complete
    } else if(camera_moved) { //render low pass
        //cout<<"Camera was moved"<<endl;
        //cout<<"\tOld - New Cam Distance: "<<dist(old_cam_eye, cam.eye)<<endl;
        if(dist(old_cam_eye, cam.eye) > MAX_TERRAIN_EYE_DISPLACEMENT) { //need to reset terrain
            cout<<"***About to generate new terrain***"<<endl;
            //NOTE: Old issue was that we were shrinking the dimensions then doing a full OpenCL reset.
            //The rand_states for example was then being allocated for the shrinked size. Then when we
            //switch to the high pass we are accessing unalocated memory. The resizing works because we
            //are setting the high pass (the max) and the low pass is always less...we never go higher.
            //The easiest solution (what we did) is to do two resets. They're quick...not a big deal.
            //***make sure we allocate the max sizes***
            if(!geometry_updating) { //if not ready don't generate more terrain
                old_cam_eye = cam.eye; //set new cam eye point
                terrain_thread = thread(StartSceneUpdate);
            } else {
                cout<<"Attempted to generate more terrain but system was not ready. Will try again."<<endl;
            }
        }

        //set low pass render settings
        rendered_low_pass = true;
        width = glutGet(GLUT_WINDOW_WIDTH) / LOW_PASS_DIV;
        height= glutGet(GLUT_WINDOW_HEIGHT) / LOW_PASS_DIV;
        cam.view_dist = CAMERA_VIEW_DIST / LOW_PASS_DIV;
        CameraUpdateOrthoBasis();
        OpenCLResetRender();
        camera_moved = false;
    } else { //just update needed
        if(rendered_low_pass) {
            width = glutGet(GLUT_WINDOW_WIDTH);
            height= glutGet(GLUT_WINDOW_HEIGHT);
            cam.view_dist = CAMERA_VIEW_DIST;
            CameraUpdateOrthoBasis();
            OpenCLResetRender();
            rendered_low_pass = false;
        }
    }
    if(request_scene_reset) {
        OpenCLResetGeometry(); //full reset needed
        request_scene_reset = false;
    }
    OpenCLRender(); //render pass
    glutPostRedisplay(); //show new render on-screen
}

static void DrawImage(void) {
    glColor3f(1.f, 1.f, 1.f);
    glEnable(GL_TEXTURE_2D); //enable tex. mapping
    glBindTexture(GL_TEXTURE_2D, 0);
    glPixelStoref(GL_UNPACK_ALIGNMENT, 4);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, pixels);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0, 0.0);
        glVertex3f(0.0, 0.0, 0.0);

        glTexCoord2f(1.0, 0.0);
        glVertex3f(glutGet(GLUT_WINDOW_WIDTH), 0.0, 0.0);

        glTexCoord2f(1.0, 1.0);
        glVertex3f(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT), 0.0);

        glTexCoord2f(0.0, 1.0);
        glVertex3f(0.0, glutGet(GLUT_WINDOW_HEIGHT), 0.0);
    glEnd();
    glDisable(GL_TEXTURE_2D); //disable tex. mapping
}

static void Display_cb(void) {
    if(resize) {
        return;
    }
    //cout"GLUT: Display_cb called"<<endl;
    glClear(GL_COLOR_BUFFER_BIT);
    glRasterPos2i(0, 0); //set raster drawing position
    DrawImage();
    //draw text
    glColor3f(0.05f, 0.05f, 0.05f);
    glRasterPos2i(10, glutGet(GLUT_WINDOW_HEIGHT) - 10); //top
    DrawString("Siyana Renderer v0.5 | OpenCL Raytracer and Terrain Gen. | By: Julian Villella");

    glRasterPos2i(10, 20); //bottom
    char str_spp[40];
    sprintf(str_spp, "Samples Per Pixel: %i", spp);
    DrawString(str_spp);

    glRasterPos2i(10, 10); //bottom
    char str_fps[40];
    float fps = 1.f/((glutGet(GLUT_ELAPSED_TIME) - time_elapsed)/1000.f);
    sprintf(str_fps, "Frames Per Second: %f", fps);
    time_elapsed = glutGet(GLUT_ELAPSED_TIME); //set new time
    DrawString(str_fps);

    //swap buffers, for double buffering
    glutSwapBuffers();
}

//------------------------------------------GLUT Wrappers
static void SetupWindow(int argc, char **argv) {
    glutInit(&argc, argv); //parses window-system specific parameters
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowSize(width, height);
    glutInitWindowPosition(150, 50);
    /*int win_id = */glutCreateWindow("Siyana Renderer v0.5 - January 2013");
}
static void SetGLUTCallbacks(void) {
    //set callbacks
    glutDisplayFunc(Display_cb);
    glutReshapeFunc(Resize_cb);
    glutKeyboardFunc(Keyboard_cb);
    glutMotionFunc(Mouse_cb);
    glutPassiveMotionFunc(Mouse_cb);
    glutIdleFunc(BackgroundProcess_cb);
}
void SetupGLUT(int argc, char** argv) {
    AllocateLocalImageMem(); //allocate pixel array
    SetupWindow(argc, argv);
    SetGLUTCallbacks();
    SetupViewport();
    glutSetCursor(GLUT_CURSOR_NONE);

    old_cam_eye = cam.eye;
    geometry_updating = false;
    request_scene_reset = false;
    float3 dimensions = voxel_size * float3(num_voxels);
    float min_length = min(dimensions.x, dimensions.z); //don't factor in y-axis
    MAX_TERRAIN_EYE_DISPLACEMENT = min_length*0.25f; //must be less than half
    cout<<"MAX_TERRAIN_EYE_DISPLACEMENT = "<<MAX_TERRAIN_EYE_DISPLACEMENT<<endl;
}
void GLUTCleanUp(void) {
    FreeLocalImageMem(); //free pixel mem
}
