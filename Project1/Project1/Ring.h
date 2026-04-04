#pragma once
#include "geometry.h"
#include <list>
#include <vector>
#include <cstdint>

// VertexNode: element stored in each ring's linked list.
struct VertexNode {
    Point pt;
    bool operator==(const Point& o) const { return pt == o; }
    bool operator!=(const Point& o) const { return pt != o; }
};

class Ring {
public:
    using Iter = std::list<VertexNode>::iterator;

    int  id;
    bool isExterior;
    std::list<VertexNode> verts;

    Ring(int id, const std::vector<Point>& pts)
        : id(id), isExterior(id == 0) {
        for (const auto& p : pts) verts.push_back({ p });
    }

    int size() const { return (int)verts.size(); }

    double signedArea() const {
        double a = 0.0;
        auto it = verts.begin();
        auto prev = std::prev(verts.end());
        for (; it != verts.end(); prev = it, ++it)
            a += (prev->pt.x * it->pt.y - it->pt.x * prev->pt.y);
        return a * 0.5;
    }

    std::vector<Point> toVector() const {
        std::vector<Point> v;
        v.reserve(verts.size());
        for (const auto& n : verts) v.push_back(n.pt);
        return v;
    }
};