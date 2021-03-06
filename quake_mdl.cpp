// @theotime
// MIT licence

// ./quake_mdl.exe progs/demon.mdl
// PAK0.PAK must be in the folder.

// resources used:
// https://quakewiki.org/wiki/.pak
// https://quakewiki.org/wiki/pak0.pak
// http://tfc.duke.free.fr/coding/mdl-specs-en.html


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

typedef struct vec3_s {
	float x;
	float y;
	float z;
} vec3_t;

typedef struct face_s {
	uint32_t a;
	uint32_t b;
	uint32_t c;
} face_t;

typedef struct mdl_header_s {
	uint32_t id;
	uint32_t version;

	vec3_t scale;
	vec3_t translate;
	float boundingradius;
	vec3_t eye_pos;

	uint32_t num_skins;
	uint32_t skinwidth;
	uint32_t skinheight;

	uint32_t num_verts;
	uint32_t num_tris;
	uint32_t num_frames;

	uint32_t sync_type;
	uint32_t flags;
	uint32_t size;
} mdl_header_t;

typedef struct mdl_triangle_s {
	uint32_t facefront;
	face_t face;
} mdl_triangle_t;

typedef struct mdl_vertex_s {
	unsigned char v[3];
	unsigned char normalIndex;
} mdl_vertex_t;

int main(int argc, char *argv[]) {

	if (argc != 2) {
		std::cout << argv[0] << " progs/demon.mdl" << std::endl;
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

			// faces, vertices
			std::vector<face_t> faces;
			std::vector<vec3_t> vertices;

			mdl_header_t mdl_header;
			fs.seekg(file_header.offset, std::ios_base::beg);
			fs.read((char *)&mdl_header, sizeof(mdl_header_t));
			std::cout << mdl_header.id << std::endl;

			std::cout << "num_skins: " << mdl_header.num_skins << std::endl;
			std::cout << "num_verts: " << mdl_header.num_verts << std::endl;
			std::cout << "num_tris: " << mdl_header.num_tris << std::endl;
			std::cout << "num_frames: " << mdl_header.num_frames << std::endl;
			uint32_t tex_group;
			fs.read((char *)&tex_group, sizeof(uint32_t));
			std::cout << "tex_group: " << tex_group << std::endl;

			// mdl_skin_t.data
			fs.seekg(mdl_header.skinheight * mdl_header.skinwidth, std::ios_base::cur);
			// mdl_texcoord_t
			fs.seekg(mdl_header.num_verts * sizeof(uint32_t) * 3, std::ios_base::cur);

			// mdl_triangle_t
			face_t face;
			uint32_t facefront;
			for (uint32_t i = 0; i < mdl_header.num_tris; i++) {
				fs.read((char *)&facefront, sizeof(uint32_t));
				fs.read((char *)&face, sizeof(face_t));
				faces.push_back(face);
			}

			// mdl_frame_t
			uint32_t frame_type;
			fs.read((char *)&frame_type, sizeof(uint32_t));
			std::cout << "frame_type: " << frame_type << std::endl;

			// mdl_simpleframe_t
			fs.seekg(sizeof(uint32_t) * 2, std::ios_base::cur);// bboxmin, bboxmax

			char name[16];
			fs.read((char *)&name, sizeof(char) * 16);
			std::cout << "name: " << name << std::endl;

			// mdl_vertex_t
			vec3_t v;
			mdl_vertex_t vertex;
			for (uint32_t i = 0; i < mdl_header.num_verts; i++) {
				fs.read((char *)&vertex, sizeof(mdl_vertex_t));
				v.x = mdl_header.scale.x * vertex.v[0] + mdl_header.translate.x;
				v.y = mdl_header.scale.y * vertex.v[1] + mdl_header.translate.y;
				v.z = mdl_header.scale.z * vertex.v[2] + mdl_header.translate.z;
				vertices.push_back(v);
			}

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

			ofs << "o mdl" << std::endl;
			for (std::vector<vec3_t>::iterator it = vertices.begin(); it != vertices.end(); it++) {
				ofs << "v " << it->x << " " << it->y << " " << it->z << " " << std::endl;
			}

			for (std::vector<face_t>::iterator it = faces.begin(); it != faces.end(); it++) {
				ofs << "f " << it->a + 1 << " " << it->b + 1 << " " << it->c + 1 << " " << std::endl;
			}

			ofs.close();
		}
		//std::cout << file_header.name << std::endl;
	}

	if (!file_present) {
		std::cout << "There is no such file entry in pak." << std::endl;
	}

	std::cout << "Done." << std::endl;
	return 0;
}

