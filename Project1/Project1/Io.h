#pragma once
#include "ring.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <algorithm>

// ---------------------------------------------------------------------------
// Parse the input CSV file and return one Ring per ring_id found.
// Expected columns: ring_id,vertex_id,x,y
// ---------------------------------------------------------------------------
inline std::vector<Ring> readCSV(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Cannot open file: " + path);

    std::string line;
    std::getline(file, line); // skip header

    // Collect points grouped by ring_id (preserving vertex order)
    std::map<int, std::vector<std::pair<int, Point>>> raw; // ring_id -> [(vertex_id, pt)]

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        std::string tok;
        int ring_id, vertex_id;
        double x, y;
        std::getline(ss, tok, ','); ring_id = std::stoi(tok);
        std::getline(ss, tok, ','); vertex_id = std::stoi(tok);
        std::getline(ss, tok, ','); x = std::stod(tok);
        std::getline(ss, tok, ','); y = std::stod(tok);
        raw[ring_id].emplace_back(vertex_id, Point{ x, y });
    }

    // Build rings in ring_id order
    std::vector<Ring> rings;
    for (auto& [rid, vlist] : raw) {
        // Sort by vertex_id to ensure correct order
        std::sort(vlist.begin(), vlist.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
        std::vector<Point> pts;
        pts.reserve(vlist.size());
        for (auto& [vid, pt] : vlist) pts.push_back(pt);
        rings.emplace_back(rid, pts);
    }
    return rings;
}

// ---------------------------------------------------------------------------
// Print the simplified rings to stdout in the required CSV format.
// Also prints the three summary lines at the end.
// ---------------------------------------------------------------------------
inline void writeOutput(const std::vector<Ring>& rings,
    double inputArea,
    double arealDisplacement) {
    std::cout << "ring_id,vertex_id,x,y\n";
    std::cout << std::fixed;

    double outputArea = 0.0;
    for (const auto& ring : rings) {
        outputArea += ring.signedArea();
        int vid = 0;
        for (const auto& p : ring.verts) {
            // Print coordinates: strip trailing zeros but keep precision
            auto fmt = [](double v) -> std::string {
                // Use %g-style: drop trailing zeros
                char buf[64];
                // Try integer first
                if (v == (long long)v && std::abs(v) < 1e15)
                    snprintf(buf, sizeof(buf), "%lld", (long long)v);
                else
                    snprintf(buf, sizeof(buf), "%g", v);
                return buf;
                };
            std::cout << ring.id << ',' << vid++ << ','
                << fmt(p.pt.x) << ',' << fmt(p.pt.y) << '\n';
        }
    }

    // Summary lines in scientific notation
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "Total signed area in input: " << inputArea << '\n';
    std::cout << "Total signed area in output: " << outputArea << '\n';
    std::cout << "Total areal displacement: " << arealDisplacement << '\n';
}