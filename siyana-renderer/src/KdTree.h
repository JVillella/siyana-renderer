#ifndef KDTREE_H_INCLUDED
#define KDTREE_H_INCLUDED

#define __OPENCL_HOST__
    #include "Utilities.h"
    #include "BBox.h"
    #include <vector>
    #include <iostream>
#undef __OPENCL_HOST__

using namespace std;

extern TriangleMesh* meshes;
extern int num_meshes;

extern Triangle* triangles;
extern int total_scene_tris;

extern float3* vertices;

extern KdNode* kdnodes;
extern int num_kdnodes;
extern int* kdsorted_tri_indices;
extern int num_kdsorted_tri_indices;

extern BBox scene_bbox;

inline
int GetTriangleID(int mesh_index, int tri_index) {
	int id = 0;
	for(int i = 0; i < mesh_index; i++) {
		id += meshes[i].num_tris;
	}
	id += tri_index;
	return id;
}

inline
void GetMeshAndTri(int id, int* mesh, int* tri) {
    *mesh = 0; *tri = 0;
	int tri_count = -1; //offset
	for(int m = 0; m < num_meshes; m++) {
	    *mesh = m;
		tri_count += meshes[m].num_tris;
        if(tri_count == id) {
            *tri = meshes[m].num_tris - 1;
            return;
		} else if(tri_count > id) {
            *tri = id - (tri_count - (meshes[m].num_tris - 1));
            return;
		}
	}
}

inline
BBox GetBBox(const Triangle& tri) {
    BBox bbox = IdentityBBox();
    bbox = Union(bbox, vertices[tri.vi[0]]);
    bbox = Union(bbox, vertices[tri.vi[1]]);
    bbox = Union(bbox, vertices[tri.vi[2]]);
    return bbox;
}

inline
BBox GetSceneBBox() {
    BBox root = IdentityBBox();
    for(int i = 0; i < total_scene_tris; i++) {
        root = Union(root, GetBBox(triangles[i]));
    }
    return root;
}

inline
void PrintKdTree() {
    cout<<"--- [Printing Kd Tree] --- (NUM: "<<num_kdnodes<<")"<<endl;
    for(int i = 0; i < num_kdnodes; i++) {
        cout<<"ARRAY INDEX: "<<i<<endl;
        if(kdnodes[i].is_leaf) {
            cout<<"\tLEAF NODE"<<endl;
            cout<<"\t\t("<<kdsorted_tri_indices[kdnodes[i].data[TRI_START_INDEX]];
            int end_index = kdnodes[i].data[TRI_START_INDEX] + kdnodes[i].data[NUM_TRIS];
            for(int j = kdnodes[i].data[TRI_START_INDEX] + 1; j < end_index; j++) {
                cout<<", "<<kdsorted_tri_indices[j];
            }
            cout<<")"<<endl;
        } else {
            cout<<"\tINNER NODE"<<endl;
            cout<<"\t\tLeft Child: "<<kdnodes[i].data[LEFT_CHILD]<<endl;
            cout<<"\t\tRight Child: "<<kdnodes[i].data[RIGHT_CHILD]<<endl;
        }
        cout<<"\tSplit Axis "<<kdnodes[i].split_axis<<endl;
        cout<<"\tSplit Posi "<<kdnodes[i].split_pos<<endl;
    }
}

void BuildKdTree();

inline
void ResetKdTree() {
    delete[] kdnodes;
    kdnodes = NULL;
    num_kdnodes = 0;

    delete[] kdsorted_tri_indices;
    kdsorted_tri_indices = NULL;;
    num_kdsorted_tri_indices = 0;
}

#endif // KDTREE_H_INCLUDED
