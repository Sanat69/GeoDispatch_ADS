#include <stdlib.h>
#include <math.h>
#include "kd.h"

// ─────────────────────────────────────────────────────────────
// STEP 1: COMPARE FUNCTIONS
//
// qsort() needs help to know "which point comes first?"
// We give it two functions — one for x, one for y.
//
// Rule: return negative if A comes first,
//       return positive if B comes first,
//       return 0 if they are equal.
// ─────────────────────────────────────────────────────────────

int compare_x(const void *a, const void *b) {
    point_t *pa = (point_t *)a;   // cast void* back to point_t*
    point_t *pb = (point_t *)b;

    if (pa->x < pb->x) return -1; // A has smaller x → A goes first
    if (pa->x > pb->x) return  1; // B has smaller x → B goes first
    return 0;                      // equal x → doesn't matter
}

int compare_y(const void *a, const void *b) {
    point_t *pa = (point_t *)a;
    point_t *pb = (point_t *)b;

    if (pa->y < pb->y) return -1;
    if (pa->y > pb->y) return  1;
    return 0;
}

// ─────────────────────────────────────────────────────────────
// STEP 2: CREATE A SINGLE NODE
//
// This is just a small helper to avoid repeating malloc code.
// It makes one tree node, fills it in, and returns it.
// ─────────────────────────────────────────────────────────────

static kdnode_t *make_node(point_t p, int axis) {
    kdnode_t *node = (kdnode_t *)malloc(sizeof(kdnode_t));

    node->point   = p;      // store the facility point
    node->axis    = axis;   // remember which axis we split on here
    node->deleted = 0;      // facility starts online (Nikhil flips this later)
    node->left    = NULL;   // no children yet
    node->right   = NULL;

    return node;
}

// ─────────────────────────────────────────────────────────────
// STEP 3: THE RECURSIVE BUILD
//
// Think of it like a phone book sorted two ways at once.
//
// At each step:
//   1. Look at the points you have right now (lo to hi)
//   2. Sort them by x (even depth) or y (odd depth)
//   3. Pick the MIDDLE point — it becomes this node
//   4. Left half  → becomes the left subtree  (repeat)
//   5. Right half → becomes the right subtree (repeat)
//   6. Stop when there are no points left
//
// "depth" just tracks how deep we are so we know which axis to use.
// ─────────────────────────────────────────────────────────────

static kdnode_t *build_recursive(point_t *pts, int lo, int hi, int depth) {

    // BASE CASE: no points in this section → return nothing
    if (lo > hi) return NULL;

    // Which axis do we sort on at this depth?
    // Even depth (0, 2, 4...) → sort by x
    // Odd  depth (1, 3, 5...) → sort by y
    int axis  = depth % 2;

    // How many points are in this section?
    int count = hi - lo + 1;

    // Sort just this section of the array
    if (axis == 0) {
        qsort(pts + lo, count, sizeof(point_t), compare_x);
    } else {
        qsort(pts + lo, count, sizeof(point_t), compare_y);
    }

    // The middle index becomes THIS node (median keeps tree balanced)
    int mid = (lo + hi) / 2;

    // Build this node
    kdnode_t *node = make_node(pts[mid], axis);

    // Recurse on the left half  (points before the median)
    node->left  = build_recursive(pts, lo,      mid - 1, depth + 1);

    // Recurse on the right half (points after the median)
    node->right = build_recursive(pts, mid + 1, hi,      depth + 1);

    return node;
}

// ─────────────────────────────────────────────────────────────
// STEP 4: kd_build  (the function your teammates actually call)
//
// pts = the original array of facilities
// n   = number of facilities
//
// WHY COPY? Because qsort shuffles the array around.
// We don't want to mess up the original order that Nikhil loaded.
// ─────────────────────────────────────────────────────────────

kdnode_t *kd_build(point_t *pts, int n) {

    // Safety check — nothing to build
    if (pts == NULL || n <= 0) return NULL;

    // Make a copy of the array so the original stays untouched
    point_t *copy = (point_t *)malloc(sizeof(point_t) * n);
    for (int i = 0; i < n; i++) {
        copy[i] = pts[i];
    }

    // Build the tree from depth 0
    kdnode_t *root = build_recursive(copy, 0, n - 1, 0);

    // Copy is no longer needed — the tree has all the data now
    free(copy);

    return root;
}

// ─────────────────────────────────────────────────────────────
// STEP 5: kd_free  (always clean up memory when done!)
//
// We visit children BEFORE the parent (post-order traversal).
// This way we never free a node that still has kids pointing to it.
// ─────────────────────────────────────────────────────────────

