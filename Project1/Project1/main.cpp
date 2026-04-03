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

struct WorkItem {
	int ring_id;
	int a, b, c, d;
	Point e;
	double displacement;
};

// Function prototypes ----------------------------------------------------------------------------------------------------
double computeArea(const Ring& ring);
std::vector<WorkItem> buildWorklist(const Polygon& polygon);
void printWorklist(const std::vector<WorkItem>& worklist, const Polygon& polygon);

double cross(const Point& p, const Point& q);
Point computeE(const Point& A, const Point& B, const Point& C, const Point& D);

// Function definitions ----------------------------------------------------------------------------------------------------
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

std::vector<WorkItem> buildWorklist(const Polygon& polygon) {
	std::vector<WorkItem> worklist;

	for (int ring_id = 0; ring_id < static_cast<int>(polygon.size()); ring_id++) {
		const Ring& ring = polygon[ring_id];
		int m = static_cast<int>(ring.size());

		// Need at least 4 vertices to form A-B-C-D
		if (m < 4) {
			continue;
		}

		for (int i = 0; i < m; i++) {
			WorkItem item;
			item.ring_id = ring_id;
			item.a = i;
			item.b = (i + 1) % m;
			item.c = (i + 2) % m;
			item.d = (i + 3) % m;

			const Point& A = ring[item.a];
			const Point& B = ring[item.b];
			const Point& C = ring[item.c];
			const Point& D = ring[item.d];

			item.e = computeE(A, B, C, D);
			item.displacement = 0.0; // temporary for now

			worklist.push_back(item);
		}
	}

	return worklist;
}

void printWorklist(const std::vector<WorkItem>& worklist, const Polygon& polygon) {
	for (const auto& item : worklist) {
		const Ring& ring = polygon[item.ring_id];

		const Point& A = ring[item.a];
		const Point& B = ring[item.b];
		const Point& C = ring[item.c];
		const Point& D = ring[item.d];
		const Point& E = item.e;

		std::cout << "Ring " << item.ring_id
			<< " | indices: "
			<< item.a << "," << item.b << "," << item.c << "," << item.d
			<< " | points: "
			<< "A(" << A.x << "," << A.y << ") "
			<< "B(" << B.x << "," << B.y << ") "
			<< "C(" << C.x << "," << C.y << ") "
			<< "D(" << D.x << "," << D.y << ") "
			<< " | E(" << E.x << "," << E.y << ") "
			<< std::endl;
	}
}

double cross(const Point& p, const Point& q) {
	return p.x * q.y - p.y * q.x;
}

