#include <stdlib.h>
#include <stdio.h>
#include "voronoi.h"

/* STUB implementation since P3 did not provide it. */

dcel_t *voronoi_build(point_t *sites, int n) {
    dcel_t *d = calloc(1, sizeof(dcel_t));
    if (!d || n <= 0) return d;
    
    d->nf = n;
    d->faces = calloc(n, sizeof(face_t *));
    
    for (int i = 0; i < n; i++) {
        face_t *f = calloc(1, sizeof(face_t));
        f->site_id = sites[i].id;
        /* create a dummy boundary so clipping doesn't crash */
        f->outer_edge = calloc(1, sizeof(half_edge_t));
        f->outer_edge->face = f;
        
        vertex_t *v = calloc(1, sizeof(vertex_t));
        v->x = sites[i].x;
        v->y = sites[i].y;
        f->outer_edge->origin = v;
        f->outer_edge->next = f->outer_edge;
        f->outer_edge->prev = f->outer_edge;
        
        d->faces[i] = f;
    }
    return d;
}

face_t **dcel_neighbours(dcel_t *d, int site_id, int *out_count) {
    (void)d; (void)site_id;
    *out_count = 0;
    return NULL;
}

void voronoi_free(dcel_t *d) {
    if (!d) return;
    for (int i = 0; i < d->nf; i++) {
        if (d->faces[i]) {
            if (d->faces[i]->outer_edge) {
                free(d->faces[i]->outer_edge->origin);
                free(d->faces[i]->outer_edge);
            }
            free(d->faces[i]);
        }
    }
    free(d->faces);
    free(d);
}

void voronoi_insert_site(dcel_t *d, point_t new_site) {
    (void)d; (void)new_site;
}
