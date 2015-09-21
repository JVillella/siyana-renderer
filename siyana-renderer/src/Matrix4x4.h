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

#ifndef MATRIX4X4_H_INCLUDED
#define MATRIX4X4_H_INCLUDED

#define __OPENCL_HOST__
    #include "Utilities.h"
#undef __OPENCL_HOST__

class Matrix4x4 {
public:
    //-------------------------------------------Data Members
    float m[4][4]; //4 by 4 array
    //-------------------------------------------Constructors
    Matrix4x4(); //Starts off as an identity matrix
    Matrix4x4(float e00, float e01, float e02, float e03,
              float e10, float e11, float e12, float e13,
              float e20, float e21, float e22, float e23,
              float e30, float e31, float e32, float e33);
    //-------------------------------------------Destructors
    ~Matrix4x4() {}
    //-------------------------------------------Copy Constructor
    Matrix4x4(const Matrix4x4 &matrix);
    //-------------------------------------------Operator Overloading
    Matrix4x4 operator* (const Matrix4x4 &mat) const;
    //Point3D operator* (const Point3D &p) const;
    float3 operator* (const float3 &v) const;
    //-------------------------------------------Member Functions
    void SetIdentity(); //Convert matrix to an identity matrix
};

//-----------------------------------------------Constructors
inline
Matrix4x4::Matrix4x4() { //Start off array as an identity array
    for(int x = 0; x < 4; x++) {
        for(int y = 0; y < 4; y++) {
            if(x == y) {
                m[x][y] = 1.0;
            } else {
                m[x][y] = 0.0;
            }
        }
    }
}
inline
Matrix4x4::Matrix4x4(float e00, float e01, float e02, float e03,
                     float e10, float e11, float e12, float e13,
                     float e20, float e21, float e22, float e23,
                     float e30, float e31, float e32, float e33) {
    m[0][0] = e00; m[0][1] = e01; m[0][2] = e02; m[0][3] = e03;
    m[1][0] = e10; m[1][1] = e11; m[1][2] = e12; m[1][3] = e13;
    m[2][0] = e20; m[2][1] = e21; m[2][2] = e22; m[2][3] = e23;
    m[3][0] = e30; m[3][1] = e31; m[3][2] = e32; m[3][3] = e33;
}
//------------------------------------------Copy Constructor
inline
Matrix4x4::Matrix4x4(const Matrix4x4 &matrix) { //Copy array
    for(int x = 0; x < 4; x++) {
        for(int y = 0; y < 4; y++) {
            m[x][y] = matrix.m[x][y];
        }
    }
}
//------------------------------------------Operator Overloading
inline
Matrix4x4 Matrix4x4::operator* (const Matrix4x4 &mat) const {
    Matrix4x4 product; //This Matrix4x4's elements are the product of the two Matrices
    for(int x = 0; x < 4; x++) {
        for(int y = 0; y < 4; y++) {
            float sum = 0.0; //Reset total sum
            for(int i = 0; i < 4; i++) {
                sum += m[x][i] * mat.m[i][y];
            }
            product.m[x][y] = sum; //Assign sum values to product array
        }
    }
    return product; //Return Matrix
}

float3 Matrix4x4::operator* (const float3 &p) const {
    //Add the last column of the matrix to transformed point
    return float3(m[0][0] * p.x + m[0][1] * p.y + m[0][2] * p.z + m[0][3],
                  m[1][0] * p.x + m[1][1] * p.y + m[1][2] * p.z + m[1][3],
                  m[2][0] * p.x + m[2][1] * p.y + m[2][2] * p.z + m[2][3]);
}
/*
inline
float3 Matrix4x4::operator* (const float3 &v) const {
    return float3(m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
                  m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
                  m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z);
}
*/
//-----------------------------------------------Member Functions
inline
void Matrix4x4::SetIdentity() {
    for(int x = 0; x < 4; x++) {
        for(int y = 0; y < 4; y++) {
            if(x == y)
                m[x][y] = 1.0;
            else
                m[x][y] = 0.0;
        }
    }
}
//-----------------------------------------------Non-Member Functions
//Swap matrix rows and columns to get transpose
inline
Matrix4x4 Transpose(const Matrix4x4 &mat) {
    return Matrix4x4(mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
                     mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
                     mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
                     mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]);
}
Matrix4x4 Translate(const float3 &d) {
    Matrix4x4 m(1.0, 0.0, 0.0, d.x,
                0.0, 1.0, 0.0, d.y,
                0.0, 0.0, 1.0, d.z,
                0.0, 0.0, 0.0, 1.0);
    return m;
}
#endif // MATRIX4X4_H_INCLUDED
