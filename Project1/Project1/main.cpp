#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>

#include <cmath>

// Data Structure ----------------------------------------------------------------------------------------------------
struct Point {
	double x, y;
};

using Ring = std::vector<Point>;     // one polygon ring
using Polygon = std::vector<Ring>;   // full polygon (outer + holes)

// Functions ----------------------------------------------------------------------------------------------------
double computeArea(const Ring& ring) {
	double area = 0.0;
	int n = ring.size();

	for (int i = 0; i < n; i++) {
		const auto& p1 = ring[i];
		const auto& p2 = ring[(i + 1) % n];

		area += (p1.x * p2.y - p2.x * p1.y);
	}

	return area / 2.0; // signed area
}

// Start of the program ----------------------------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
	//std::cout << argc << std::endl;
	//for (int i{};i < argc; i++) {
	//	std::cout << argv[i] << std::endl;
	//}

	if (argc < 2) {
		std::cout << "Usage: program <csv_file>" << std::endl;
		return 1;
	}

	std::ifstream file(argv[1]);

	if (!file.is_open()) {
		std::cout << "Failed to open file" << std::endl;
		return 1;
	}

	std::string line;

	std::getline(file, line); // skip the header

	//std::map<int, Ring> temp; // group by ring_id
	std::map<int, std::map<int, Point>> temp;

	while (std::getline(file, line)) { // reads one row
		std::stringstream ss(line); // treat row as stream
		std::string token;

		int ring_id, vertex_id;
		double x, y;

		std::getline(ss, token, ',');
		ring_id = std::stoi(token);

		std::getline(ss, token, ',');
		vertex_id = std::stod(token);

		std::getline(ss, token, ',');
		x = std::stod(token);

		std::getline(ss, token, ',');
		y = std::stod(token);

		//temp[ring_id].push_back({x,y});
		temp[ring_id][vertex_id]={x, y};

		//std::cout << "Ring: " << ring_id << " Vertex: " << vertex_id << " (" << x << ", " << y << ")" << std::endl;
	}

	// Store the input polygon ----------------------------------------------------------------------------------------------------
	Polygon polygon;
	//for (auto& [id, ring] : temp) {
	//	polygon.push_back(ring);
	//}
	for (auto& [ring_id, ring_map] : temp) {
		Ring ring;

		for (auto& [vertex_id, point] : ring_map) {
			ring.push_back(point);
		}

		polygon.push_back(ring);
	}

	// Print the stored polygon ----------------------------------------------------------------------------------------------------
	int ring_id, vertex_id;
	double x, y;
	ring_id = 0;
	for (const auto& ring : polygon) {
		vertex_id = 0;
		for (const auto& point : ring) {
			std::cout << "Ring: " << ring_id << " Vertex: " << vertex_id << " (" << point.x << ", " << point.y << ")" << std::endl;
			vertex_id++;
		}
		ring_id++;
	}

	// Calculate the area for each ring ----------------------------------------------------------------------------------------------------
	for (int i = 0; i < polygon.size(); i++) {
		double area = computeArea(polygon[i]);

		std::cout << "Ring " << i
			<< " Area: " << area << "\n";

		if (area > 0)
			std::cout << "CCW (outer ring)\n";
		else
			std::cout << "CW (hole)\n";
	}

	// (Optional) PPM visualizer ----------------------------------------------------------------------------------------------------
	const int WIDTH = 500;
	const int HEIGHT = 500;

	// Find bounding box
	double minX = 1e9, maxX = -1e9;
	double minY = 1e9, maxY = -1e9;

	for (const auto& ring : polygon) {
		for (const auto& p : ring) {
			minX = std::min(minX, p.x);
			maxX = std::max(maxX, p.x);
			minY = std::min(minY, p.y);
			maxY = std::max(maxY, p.y);
		}
	}

	auto toPixel = [&](double x, double y) {
		int px = (int)((x - minX) / (maxX - minX) * (WIDTH - 1));
		int py = (int)((y - minY) / (maxY - minY) * (HEIGHT - 1));
		py = HEIGHT - 1 - py; // flip Y
		return std::pair<int, int>(px, py);
		};

	std::vector<std::vector<int>> img(HEIGHT, std::vector<int>(WIDTH, 0));

	auto drawLine = [&](int x0, int y0, int x1, int y1) {
		int dx = std::abs(x1 - x0), dy = -std::abs(y1 - y0);
		int sx = x0 < x1 ? 1 : -1;
		int sy = y0 < y1 ? 1 : -1;
		int err = dx + dy;

		while (true) {
			if (x0 >= 0 && x0 < WIDTH && y0 >= 0 && y0 < HEIGHT)
				img[y0][x0] = 255;

			if (x0 == x1 && y0 == y1) break;
			int e2 = 2 * err;
			if (e2 >= dy) { err += dy; x0 += sx; }
			if (e2 <= dx) { err += dx; y0 += sy; }
		}
		};

	for (const auto& ring : polygon) {
		for (size_t i = 0; i < ring.size(); i++) {
			auto [x0, y0] = toPixel(ring[i].x, ring[i].y);
			auto [x1, y1] = toPixel(ring[(i + 1) % ring.size()].x,
				ring[(i + 1) % ring.size()].y);

			drawLine(x0, y0, x1, y1);
		}
	}

	std::ofstream out("output.ppm");
	out << "P3\n" << WIDTH << " " << HEIGHT << "\n255\n";

	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			int v = img[y][x];
			out << v << " " << v << " " << v << " ";
		}
		out << "\n";
	}

	file.close();
	return 0;
}