void kd_free(kdnode_t *root)
{
    if (root == NULL) return;   // nothing to free

    kd_free(root->left);        // free left subtree first
    kd_free(root->right);       // free right subtree
    free(root);                 // now safe to free this node
}

#include <float.h>  // for DBL_MAX — add this at top of kd.c

// ─────────────────────────────────────────────────────────────
// HELPER: distance squared between two points
//
// We use SQUARED distance everywhere during search.
// Avoids calling sqrt() on every comparison — sqrt is slow.
// We only sqrt at the very end if we need actual distance.
// ─────────────────────────────────────────────────────────────

static double dist_squared(point_t a, point_t b) {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return dx*dx + dy*dy;
}

// ─────────────────────────────────────────────────────────────
// THE RECURSIVE SEARCH
//
// At each node we:
//   1. Check if this node is closer than our best so far
//   2. Decide which child to visit FIRST (the one on the
//      same side as the query point — more likely to be close)
//   3. After visiting the near side, CHECK if the far side
//      could possibly have something closer
//      → if the perpendicular distance to the split line
//        is already worse than our best, SKIP the far side
//   4. That "skip" is what makes this fast
//
// best_point and best_dist are pointers so we can update
// them as we find better answers deeper in the tree
// ─────────────────────────────────────────────────────────────

static void nn_recursive(kdnode_t *node, point_t query,
                          point_t *best_point, double *best_dist) {

    // Nothing here — backtrack
    if (node == NULL) return;

    // Only consider this node if it hasn't been deleted (Nikhil's soft-delete)
    if (!node->deleted) {
        double d = dist_squared(node->point, query);
        if (d < *best_dist) {
            *best_dist  = d;
            *best_point = node->point;   // new best found
        }
    }

    // ── DECIDE WHICH SIDE TO VISIT FIRST ──────────────────
    //
    // The split axis tells us: at this node, everything in
    // the left subtree has a smaller x (or y), right has larger.
    //
    // We check which side the query point falls on —
    // that side is more likely to contain the nearest neighbor.
    //
    // diff = query coordinate - this node's coordinate on split axis
    // negative diff → query is on the LEFT side
    // positive diff → query is on the RIGHT side
    // ──────────────────────────────────────────────────────

    double diff;
    if (node->axis == 0) {
        diff = query.x - node->point.x;  // splitting on x
    } else {
        diff = query.y - node->point.y;  // splitting on y
    }

    // Visit the NEAR side first (same side as query point)
    kdnode_t *near_side = (diff < 0) ? node->left  : node->right;
    kdnode_t *far_side  = (diff < 0) ? node->right : node->left;

    nn_recursive(near_side, query, best_point, best_dist);

    // ── THE KEY PRUNING STEP ───────────────────────────────
    //
    // After visiting the near side, ask:
    // "Could the far side possibly have something closer?"
    //
    // The closest ANY point in the far subtree could be
    // is the perpendicular distance to the splitting line.
    // That distance is just |diff| (on the split axis).
    //
    // If diff*diff >= best_dist → the entire far subtree
    // is AT LEAST as far as our current best → SKIP IT.
    //
    // If diff*diff < best_dist → the splitting hyperplane
    // cuts through our best-distance circle → must check far side.
    // ──────────────────────────────────────────────────────

    if (diff * diff < *best_dist) {
        nn_recursive(far_side, query, best_point, best_dist);
    }
    // else: entire far subtree pruned — this is the speedup
}

// ─────────────────────────────────────────────────────────────
// kd_nearest — the public function
//
// query = the incident location (lat/lon converted to x/y by Sanat's bridge)
// returns = the closest non-deleted facility
//
// If the tree is empty or all nodes are deleted, returns a
// point with id = -1 as a sentinel (api.py checks for this)
// ─────────────────────────────────────────────────────────────

point_t kd_nearest(kdnode_t *root, point_t query) {

    // Start with "no best found yet"
    point_t best_point = {0.0, 0.0, -1};  // id=-1 means not found
    double  best_dist  = DBL_MAX;          // infinity

    nn_recursive(root, query, &best_point, &best_dist);

    return best_point;
}

// ─────────────────────────────────────────────────────────────
// MAX-HEAP of size k
//
// Why max-heap and not min-heap?
// Because we want fast access to the WORST (farthest) of our
// current k candidates — that's what we compare against.
// If a new point beats the worst, it earns a spot.
// ─────────────────────────────────────────────────────────────

typedef struct {
    double  dist;   // distance squared from query
    point_t pt;     // the facility
} heap_entry_t;

