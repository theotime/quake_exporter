// @theotime
// MIT licence

// ./quake_bsp.exe maps/start.bsp
// PAK0.PAK must be in the folder.

// resources used:
// http://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_4.htm#CBSP0


#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

#define PACK 0x4b434150

typedef struct pak_header_s {
	uint32_t id;
	uint32_t offset;
	uint32_t size;
} pak_header_t;

typedef struct pak_file_s {
	char name[56];
	uint32_t offset;
	uint32_t size;
} pak_file_t;

typedef struct dentry_s {
	uint32_t offset;
	uint32_t size;
} dentry_t;

typedef struct dheader_s {
	uint32_t version;
	dentry_t entities;
	dentry_t planes;
	dentry_t miptex;
	dentry_t vertices;
	dentry_t visilist;
	dentry_t nodes;
	dentry_t texinfo;
	dentry_t faces;
	dentry_t lightmaps;
	dentry_t clipnodes;
	dentry_t leaves;
	dentry_t lface;
	dentry_t edges;
	dentry_t ledges;
	dentry_t models;
} dheader_t;

typedef struct vec3_s {
	float x;
	float y;
	float z;
} vec3_t;

typedef struct ivec3_s {
	uint32_t x;
	uint32_t y;
	uint32_t z;
} ivec3_t;

typedef struct vertex_s {
	float x;
	float y;
	float z;
} vertex_t;

typedef struct edge_s {
	uint16_t v0;
	uint16_t v1;
} edge_t;

typedef struct face_s {
	uint16_t plane_id;
	uint16_t side;
	uint32_t ledge_id;
	uint16_t ledge_num;
	uint16_t texinfo_id;
	uint8_t typelight;
	uint8_t baselight;
	uint8_t light[2];
	uint32_t lightmap;
} face_t;

int main(int argc, char *argv[]) {

	if (argc != 2) {
		std::cout << argv[0] << " maps/start.bsp" << std::endl;
		exit(1);
	}

	std::ifstream fs;
	fs.open("PAK0.PAK", std::ifstream::binary);
	if (!fs.is_open()) {
		std::cerr << "Error opening input file" << std::endl;
		exit(1);
	}

	pak_header_t header;
	pak_file_t file_header;
	fs.read((char *)&header, sizeof(pak_header_t));
	//std::cout << std::hex << id << std::endl;
	if (header.id != PACK) {
		std::cerr << "Not a .pak file" << std::endl;
	}

	int num_files = header.size / sizeof(pak_file_t);
	std::cout << "offset: " << header.offset << std::endl;
	std::cout << "size: " << header.size << std::endl;
	std::cout << num_files << " files." << std::endl;

	fs.seekg(header.offset, std::ios_base::beg);

	int file_present = 0;
	for (int i = 0; i < num_files; i++) {
		fs.read((char *)&file_header, sizeof(pak_file_t));

		if (!std::strcmp(file_header.name, argv[1])) {
			file_present = 1;
			std::cout << "File present." << std::endl;

			fs.seekg(file_header.offset, std::ios_base::beg);
			int file_beg = fs.tellg();

			// faces, vertices
			std::vector<ivec3_t> faces;
			std::vector<vec3_t> vertices;

			dheader_t header;
			fs.read((char *)&header, sizeof(dheader_t));
			std::cout << header.version << std::endl;
			int num_vertices = header.vertices.size / sizeof(vertex_t);
			int num_faces = header.faces.size / sizeof(face_t);
			int num_edges = header.edges.size / sizeof(edge_t);
			int num_ledges = header.ledges.size / sizeof(uint32_t);
			std::cout << "num_vertices: " << num_vertices << std::endl;
			std::cout << "num_faces: " << num_faces << std::endl;
			std::cout << "num_edges: " << num_edges << std::endl;
			std::cout << "num_ledges: " << num_ledges << std::endl;

			// vertices
			fs.seekg(file_beg + header.vertices.offset, std::ios_base::beg);
			vec3_t v;
			for (unsigned int i = 0; i < num_vertices; i++) {
				fs.read((char *)&v, sizeof(vec3_t));
				v.x /= 100.;
				v.y /= 100.;
				v.z /= 100.;
				vertices.push_back(v);
			}

			// edges
			edge_t *edges = new edge_t[num_edges];
			fs.seekg(file_beg + header.edges.offset, std::ios_base::beg);
			fs.read((char *)edges, sizeof(edge_t) * num_edges);

			// ledges
			int *ledges = new int[num_ledges];
			fs.seekg(file_beg + header.ledges.offset, std::ios_base::beg);
			fs.read((char *)ledges, sizeof(int) * num_ledges);

			// faces
			fs.seekg(file_beg + header.faces.offset, std::ios_base::beg);
			face_t f;
			ivec3_t face;
			int num_tris = 0;
			for (unsigned int i = 0; i < num_faces; i++) {
				fs.read((char *)&f, sizeof(face_t));

				//std::cout << f.ledge_num << std::endl;

				if (f.ledge_num < 3) {
					std::cout << "polygon < 3 edges found." << std::endl;
				}
				else {
					// convert polygons to triangles
					// http://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-polygon-mesh
					uint16_t vertex0;
					if (ledges[f.ledge_id] > 0) {
						vertex0 = edges[abs(ledges[f.ledge_id])].v0;
					}
					else { // inverse v0, v1
						vertex0 = edges[abs(ledges[f.ledge_id])].v1;
					}

					uint16_t vertex1, vertex2;
					for (int k = 1; k < f.ledge_num - 1; k++) { // ignore the last edge
						if (ledges[f.ledge_id + k] > 0) {
							vertex1 = edges[abs(ledges[f.ledge_id + k])].v0;
							vertex2 = edges[abs(ledges[f.ledge_id + k])].v1;
						}
						else { // inverse v0, v1
							vertex1 = edges[abs(ledges[f.ledge_id + k])].v1;
							vertex2 = edges[abs(ledges[f.ledge_id + k])].v0;
						}

						num_tris++;
						face.x = vertex0;
						face.y = vertex1;
						face.z = vertex2;
						faces.push_back(face);
					}

				}
			}

			std::cout << "num tris: " << num_tris << std::endl;

			delete ledges;
			delete edges;

			uint32_t data;
			fs.read((char *)&data, sizeof(uint32_t));
			std::cout << "next data: " << data << std::endl;

			std::ofstream ofs;
			std::string filename;
			filename.append(argv[1]);
			filename.append(".obj");
			std::replace(filename.begin(), filename.end(), '/', '_');
			ofs.open(filename);

			if (!ofs.is_open()) {
				std::cerr << "Error opening output file" << std::endl;
				exit(1);
			}

			ofs << "o bsp" << std::endl;
			for (std::vector<vec3_t>::iterator it = vertices.begin(); it != vertices.end(); it++) {
				ofs << "v " << it->x << " " << it->y << " " << it->z << " " << std::endl;
			}

			for (std::vector<ivec3_t>::iterator it = faces.begin(); it != faces.end(); it++) {
				ofs << "f " << it->x + 1 << " " << it->y + 1 << " " << it->z + 1 << " " << std::endl;
			}

			ofs.close();
		}
	}

	if (!file_present) {
		std::cout << "There is no such file entry in pak." << std::endl;
	}

	std::cout << "Done." << std::endl;
	return 0;
}

