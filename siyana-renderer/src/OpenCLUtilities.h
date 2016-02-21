#ifndef OPENCLUTILITIES_H_INCLUDED
#define OPENCLUTILITIES_H_INCLUDED

#ifdef __APPLE__
	#include <OpenCL/opencl.h>
#else
	#include <CL/cl.h>
#endif

//------------------------------------------Procedure Wrappers
void SetupOpenCL(int _width, int _height);
void OpenCLCleanUp(void);
void OpenCLRender(void);
void OpenCLFullReset(void);
void OpenCLResetRender(void);
void OpenCLResetGeometry(void);

#endif //OPENCLUTILITIES_H_INCLUDED
