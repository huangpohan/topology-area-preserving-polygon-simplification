#pragma once
#include "geometry.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cmath>
#include <functional>
#include <cstdint>

// ---------------------------------------------------------------------------
// A segment stored in the index: two endpoints plus an opaque ID the caller
// uses to skip self-adjacent edges during intersection tests.
// ---------------------------------------------------------------------------
struct IndexedSegment {
    Point  p1, p2;
    int    ringIdx;
};

// ---------------------------------------------------------------------------
// UniformGrid spatial index.
//
// The world is divided into a grid of square cells of side `cellSize`.
// Each segment is inserted into every cell whose bounding box overlaps the
// segment's axis-aligned bounding box (AABB).
//
// Query: given a query segment, return all segments in overlapping cells.
// The caller must filter the results to only keep true intersections.
//
// Complexity:
//   Insert / Remove : O(k) where k = number of cells covered by the segment
//   Query           : O(k + r) where r = number of candidate segments returned
//   For polygon data with roughly uniform edge lengths, k is O(1) and
//   r is O(local density), giving near-O(1) queries in practice.
// ---------------------------------------------------------------------------
class SpatialIndex {
public:
    // Build an empty index with the given cell size.
    // A good default: set cellSize to roughly the average segment length,
    // or estimate it from the bounding box and vertex count.
    explicit SpatialIndex(double cellSize = 1.0) : cellSize_(cellSize) {}

    // Re-build from scratch using the given cell size.
    void reset(double cellSize) {
        cellSize_ = cellSize;
        grid_.clear();
    }

    // Insert a segment. Returns an integer handle (index into segments_) that
    // can be used to remove the segment later.
    int insert(const IndexedSegment& seg) {
        int handle = (int)segments_.size();
        segments_.push_back(seg);
        active_.push_back(true);
        for (auto [cx, cy] : cellsForSegment(seg.p1, seg.p2))
            grid_[cellKey(cx, cy)].push_back(handle);
        return handle;
    }

    // Mark a segment as removed (lazy deletion — doesn't shrink the grid).
    void remove(int handle) {
        if (handle >= 0 && handle < (int)active_.size())
            active_[handle] = false;
    }

    // Query: collect all active segments whose cells overlap the AABB of
    // the query segment (p1, p2).  Results are de-duplicated.
    // The caller is responsible for the actual intersection test.
    std::vector<const IndexedSegment*> query(const Point& p1, const Point& p2) const {
        std::unordered_set<int> seen;
        std::vector<const IndexedSegment*> result;

        for (auto [cx, cy] : cellsForSegment(p1, p2)) {
            auto it = grid_.find(cellKey(cx, cy));
            if (it == grid_.end()) continue;
            for (int h : it->second) {
                if (!active_[h]) continue;
                if (seen.insert(h).second)
                    result.push_back(&segments_[h]);
            }
        }
        return result;
    }

    // Convenience: query for a pair of new edges (A→E and E→D) together.
    std::vector<const IndexedSegment*> queryTwo(const Point& A, const Point& E,
        const Point& D) const {
        std::unordered_set<int> seen;
        std::vector<const IndexedSegment*> result;

        auto collect = [&](const Point& q1, const Point& q2) {
            for (auto [cx, cy] : cellsForSegment(q1, q2)) {
                auto it = grid_.find(cellKey(cx, cy));
                if (it == grid_.end()) continue;
                for (int h : it->second) {
                    if (!active_[h]) continue;
                    if (seen.insert(h).second)
                        result.push_back(&segments_[h]);
                }
            }
            };
        collect(A, E);
        collect(E, D);
        return result;
    }

    int segmentCount() const {
        int n = 0;
        for (bool a : active_) n += a ? 1 : 0;
        return n;
    }

private:
    double cellSize_;

    // Flat storage of all segments ever inserted (active_ tracks live ones).
    std::vector<IndexedSegment> segments_;
    std::vector<bool>           active_;

    // Grid: cell key -> list of segment handles in that cell.
    // Key encodes (cx, cy) as a 64-bit integer to avoid a pair hash.
    using Grid = std::unordered_map<int64_t, std::vector<int>>;
    Grid grid_;

    int64_t cellKey(int cx, int cy) const {
        // Pack two 32-bit signed ints into one 64-bit key.
        return (int64_t)(uint32_t)cx | ((int64_t)(uint32_t)cy << 32);
    }

    int worldToCell(double coord) const {
        return (int)std::floor(coord / cellSize_);
    }

    // Return all (cx,cy) grid cells that the AABB of segment (p1,p2) covers.
    std::vector<std::pair<int, int>> cellsForSegment(const Point& p1,
        const Point& p2) const {
        double minx = std::min(p1.x, p2.x);
        double miny = std::min(p1.y, p2.y);
        double maxx = std::max(p1.x, p2.x);
        double maxy = std::max(p1.y, p2.y);

        int x0 = worldToCell(minx);
        int y0 = worldToCell(miny);
        int x1 = worldToCell(maxx);
        int y1 = worldToCell(maxy);

        std::vector<std::pair<int, int>> cells;
        cells.reserve((x1 - x0 + 1) * (y1 - y0 + 1));
        for (int cx = x0; cx <= x1; ++cx)
            for (int cy = y0; cy <= y1; ++cy)
                cells.emplace_back(cx, cy);
        return cells;
    }
};

// ---------------------------------------------------------------------------
// Helper: choose a good cell size for a polygon.
// Rule of thumb: avg segment length.  We compute it from the bounding box
// diagonal and total vertex count as a cheap approximation.
// ---------------------------------------------------------------------------
inline double chooseCellSize(const std::vector<Ring>& rings) {
    double minx = 1e18, miny = 1e18;
    double maxx = -1e18, maxy = -1e18;
    int    totalVerts = 0;

    for (const auto& ring : rings) {
        for (const auto& p : ring.verts) {
            minx = std::min(minx, p.pt.x); miny = std::min(miny, p.pt.y);
            maxx = std::max(maxx, p.pt.x); maxy = std::max(maxy, p.pt.y);
        }
        totalVerts += ring.size();
    }

    if (totalVerts < 2) return 1.0;

    double diag = std::sqrt((maxx - minx) * (maxx - minx) + (maxy - miny) * (maxy - miny));
    // Use 2× average segment length as the cell size so that each segment
    // typically touches only 1–4 cells.
    double avgLen = diag / std::sqrt((double)totalVerts);
    return std::max(avgLen * 2.0, 1e-9);
}

// Remove the first active segment matching coordinates (p1,p2).
// Used for incremental updates after a collapse.
// O(k) cells checked, O(s) segments per cell scanned.
void removeByCoords(const Point& p1, const Point& p2) {
    for (auto [cx, cy] : cellsForSegment(p1, p2)) {
        auto it = grid_.find(cellKey(cx, cy));
        if (it == grid_.end()) continue;
        for (int h : it->second) {
            if (!active_[h]) continue;
            const auto& s = segments_[h];
            if (s.p1 == p1 && s.p2 == p2) {
                active_[h] = false;
                return;  // remove first match only
            }
        }
    }
}