// ── sift down: fix heap after replacing the root ──
static void sift_down(heap_entry_t *heap, int size, int i) {
    while (1) {
        int largest = i;
        int left    = 2 * i + 1;
        int right   = 2 * i + 2;

        if (left  < size && heap[left].dist  > heap[largest].dist) largest = left;
        if (right < size && heap[right].dist > heap[largest].dist) largest = right;

        if (largest == i) break;  // heap property restored

        // swap
        heap_entry_t tmp = heap[i];
        heap[i]          = heap[largest];
        heap[largest]    = tmp;
        i                = largest;
    }
}

// ── sift up: fix heap after inserting at the end ──
static void sift_up(heap_entry_t *heap, int i) {
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (heap[parent].dist >= heap[i].dist) break;  // already correct

        // swap
        heap_entry_t tmp  = heap[parent];
        heap[parent]      = heap[i];
        heap[i]           = tmp;
        i                 = parent;
    }
}

// ── try to add a candidate into the heap ──────────
// If heap not full yet → just insert
// If heap is full and new point is closer than worst → replace worst
static void heap_try_insert(heap_entry_t *heap, int *heap_size,
                             int k, double dist, point_t pt) {
    if (*heap_size < k) {
        // heap not full yet — just add it
        heap[*heap_size].dist = dist;
        heap[*heap_size].pt   = pt;
        (*heap_size)++;
        sift_up(heap, *heap_size - 1);

    } else if (dist < heap[0].dist) {
        // new point is closer than the current worst → replace root
        heap[0].dist = dist;
        heap[0].pt   = pt;
        sift_down(heap, *heap_size, 0);
    }
    // else: new point is farther than all k current best → ignore
}

// ─────────────────────────────────────────────────────────────
// THE RECURSIVE k-NN SEARCH
//
// Same structure as nn_recursive from Task 2.
// Only difference: instead of one best_point we have a heap.
//
// Pruning condition changes slightly:
// If heap is full, we only explore the far side if
// diff*diff < heap[0].dist (the current kth-nearest distance).
// If heap isn't full yet we MUST explore both sides
// because we still need more candidates.
// ─────────────────────────────────────────────────────────────

static void knn_recursive(kdnode_t *node, point_t query,
                           heap_entry_t *heap, int *heap_size, int k) {

    if (node == NULL) return;

    // Check this node
    if (!node->deleted) {
        double d = dist_squared(node->point, query);
        heap_try_insert(heap, heap_size, k, d, node->point);
    }

    // Which side is the query on?
    double diff;
    if (node->axis == 0) {
        diff = query.x - node->point.x;
    } else {
        diff = query.y - node->point.y;
    }

    kdnode_t *near_side = (diff < 0) ? node->left  : node->right;
    kdnode_t *far_side  = (diff < 0) ? node->right : node->left;

    // Always visit near side first
    knn_recursive(near_side, query, heap, heap_size, k);

    // Only visit far side if it could contain a closer point
    // If heap isn't full yet → must visit (we need more candidates)
    // If heap is full → only visit if splitting plane is within current worst distance
    if (*heap_size < k || diff * diff < heap[0].dist) {
        knn_recursive(far_side, query, heap, heap_size, k);
    }
}

// ─────────────────────────────────────────────────────────────
// kd_knn — the public function
//
// query     = the incident location
// k         = how many nearest facilities you want
// out_count = how many results are actually returned
//             (could be less than k if fewer facilities exist)
//
// Returns a malloc'd array — CALLER must free() it
// Results are sorted nearest-first
// ─────────────────────────────────────────────────────────────

point_t *kd_knn(kdnode_t *root, point_t query, int k, int *out_count) {

    // Safety
    if (root == NULL || k <= 0) {
        *out_count = 0;
        return NULL;
    }

    // Allocate the heap
    heap_entry_t *heap = (heap_entry_t *)malloc(sizeof(heap_entry_t) * k);
    int heap_size = 0;

    // Run the search
    knn_recursive(root, query, heap, &heap_size, k);

    // heap_size is how many results we actually found
    *out_count = heap_size;

    // Copy results into a plain point_t array
    // The heap is a max-heap so index 0 is the FARTHEST of the k results.
    // Reverse it so index 0 is the NEAREST (more intuitive for the caller).
    point_t *results = (point_t *)malloc(sizeof(point_t) * heap_size);
    for (int i = 0; i < heap_size; i++) {
        results[i] = heap[heap_size - 1 - i].pt;
    }

    free(heap);
    return results;
}