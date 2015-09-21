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

#ifndef MARCHINGCUBES_H_INCLUDED
#define MARCHINGCUBES_H_INCLUDED

#define __OPENCL_HOST__
    #include "Utilities.h"
    #include "BuildMesh.h" //for intermediate Mesh class
#undef __OPENCL_HOST__

//------------------------------------------Kernel
void GenerateTerrain(Mesh* mesh, const float3& voxel_size, const int3& num_voxels);

#endif // MARCHINGCUBES_H_INCLUDED
