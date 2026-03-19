#ifndef VORONOI_H
#define VORONOI_H

#include "kd.h"

// Task 2 — DCEL implementation — dcel.h structures
typedef struct half_edge half_edge_t;
typedef struct face face_t;

typedef struct vertex {
    double x, y;
    half_edge_t* incident_edge;
} vertex_t;

struct half_edge {
    vertex_t* origin;
    half_edge_t* twin;
    half_edge_t* next;
    half_edge_t* prev;
    face_t* face;
    int is_infinite;
};

struct face {
    int site_id;
    half_edge_t* outer_edge;
    double area;          // computed by P4
    int is_underserved;   // set by P4
};

typedef struct dcel {
    vertex_t** vertices;
    int nv;
    half_edge_t** edges;
    int ne;
    face_t** faces;
    int nf;
    
    // Internal capacities for dynamic array reallocation
    int max_v, max_e, max_f;
} dcel_t;

// P3 exports
dcel_t* voronoi_build(point_t* sites, int n);
face_t** dcel_neighbours(dcel_t* d, int site_id, int* out_count);
void voronoi_free(dcel_t* d);

// Incremental insertion
void voronoi_insert_site(dcel_t* d, point_t new_site);

// P4 exports (additions to voronoi.h)
void clip_to_bbox(dcel_t* d, double xmin, double ymin, double xmax, double ymax);
double cell_area(dcel_t* d, int face_id);
int* flag_underserved(dcel_t* d, double threshold, int* out_count);

#endif
