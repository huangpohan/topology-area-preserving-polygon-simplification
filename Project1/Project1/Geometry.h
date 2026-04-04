#pragma once
#include <cmath>
#include <list>
#include <vector>

// ---------------------------------------------------------------------------
// Basic 2-D point
// ---------------------------------------------------------------------------
struct Point {
    double x, y;
    Point(double x = 0, double y = 0) : x(x), y(y) {}
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Point& o) const { return !(*this == o); }
};

// ---------------------------------------------------------------------------
// Signed area of a triangle (positive = CCW)
// ---------------------------------------------------------------------------
inline double triangleSignedArea(const Point& a, const Point& b, const Point& c) {
    return 0.5 * ((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y));
}

// ---------------------------------------------------------------------------
// Signed area of a ring using the shoelace formula.
// Positive for CCW (exterior), negative for CW (interior/hole).
// ---------------------------------------------------------------------------
inline double ringSignedArea(const std::vector<Point>& pts) {
    double area = 0.0;
    int n = (int)pts.size();
    for (int i = 0; i < n; ++i) {
        const Point& a = pts[i];
        const Point& b = pts[(i + 1) % n];
        area += (a.x * b.y - b.x * a.y);
    }
    return area * 0.5;
}

// ---------------------------------------------------------------------------
// Segment–segment intersection test (open segments, excludes shared endpoints)
// Returns true if segments (p1,p2) and (p3,p4) properly intersect.
// ---------------------------------------------------------------------------
inline double cross2d(double ax, double ay, double bx, double by) {
    return ax * by - ay * bx;
}

inline bool segmentsIntersect(const Point& p1, const Point& p2,
    const Point& p3, const Point& p4) {
    double d1x = p2.x - p1.x, d1y = p2.y - p1.y;
    double d2x = p4.x - p3.x, d2y = p4.y - p3.y;
    double denom = cross2d(d1x, d1y, d2x, d2y);
    if (std::abs(denom) < 1e-12) return false; // parallel

    double dx = p3.x - p1.x, dy = p3.y - p1.y;
    double t = cross2d(dx, dy, d2x, d2y) / denom;
    double u = cross2d(dx, dy, d1x, d1y) / denom;
    // Strictly interior to both segments (open: excludes endpoints)
    return t > 1e-10 && t < 1.0 - 1e-10 &&
        u > 1e-10 && u < 1.0 - 1e-10;
}