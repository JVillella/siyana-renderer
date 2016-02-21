#ifndef GLUTUTILITIES_H_INCLUDED
#define GLUTUTILITIES_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include "OpenCLUtilities.h"

void SetupGLUT(int argc, char** argv);
void GLUTCleanUp(void);

#endif //GLUTUTILITIES_H_INCLUDED
