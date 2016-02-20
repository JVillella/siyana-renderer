#include "KdTree.h"

#define COST_ISECT 1.f
#define COST_TRAVERSAL 5.6f
#define EMPTY_BONUS 0.f

//c++ deprecated file-scope variables in favour
//of anonymous namespace...acts the same.
namespace {
    vector<int> kdsorted_tri_vec;
    vector<KdNode> nodes_vec;
    vector<BBox> tri_bboxes;
    int kd_index = 0;
}

struct EdgeBound {
    //Data Members
    float pos;
    int side; //near side = 0, far side = 1
    //Functions
    EdgeBound(float _pos, int _side) : pos(_pos), side(_side) {}
    bool operator< (const EdgeBound& e) const {
        if(pos == e.pos) {
            return (side < e.side); //let sides determine tie breaker
        } else {
            return (pos < e.pos);
        }
    }
};

float SplitPlaneDistance(int split_axis, const BBox& bbox) {
    float median_dist =  0.5f * fabs(bbox.max[split_axis] - bbox.min[split_axis]);
    return bbox.min[split_axis] + median_dist;
}

float SAHGetSplitCost(const BBox& bbox, float edge_pos,
    int split_axis, int num_tris_left, int num_tris_right) {
    //
    float invParentSA = 1.f / SurfaceArea(bbox);
    //probability equation for determining to continue splitting
    float rel_empty_bonus = (num_tris_right == 0 || num_tris_left == 0) ? EMPTY_BONUS : 0.f;
//cout<<"EDGE POS: "<<edge_pos<<endl;
    BBox left_bbox = bbox; left_bbox.max[split_axis] = edge_pos;
    BBox right_bbox= bbox;right_bbox.min[split_axis] = edge_pos;
    float p_left = SurfaceArea(left_bbox) * invParentSA;
    float p_right= SurfaceArea(right_bbox)* invParentSA;
//cout<<"Surface Area Below BBox: "<<SurfaceArea(below_bbox)<<endl;
    return COST_TRAVERSAL + COST_ISECT * (1.f - rel_empty_bonus) *
        (p_left * num_tris_left + p_right * num_tris_right);
}

float SAHGetLeafCost(int num_tris) {
    return COST_ISECT * num_tris;
}

float SAHSplitPlaneDistance(int split_axis, const BBox& bbox,
    const vector<int> &tris, float* best_cost) {
    //
    float running_cost = INFINITY;
    float best_split = -1;
    *best_cost = INFINITY;

    //setup edges
    vector<EdgeBound> edges;//edges of tri bboxes inside this bbox
    edges.reserve(2 * tris.size());
    for(int i = 0; i < (int)tris.size(); i++) {
        edges.push_back(EdgeBound(tri_bboxes[tris[i]].min[split_axis], 0));
        edges.push_back(EdgeBound(tri_bboxes[tris[i]].max[split_axis], 1));
    }
    sort(edges.begin(), edges.end()); //sort edges (small to max) default < oper.

    //deal with costs
    int num_tris_left = 0;
    int num_tris_right = tris.size();
    for(int i = 0; i < (int)edges.size(); i++) {
        //if edge is within parent bbox
        if(edges[i].side == 1) { num_tris_right--; } //far side
        if(edges[i].pos > bbox.min[split_axis] && edges[i].pos < bbox.max[split_axis]) {
            running_cost = SAHGetSplitCost(bbox, edges[i].pos,
                split_axis, num_tris_left, num_tris_right);
            if(running_cost < *best_cost) {
                *best_cost = running_cost;
                best_split = edges[i].pos;
            }
        }
        if(edges[i].side == 0) {num_tris_left++;} //near side
    }
    return best_split;
}

static int RecursiveBuild(const vector<int> &tris, const BBox &bbox, int depth) {
    //Create new Node
	KdNode node;

	//minimum object size met
    if(tris.size() <= 3 || depth > 24) { //is leaf node
        node.is_leaf = true;
        node.data[TRI_START_INDEX] = kdsorted_tri_vec.size();
        node.data[NUM_TRIS] = (int)tris.size();
        //add triangles to global vector
        for(int i = 0; i < (int)tris.size(); i++) {
            kdsorted_tri_vec.push_back(tris[i]); //add to KdNode's object array
        }
    } else { //recurse, is interior node (or may still be leaf if meets SAH criteria)
        float best_cost;
        node.split_axis = MaxAxis(bbox);
        node.split_pos = SAHSplitPlaneDistance(node.split_axis, bbox, tris, &best_cost);
        if(SAHGetLeafCost((int)tris.size()) < best_cost) {
            node.is_leaf = true;
            node.data[TRI_START_INDEX] = kdsorted_tri_vec.size();
            node.data[NUM_TRIS] = (int)tris.size();
            //add triangles to global vector
            for(int i = 0; i < (int)tris.size(); i++) {
                kdsorted_tri_vec.push_back(tris[i]); //add to KdNode's object array
            }
        } else {
            node.is_leaf = false;

            //construct left and right child nodes
            BBox left_bbox = bbox;
            left_bbox.max[node.split_axis] = node.split_pos;
            vector<int> left_tris;
            for(int i = 0; i < (int)tris.size(); i++) {
                if(Overlaps(left_bbox, tri_bboxes[tris[i]])) {
                    left_tris.push_back(tris[i]);
                }
            }
            BBox right_bbox = bbox;
            right_bbox.min[node.split_axis] = node.split_pos;
            vector<int> right_tris;
            for(int i = 0; i < (int)tris.size(); i++) {
                if(Overlaps(right_bbox, tri_bboxes[tris[i]])) {
                    right_tris.push_back(tris[i]);
                }
            }
            node.data[LEFT_CHILD] = RecursiveBuild(left_tris, left_bbox, depth + 1);
            node.data[RIGHT_CHILD]= RecursiveBuild(right_tris,right_bbox,depth + 1);
        }
    }
    nodes_vec.push_back(node);
	return kd_index++;
}

void BuildKdTree() {
    //set scene bbox and fill tri bbox array
    scene_bbox = IdentityBBox();
    tri_bboxes.reserve(total_scene_tris);
    for(int i = 0; i < total_scene_tris; i++) {
        BBox tri_bbox = GetBBox(triangles[i]);
        tri_bboxes.push_back(tri_bbox);
        scene_bbox = Union(scene_bbox, tri_bbox);
    }

    //setup global 1D triangle array
    vector<int> tri_indices;
    tri_indices.reserve(total_scene_tris);
    for(int i = 0; i < total_scene_tris; i++) {
        tri_indices.push_back(i);
    }

	RecursiveBuild(tri_indices, scene_bbox, 0);

    //copy nodes vector into nodes array
    num_kdnodes = nodes_vec.size();
    kdnodes = new KdNode[num_kdnodes];
    for(int i = 0; i < (int)nodes_vec.size(); i++) {
        kdnodes[i] = nodes_vec[i];
    }

    //copy vector into sorted triangle indices array
    num_kdsorted_tri_indices = kdsorted_tri_vec.size();
    kdsorted_tri_indices = new int[num_kdsorted_tri_indices];
    for(int i = 0; i < (int)kdsorted_tri_vec.size(); i++) {
        kdsorted_tri_indices[i] = kdsorted_tri_vec[i];
    }

    //put globals back to default - necessary for additional build calls
	kd_index = 0;
    kdsorted_tri_vec.erase(kdsorted_tri_vec.begin(), kdsorted_tri_vec.end());
    nodes_vec.erase(nodes_vec.begin(), nodes_vec.end());
    tri_bboxes.erase(tri_bboxes.begin(), tri_bboxes.end());
}
