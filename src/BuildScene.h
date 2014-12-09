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

#ifndef BUILDSCENE_H_INCLUDED
#define BUILDSCENE_H_INCLUDED

//Should put this in BuildScene.cpp but it
//needs to be here for the declarations below
#define __OPENCL_HOST__
    #include "Utilities.h"
    #include "Camera.h"
#undef __OPENCL_HOST__

//terrain constants - used in GlutUtils to determine when to update geometry
const float3 voxel_size = float3(2, 2, 2);
const int3 num_voxels = int3(200, 40, 200);

void SetupScene();
void UpdateScene();
void CleanScene();

#endif //BUILDSCENE_H_INCLUDED
