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

#ifndef CAMERA_H_INCLUDED
#define CAMERA_H_INCLUDED

#define CAMERA_VIEW_DIST 800.f

extern Camera cam;

inline
void CameraSetupOrthoBasis(float3 origin, float3 focal_point) {

    cam.eye = origin;
    cam.focal = focal_point;
    cam.up = float3(0.f, 1.f, 0.f);
    cam.view_dist = CAMERA_VIEW_DIST;

    //cam->focal_plane_dist = distance(cam->eye, cam->focal);
    cam.focal_plane_dist = 4.f;
    cam.f_stop = 150.f; //greater number = longer DoF

    float3 view_dir = normalize(cam.focal - cam.eye);
    cam.w = view_dir; //ONB always in right handed systems
    cam.u = normalize(cross(cam.up, cam.w));
    cam.v = normalize(cross(cam.w, cam.u));

    //check for singularity, if conditions met, orientations must be hardcoded
    if(cam.eye.x == cam.focal.x && cam.eye.z == cam.focal.z
       && cam.focal.y < cam.eye.y) { //if camera looking straight down
        cam.u = float3(0.f, 0.f, 1.f);
        cam.v = float3(1.f, 0.f, 0.f);
        cam.w = float3(0.f, 1.f, 0.f);
    } else if(cam.eye.x == cam.focal.x && cam.eye.z == cam.focal.z
        && cam.focal.y > cam.eye.y) { //if camera looking straight down
    	cam.u = float3(1.f, 0.f, 0.f);
        cam.v = float3(0.f, 0.f, 1.f);
        cam.w = float3(0.f, -1.f, 0.f);
    }
}

inline
void CameraUpdateOrthoBasis() {
    float3 view_dir = normalize(cam.focal - cam.eye);
    //cam->up = normalize(float3(view_dir.x, view_dir.z, -view_dir.y));
    cam.w = view_dir; //ONB always in right handed systems
    cam.u = normalize(cross(cam.up, cam.w));
    cam.v = normalize(cross(cam.w, cam.u));

    //check for singularity, if conditions met, orientations must be hardcoded
    if(cam.eye.x == cam.focal.x && cam.eye.z == cam.focal.z
       && cam.focal.y < cam.eye.y) { //if camera looking straight down
        cam.u = float3(0.f, 0.f, 1.f);
        cam.v = float3(1.f, 0.f, 0.f);
        cam.w = float3(0.f, 1.f, 0.f);
    } else if(cam.eye.x == cam.focal.x && cam.eye.z == cam.focal.z
        && cam.focal.y > cam.eye.y) { //if camera looking straight down
    	cam.u = float3(1.f, 0.f, 0.f);
        cam.v = float3(0.f, 0.f, 1.f);
        cam.w = float3(0.f, -1.f, 0.f);
    }
}

#endif //CAMERA_H_INCLUDED
