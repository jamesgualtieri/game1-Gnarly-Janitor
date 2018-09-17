#include "WalkMesh.hpp"

static glm::vec3 triangle_to_world(
    std::vector<glm::vec3> triangle, const glm::vec3 &pos) {
    // I'll be honest this is almost the exact same as the following source:
    // https://www.gamedev.net/forums/topic/552906-closest-point-on-triangle/
    // but it works!

	glm::vec3 edge0 = triangle[1] - triangle[0];
	glm::vec3 edge1 = triangle[2] - triangle[0];
	glm::vec3 v0 = triangle[0] - pos;

	float a = glm::dot(edge0, edge0);
	float b = glm::dot(edge0, edge1);
	float c = glm::dot(edge1, edge1);
	float d = glm::dot(edge0, v0);
	float e = glm::dot(edge1, v0);

	float det = a * c - b * b;
	float s = b * e - c * d;
	float t = b * d - a * e;

	if (s + t < det) {
		if (s < 0.0f) {
			if (t < 0.0f) {
		        if (d < 0.0f) {
					s = glm::clamp(-d / a, 0.0f, 1.0f);
					t = 0.0f;
				} else {
					s = 0.0f;
					t = glm::clamp(-e / c, 0.0f, 1.0f);
				}
			} else {
				s = 0.0f;
				t = glm::clamp(-e / c, 0.0f, 1.0f);
			}
		} else if (t < 0.f) {
			s = glm::clamp(-d / a, 0.0f, 1.0f);
			t = 0.0f;
		} else {
			float invDet = 1.0f / det;
			s *= invDet;
			t *= invDet;
		}
	} else {
	if (s < 0.0f) {
		float tmp0 = b + d;
		float tmp1 = c + e;
		if (tmp1 > tmp0) {
			float numer = tmp1 - tmp0;
			float denom = a - 2.0f * b + c;
			s = glm::clamp(numer / denom, 0.0f, 1.0f);
			t = 1.0f - s;
		} else {
			t = glm::clamp(-e / c, 0.0f, 1.0f);
			s = 0.0f;
		}
	} else if (t < 0.f) {
		if (a + d > b + e) {
			float numer = c + e - b - d;
			float denom = a - 2.0f * b + c;
			s = glm::clamp(numer / denom, 0.0f, 1.0f);
			t = 1 - s;
		} else {
			s = glm::clamp(-e / c, 0.0f, 1.0f);
			t = 0.0f;
		}
	} else {
		float numer = c + e - b - d;
		float denom = a - 2.0f * b + c;
		s = glm::clamp(numer / denom, 0.0f, 1.0f);
		t = 1.0f - s;
		}
	}

	return triangle[0] + s * edge0 + t * edge1;
}

//Convert to barycentric coodrinates from point/vertices
// Heavily sorced from https://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
// (which referenced http://realtimecollisiondetection.net/)
glm::vec3 barycentric(glm::vec3 p0, glm::vec3 a, glm::vec3 b, glm::vec3 c) {
  float u, v, w;

  glm::vec3 v0 = b - a, v1 = c - a, v2 = p0 - a;
  float d00 = glm::dot(v0, v0);
  float d01 = glm::dot(v0, v1);
  float d11 = glm::dot(v1, v1);
  float d20 = glm::dot(v2, v0);
  float d21 = glm::dot(v2, v1);
  float denom = d00 * d11 - d01 * d01;
  v = (d11 * d20 - d01 * d21) / denom;
  w = (d00 * d21 - d01 * d20) / denom;
  u = 1.0f - v - w;

  return glm::vec3(u, v, w);
}

WalkMesh::WalkMesh(std::string file){
	std::ifstream filename(file, std::ios::binary);

	read_chunk(filename, "tri0", &triangles);
	read_chunk(filename, "vrt0", &vertices);
	read_chunk(filename, "nrm0", &vertex_normals);

	for (auto const &t : triangles) {
        //TODO: construct next_vertex map
		next_vertex[glm::uvec2(t.x, t.y)] = t.z;
		next_vertex[glm::uvec2(t.y, t.z)] = t.x;
		next_vertex[glm::uvec2(t.z, t.x)] = t.y;
	}
}