double cross(const Point& a, const Point& b, const Point& c) {
	// cross product of AB x AC
	return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

int sideOfPointToDirectedLine(const Point& p, const Point& a, const Point& d) {
	double s = cross(a, d, p);
	const double EPS = 1e-9;
	if (s > EPS) return 1;
	if (s < -EPS) return -1;
	return 0;
}

double distancePointToLineTwiceArea(const Point& p, const Point& a, const Point& d) {
	// proportional to perpendicular distance from p to line AD
	// no need to divide by |AD| since only comparing B vs C
	return std::abs(cross(a, d, p));
}

Point intersectLineWithSegment(const Point& p1, const Point& p2,
	double a, double b, double c) {
	double f1 = a * p1.x + b * p1.y + c;
	double f2 = a * p2.x + b * p2.y + c;

	const double EPS = 1e-9;

	// If one endpoint is already on the line
	if (std::abs(f1) < EPS) return p1;
	if (std::abs(f2) < EPS) return p2;

	double denom = f1 - f2;
	if (std::abs(denom) < EPS) {
		// segment is parallel to the line or numerically unstable
		return p1;
	}

	double t = f1 / (f1 - f2); // point = p1 + t(p2-p1)

	return {
		p1.x + t * (p2.x - p1.x),
		p1.y + t * (p2.y - p1.y)
	};
}

Point computeE(const Point& A, const Point& B, const Point& C, const Point& D) {
	const double EPS = 1e-9;

	// Singular case: B,C,D collinear -> optimal placement is at B
	if (std::abs(cross(B, C, D)) < EPS) {
		return B;
	}

	// Equation (1b): ax + by + c = 0
	double a = D.y - A.y;
	double b = A.x - D.x;
	double c = -B.y * A.x
		+ (A.y - C.y) * B.x
		+ (B.y - D.y) * C.x
		+ C.y * D.x;

	// Evaluate where B,C lie relative to AD
	int sideB = sideOfPointToDirectedLine(B, A, D);
	int sideC = sideOfPointToDirectedLine(C, A, D);

	// To know which side the E-line is on relative to AD,
	// sample any point on ax + by + c = 0.
	Point sampleEline;
	if (std::abs(b) > EPS) {
		sampleEline = { 0.0, -c / b };
	}
	else {
		sampleEline = { -c / a, 0.0 };
	}

	int sideEline = sideOfPointToDirectedLine(sampleEline, A, D);

	double distB = distancePointToLineTwiceArea(B, A, D);
	double distC = distancePointToLineTwiceArea(C, A, D);

	// Paper pseudocode:
	// if side(B,AD) == side(C,AD):
	//    if d(B,AD) > d(C,AD): return intersection(Eline, AB)
	//    else if d(B,AD) < d(C,AD): return intersection(Eline, CD)
	// else:
	//    if side(B,AD) == side(Eline,AD): return intersection(Eline, AB)
	//    else: return intersection(Eline, CD)

	Point E;

	if (sideB == sideC) {
		if (distB > distC + EPS) {
			E = intersectLineWithSegment(A, B, a, b, c);
		}
		else {
			E = intersectLineWithSegment(C, D, a, b, c);
		}
	}
	else {
		if (sideB == sideEline) {
			E = intersectLineWithSegment(A, B, a, b, c);
		}
		else {
			E = intersectLineWithSegment(C, D, a, b, c);
		}
	}

	// DEBUG CHECK
	double check = a * E.x + b * E.y + c;
	std::cout << "line check = " << check << '\n';

	return E;
}

// Start of the program ----------------------------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
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

	std::map<int, std::map<int, Point>> temp;

	while (std::getline(file, line)) { // reads one row
		std::stringstream ss(line); // treat row as stream
		std::string token;

		int ring_id, vertex_id;
		double x, y;

		std::getline(ss, token, ',');
		ring_id = std::stoi(token);

		std::getline(ss, token, ',');
		vertex_id = std::stoi(token);

		std::getline(ss, token, ',');
		x = std::stod(token);

		std::getline(ss, token, ',');
		y = std::stod(token);

		temp[ring_id][vertex_id]={x, y};
	}

	// Store the input polygon ----------------------------------------------------------------------------------------------------
	Polygon polygon;
	for (auto& [ring_id, ring_map] : temp) {
		Ring ring;

		for (auto& [vertex_id, point] : ring_map) {
			ring.push_back(point);
		}

		polygon.push_back(ring);
	}

	// Print the stored polygon ----------------------------------------------------------------------------------------------------
	std::cout << "Step 1: Stored Polygon" << std::endl;
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
	std::cout << std::endl;
	
	// Print the worklist (ABCD for now) ----------------------------------------------------------------------------------------------------
	std::cout << "Step 2: Worklist" << std::endl;
	std::vector<WorkItem> worklist = buildWorklist(polygon);
	printWorklist(worklist, polygon);
	std::cout << std::endl;


	// Calculate the area for each ring ----------------------------------------------------------------------------------------------------
	//for (int i = 0; i < polygon.size(); i++) {
	//	double area = computeArea(polygon[i]);

	//	std::cout << "Ring " << i
	//		<< " Area: " << area << "\n";

	//	if (area > 0)
	//		std::cout << "CCW (outer ring)\n";
	//	else
	//		std::cout << "CW (hole)\n";
	//}

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
