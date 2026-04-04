#pragma once
#include "ring.h"
#include <vector>
#include <queue>
#include <cmath>

struct Collapse {
    double     arealDisplacement;
    int        ringIdx;
    // All four iterators stored:
    //   iterB, iterC  — erased on apply; guarded by dead-set
    //   iterA, iterD  — never erased; guarded by coordinate comparison
    Ring::Iter iterA, iterB, iterC, iterD;
    const void* nodeAddrA, * nodeAddrB, * nodeAddrC, * nodeAddrD;
    Point A, B, C, D, E;     // coordinates at enqueue time
};

struct CollapseCompare {
    bool operator()(const Collapse& a, const Collapse& b) const {
        return a.arealDisplacement > b.arealDisplacement;
    }
};

using CollapseQueue = std::priority_queue<Collapse, std::vector<Collapse>, CollapseCompare>;

inline Point computeE(const Point& A, const Point& B,
    const Point& C, const Point& D) {
    double S = triangleSignedArea(A, B, D) + triangleSignedArea(B, C, D);
    double ux = D.x - A.x, uy = D.y - A.y;
    double u2 = ux * ux + uy * uy;
    if (u2 < 1e-20) return Point{ (B.x + C.x) * 0.5, (B.y + C.y) * 0.5 };
    double t = -2.0 * S / u2;
    return Point{ (A.x + D.x) * 0.5 + t * (-uy), (A.y + D.y) * 0.5 + t * ux };
}

inline double computeArealDisplacement(const Point& A, const Point& B,
    const Point& C, const Point& D,
    const Point& /*E*/) {
    return std::abs(triangleSignedArea(A, B, D) + triangleSignedArea(B, C, D));
}

double simplify(std::vector<Ring>& rings, int targetVertices);