WalkMesh::WalkPoint WalkMesh::start(glm::vec3 const &world_point) const {
	WalkPoint closest;
	float min = FLT_MAX;
    //TODO: iterate through triangles
	for (const glm::uvec3 &t : triangles) {
        std::vector<glm::vec3> v = {vertices[t[0]], vertices[t[1]], vertices[t[2]]};
		//TODO: for each triangle, find closest point on triangle to world_point
        glm::vec3 point = triangle_to_world(v, world_point);
		if (glm::distance(point, world_point) < min) {
            //TODO: if point is closest, closest.triangle gets the current triangle, closest.weights gets the barycentric coordinates
			closest.triangle = t;
			closest.weights = barycentric(point, vertices[t[0]], vertices[t[1]], vertices[t[2]]);
            min = glm::distance(point, world_point);
		}
	}

	return closest;
}

void WalkMesh::walk(WalkPoint &wp, glm::vec3 const &step, size_t d) const {
	
    //TODO: project step to barycentric coordinates to get weights_step
    glm::vec3 world_plus_step = world_point(wp) + step;
    glm::vec3 bary_weights = barycentric(world_plus_step, vertices[wp.triangle.x],
        vertices[wp.triangle.y], vertices[wp.triangle.z]);
    
    glm::vec3 weights_step = bary_weights - wp.weights;

    if (d > 10) {
        return;
    }

    if (std::min(bary_weights.x, std::min(bary_weights.y, bary_weights.z)) > 0 ) {
        //if none of the the barycentric coordinates are negative, we are still in the same triangle.
        wp.weights = bary_weights;
    } else { 
        float v0 = 0.0f;
        float v1 = 0.0f; //if we cross over an edge, grab the edge vertices.
        if (bary_weights.x < 0) {
            // if x < 0, cross over yz edge
            wp.weights += weights_step * (wp.weights.x / -weights_step.x);
            v0 = wp.triangle.y;
            v1 = wp.triangle.z;
        } else if (bary_weights.y < 0) {
            // if y < 0, cross over zx edge
            wp.weights += weights_step * (wp.weights.y / -weights_step.y);
            v0 = wp.triangle.z;
            v1 = wp.triangle.x;
        } else if (bary_weights.z < 0) {
            //etc.
            wp.weights += weights_step * (wp.weights.z / -weights_step.z);
            v0 = wp.triangle.x;
            v1 = wp.triangle.y;
        } 

        glm::vec3 world_point_edge = world_point(wp);
        glm::vec3 reduced_step = world_plus_step - world_point_edge;

        if (next_vertex.find(glm::uvec2(v1, v0)) != next_vertex.end()) {
            glm::vec3 third_vertex = next_vertex.at(glm::uvec2(v1, v0));
            // auto third_vertex = next_vertex.at(glm::uvec2(v1, v0));
            // glm::vec3 third_vertex = next_vertex.at(glm::uvec2(v1, v0));
            auto result = std::find_if( triangles.begin(), triangles.end(), 
                [&](glm::uvec3 triangle) {
                return (v0 == triangle.x ||
                        v0 == triangle.y ||
                        v0 == triangle.z) &&
                       (v1 == triangle.x ||
                        v1 == triangle.y ||
                        v1 == triangle.z) &&
                       (third_vertex == triangle.x || third_vertex == triangle.y ||
                        third_vertex == triangle.z);
            });

            // bool policy = (v0 == t.x || v0 == t.y || v0 == t.z) &&
            //             (v1 == t.x || v1 == t.y || v1 == t.z) &&
            //             (third_vertex == t.x || third_vertex == t.y || third_vertex == t.z)
            // for (const glm::uvec3 &t : triangles) {
            //     if ((v0 == t.x ||
            //         v0 == t.y ||
            //         v0 == t.z) &&
            //        (v1 == t.x ||
            //         v1 == t.y ||
            //         v1 == t.z) &&
            //        (third_vertex == t.x || third_vertex == t.y ||
            //         third_vertex == t.z)) {
            //         result = &t;
            //     }
            // }


            if (result != triangles.end()) {
                wp.triangle = *result;
                wp.weights = barycentric(world_point_edge, vertices[wp.triangle.x],
                    vertices[wp.triangle.y], vertices[wp.triangle.z]);
                walk(wp, reduced_step, d + 1);
            }
        } 
    //it mostly works but sometimes dies, idk
    }
}
