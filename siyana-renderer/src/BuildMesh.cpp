#include "BuildMesh.h"
#include <RPly/rply.h>

//------------------------------------------CallBacks
static int FaceCallBack(p_ply_argument argument) {
    //Get User Data
    void* user_data = NULL; //void pointer
    if(!ply_get_argument_user_data(argument, &user_data, NULL)) {
        cout<<"Error getting user data in FaceCallBack"<<endl;
        exit(-1);
    }
    int* face = *static_cast<int**>(user_data);
    long length;
    long value_index; //v0=0, v1=1, v2=2
    ply_get_argument_property(argument, NULL, &length, &value_index);

    if(length != 3) {
        cout<<"Faces have > 3 vertices...tesselation not supported yet"<<endl;
        exit(-1);
    }

    long face_index = 0; //allows us to keep track of the current index we are on for the face array
    ply_get_argument_element(argument, NULL, &face_index);
    if(value_index >= 0) { //avoids its value of -1
        face[(face_index * 3) + value_index] = static_cast<int>(ply_get_argument_value(argument));
    }
    return 1; //success
}

static int VertexCallBack(p_ply_argument argument) {
    //Get User Data
    long element = 0; //element will point to the arguments idata
    void* user_data = NULL; //user data will point to the arguments *pdata
    if(!ply_get_argument_user_data(argument, &user_data, &element)) {
        cout<<"Error getting user data in VertexCallBack"<<endl;
        exit(-1);
    }
    //vertex points to what user_data is pointing to
    float3* vertex = *static_cast<float3**>(user_data);
    long vertex_index = 0;
    ply_get_argument_element(argument, NULL, &vertex_index);
    vertex[vertex_index][element] = static_cast<float>(ply_get_argument_value(argument));
    return 1; //success
}

static int NormalCallBack(p_ply_argument argument) {
    //Get User Data
    long element = 0; //element will point to the arguments idata
    void* user_data = NULL; //user data will point to the arguments *pdata
    if(!ply_get_argument_user_data(argument, &user_data, &element)) {
        cout<<"Error getting user data in UVCallBack"<<endl;
        exit(-1);
    }
    //vertex points to what user_data is pointing to
    float3* normal = *static_cast<float3**>(user_data);
    long normal_index = 0;
    ply_get_argument_element(argument, NULL, &normal_index);
    normal[normal_index][element] = static_cast<float>(ply_get_argument_value(argument));
    return 1; //success
}

static int UVCallBack(p_ply_argument argument) {
    //Get User Data
    long element = 0; //element will point to the arguments idata
    void* user_data = NULL; //user data will point to the arguments *pdata
    if(!ply_get_argument_user_data(argument, &user_data, &element)) {
        cout<<"Error getting user data in UVCallBack"<<endl;
        exit(-1);
    }
    //vertex points to what user_data is pointing to
    float2* uv = *static_cast<float2**>(user_data);
    long uv_index = 0;
    ply_get_argument_element(argument, NULL, &uv_index);
    uv[uv_index][element] = static_cast<float>(ply_get_argument_value(argument));
    return 1; //success
}

