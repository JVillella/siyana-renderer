#ifndef BBOX_H_INCLUDED
#define BBOX_H_INCLUDED

inline BBox IdentityBBox() {
    return (BBox){float3( HUGE_VALUE,  HUGE_VALUE,  HUGE_VALUE),
                  float3(-HUGE_VALUE, -HUGE_VALUE, -HUGE_VALUE)};
}

inline bool Inside(const BBox& main, const BBox& sec) {
    //checks bbox is inside the bounding box
    bool x = (sec.min.x >= main.min.x) && (sec.max.x <= main.max.x);
    bool y = (sec.min.y >= main.min.y) && (sec.max.y <= main.max.y);
    bool z = (sec.min.z >= main.min.z) && (sec.max.z <= main.max.z);
    return x && y && z;
}

inline bool Inside(const BBox& bbox, const float3& p) {
    //checks point is inside the bounding box
    bool x = (p.x >= bbox.min.x) && (p.x <= bbox.max.x);
    bool y = (p.y >= bbox.min.y) && (p.y <= bbox.max.y);
    bool z = (p.z >= bbox.min.z) && (p.z <= bbox.max.z);
    return x && y && z;
}

inline bool Overlaps(const BBox& main, const BBox& sec) {
    bool x = (sec.min.x <= main.max.x) && (sec.max.x >= main.min.x);
    bool y = (sec.min.y <= main.max.y) && (sec.max.y >= main.min.y);
    bool z = (sec.min.z <= main.max.z) && (sec.max.z >= main.min.z);
    return x && y && z;
}

inline void Expand(BBox* bbox, float delta) {
    bbox->min -= delta;
    bbox->max += delta;
}

inline int MaxAxis(const BBox& bbox) {
    float3 diag = bbox.max - bbox.min;

    if (diag.x > diag.y && diag.x > diag.z) { // x-axis
        return 0;
    } else if (diag.y > diag.z) { // y-axis
        return 1;
    } else { // z-axis
        return 2;
    }
}

inline int MinAxis(const BBox& bbox) {
    float3 diag = bbox.max - bbox.min;

    if(diag.x < diag.y && diag.x < diag.z) { // x-axis
        return 0;
    } else if(diag.y < diag.z) { // y-axis
        return 1;
    } else { // z-axis
        return 2;
    }
}

inline float SurfaceArea(const BBox& bbox) {
    float3 d = bbox.max - bbox.min;
    // Formula: 2 * (w * h + w * d + h * d)
    return 2.f * ((d.x*d.y) + (d.x*d.z) + (d.y*d.z));
}

inline float Volume(const BBox& bbox) {
    float3 d(bbox.max - bbox.min);
    // Formula: w * h * d
    return (d.x * d.y * d.z);
}

inline BBox Union(const BBox& a, const BBox& b) {
    BBox bbox;
    //Calculate min corner
    bbox.min.x = min(a.min.x, b.min.x);
    bbox.min.y = min(a.min.y, b.min.y);
    bbox.min.z = min(a.min.z, b.min.z);
    
    //Calculate max corner
    bbox.max.x = max(a.max.x, b.max.x);
    bbox.max.y = max(a.max.y, b.max.y);
    bbox.max.z = max(a.max.z, b.max.z);
    return bbox;
}

inline BBox Union(const BBox& b, const float3& p) {
    BBox bbox = IdentityBBox();
    // Calculate min corner
    bbox.min.x = min(b.min.x, p.x);
    bbox.min.y = min(b.min.y, p.y);
    bbox.min.z = min(b.min.z, p.z);
    
    // Calculate max corner
    bbox.max.x = max(b.max.x, p.x);
    bbox.max.y = max(b.max.y, p.y);
    bbox.max.z = max(b.max.z, p.z);
    return bbox;
}

#endif // BBOX_H_INCLUDED
