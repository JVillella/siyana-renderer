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

#ifndef TEXTURE_H_INCLUDED
#define TEXTURE_H_INCLUDED

#define __OPENCL_HOST__
    #include "Utilities.h"
#undef __OPENCL_HOST__

#include "SOIL.h"
#include <iostream>
using namespace std;

//------------------------------------------Texture Functions
inline
void SetTexture(vector<unsigned char>* tex_vec, Texture* tex, float4 col) {
    //set struct properties
    tex->start_index = tex_vec->size();
    tex->width = 1;
    tex->height = 1;
    tex->channel = 3;

    tex_vec->reserve(tex_vec->size() + 3);
    for(int c = 0; c < 3; c++) { //per channel
        tex_vec->push_back((unsigned char)(col[c] * 255));
    }
}

inline
void SetTexture(vector<unsigned char>* tex_vec, Texture* tex, const char* filename) {
    //gather image data from file
    int w, h, chan; //chan = channels
    unsigned char* image_data = SOIL_load_image(filename, &w, &h, &chan, SOIL_LOAD_RGB);
    chan = 3; //we forced channels up above to 3. the pointer is by default set to the original
    //set struct properties
    tex->start_index = tex_vec->size();
    tex->width = w;
    tex->height = h;
    tex->channel = chan;
    tex_vec->reserve(tex_vec->size() + w * h * chan);
    int i = 0;
    for(int y = 0; y < h; y++) {
        for(int x = 0; x < w; x++) {
            for(int c = 0; c < chan; c++) {
                tex_vec->push_back(image_data[i]);
                i++;
            }
        }
    }
}

#endif // TEXTURE_H_INCLUDED