static void ReadPlyFile(const char* filename, vector<int> *faces,
    vector<float3> *vertices, vector<float3> *normals, vector<float2>* uvs) {
    //Open file
    cout<<"Opening "<<filename<<endl;
    p_ply ply = ply_open(filename, NULL, NULL, NULL);
    if(!ply) { //Not a ply file
        cout<<"Error Opening "<<filename<<" (Not a .ply file)"<<endl;
        exit(-1);
    }
    if(!ply_read_header(ply)) {
        cout<<"Error parsing "<<filename<<" header"<<endl;
        exit(-1); //error reading header
    }

    float3* vertex = NULL;
    float3* normal = NULL;
    float2* uv = NULL;
    unsigned int num_x_elements = ply_set_read_cb(ply, "vertex", "x", VertexCallBack, &vertex, 0);
    unsigned int num_y_elements = ply_set_read_cb(ply, "vertex", "y", VertexCallBack, &vertex, 1);
    unsigned int num_z_elements = ply_set_read_cb(ply, "vertex", "z", VertexCallBack, &vertex, 2);
    unsigned int num_nx_elements= ply_set_read_cb(ply, "vertex", "nx",NormalCallBack, &normal, 0);
    unsigned int num_ny_elements= ply_set_read_cb(ply, "vertex", "ny",NormalCallBack, &normal, 1);
    unsigned int num_nz_elements= ply_set_read_cb(ply, "vertex", "nz",NormalCallBack, &normal, 2);
    unsigned int num_s_elements = ply_set_read_cb(ply, "vertex", "s", UVCallBack, &uv, 0); //u
    unsigned int num_t_elements = ply_set_read_cb(ply, "vertex", "t", UVCallBack, &uv, 1); //v
    if(num_x_elements != num_y_elements || num_x_elements != num_z_elements) {
        //number of different vertex elements are not equal
        cout<<"Vertex elements in "<<filename<<" are not equal"<<endl;
        exit(-1);
    }
    if(num_nx_elements != num_ny_elements || num_nx_elements != num_nz_elements) {
        //number of different normal elements are not equal
        cout<<"Normal elements in "<<filename<<" are not equal"<<endl;
        exit(-1);
    }
    if(num_s_elements != num_t_elements) {
        //number of different vertex elements are not equal
        cout<<"UV elements in "<<filename<<" are not equal"<<endl;
        exit(-1);
    }

    //meshes have to have precomputed normals and uvs
    if(num_nx_elements == 0) {
        cout<<"Siyana required precomputed normals...exiting now."<<endl;
        exit(-1);
    }
    if(num_s_elements == 0) {
        cout<<"Siyana required precomputed uvs...exiting now."<<endl;
        exit(-1);
    }

    int* face = NULL;
    int num_faces = ply_set_read_cb(ply, "face", "vertex_indices", FaceCallBack, &face, 0);
    if(num_faces == 0) {
        cout<<filename<<" has no faces"<<endl;
        exit(-1);
    }

    //Allocated memory for vertices
    int num_verts = num_x_elements;
    vertex = new float3[num_verts]; //here we allocated the memory for vertex

    //Allocated memory for vertices
    int num_normals = num_nx_elements;
    normal = new float3[num_normals]; //here we allocated the memory for vertex

    //Allocated memory for uvs
    int num_uvs = num_s_elements;
    uv = new float2[num_uvs]; //here we allocated the memory for vertex

    //Allocated memory for faces
    unsigned int num_face_indices = num_faces * 3;
    face = new int[num_face_indices]; //allocate memory for face

    //Read .ply
    if(!ply_read(ply)) { //if false read is unsuccessful
        cout<<"Error reading .ply"<<endl;
        //Delete allocated memory
        delete[] face;
        face = NULL;
        delete[] vertex;
        vertex = NULL;
        delete[] normal;
        normal = NULL;
        delete[] uv;
        uv = NULL;
        exit(-1);
    }

    //Copy data from arrays into respective vectors
    vertices->reserve(num_verts); //Vertices
    for(unsigned int i = 0; i < num_verts; i++) {
        vertices->push_back(vertex[i]);
    }
    normals->reserve(num_normals); //Vertices
    for(unsigned int i = 0; i < num_normals; i++) {
        normals->push_back(normal[i]);
    }
    uvs->reserve(num_uvs); //UVs
    for(unsigned int i = 0; i < num_uvs; i++) {
        uvs->push_back(uv[i]);
    }
    faces->reserve(num_face_indices); //Face-Vertex Indices
    for(unsigned int i = 0; i < num_face_indices; i++) {
        faces->push_back(face[i]);
    }

    //Close .ply
    ply_close(ply);

    //Delete allocated memory
    delete[] face;
    face = NULL;
    delete[] vertex;
    vertex = NULL;
    delete[] normal;
    normal = NULL;
    delete[] uv;
    uv = NULL;

    cout<<"Finished reading ply file"<<endl;
}

void SetMesh(Mesh* mesh, const char* filename) {
    //obtain vertices and faces
    vector<float3> vertices;
    vector<float3> normals;
    vector<float2> uvs;
    vector<int> faces;
    ReadPlyFile(filename, &faces, &vertices, &normals, &uvs);

    int num_faces = faces.size() / 3;
    //print stats
    cout<<filename<<" has "<<num_faces<<" faces"<<endl;
    cout<<filename<<" has "<<vertices.size()<<" vertices"<<endl;

    //perform copies
    mesh->vertices = vertices;
    mesh->normals = normals;
    mesh->uvs = uvs;
    for(int i = 0; i < num_faces; i++) {
        Triangle tri;
        for(int j = 0; j < 3; j++) {
            tri.vi[j] = faces[i*3 + j];
        }
        mesh->triangles.push_back(tri);
    }
}
