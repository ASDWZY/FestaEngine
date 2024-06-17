#pragma once

#include "Program.h"
#include "animation.h"
#include "common/shapes.h"

#define INVERSE_COLOR mat4(-1.0f, 0.0f, 0.0f, 0.0f,\
							0.0f, -1.0f, 0.0f, 0.0f,\
							0.0f, 0.0f, -1.0f, 0.0f,\
							1.0f, 1.0f, 1.0f, 1.0f)
#define GRAY_COLOR_(r,g,b) mat4(r,r,r, 0.0f,\
							g, g, g, 0.0f,\
							b, b, b, 0.0f,\
							0.0f, 0.0f, 0.0f, 1.0f)
#define GRAY_COLOR GRAY_COLOR_(0.2126f,0.7152f,0.0722f)


namespace Festa {
	struct VAOSource {
		VAOSource(Mesh& mesh) :mesh(&mesh) {}
		VAO& get() {
			if (!vao)vao.ptr = new _VAO(*mesh);
			return vao;
		}
		Mesh* mesh = 0;
		VAO vao;
	};



	struct ShaderSource {
		ShaderSource(int type, const char* code) :type(type), code(code) {
			shader = 0;
		}
		Shader& get() {
			if(!shader) shader = new Shader(type, code);
			return *shader;
		}
		Shader* shader;
		const char* code;
		int type;
	};

	struct ProgramSource {
		ProgramSource(const std::vector<ShaderSource>& shaders) :shaders(shaders) {
			program = 0;
		}
		Program& get() {
			if(!program) {
				std::vector<uint> ids;
				for (ShaderSource s : shaders)ids.push_back(s.get().id);
				program = new Program(ids);
			}
			return *program;
		}
		
		Program* program;
		std::vector<ShaderSource> shaders;
	};

	extern Mesh RECT2MESH, RECT332MESH, CUBE332MESH, CUBELINEMESH;
	extern VAOSource RECT2, RECT332, CUBE332, CUBELINE;
	extern ShaderSource TEXTURE_VS, TEXTURE_FS, SKYBOX_VS, SKYBOX_FS, STANDARD_VS,STANDARD_FS, PICKUP_FS;
	extern ProgramSource TEXTURE_PROGRAM, SKYBOX_PROGRAM;

	void drawTexture(const Texture& texture, const mat4& posTrans = mat4(1.0f), const mat4& colorTrans = mat4(1.0f));
	void drawTexture3D(const Texture& texture, const Camera& camera,const Transform& transform, const mat4& colorTrans = mat4(1.0f));
	void drawCuboid(const Material& material, const mat4& trans = identity4());

	class GeometricModel :public Model {
	public:
		struct GeometricConfig {
			uint mesh=0;
			Material* material=0;
			mat4 trans;
			GeometricConfig() {
				trans = identity4();
			}
			GeometricConfig(uint mesh, Material* material_, const Transform& transform = Transform()) :
				mesh(mesh) {
				material = material_;
				trans = transform.toMatrix();
			}
			GeometricConfig(uint mesh,Material* material_, const mat4& trans) :
				mesh(mesh), material(material_), trans(trans) {}
		};
		std::vector<mat4> meshMatrices;
		GeometricModel() {}
		GeometricModel(const std::vector<GeometricConfig>& config, const std::vector<Mesh>& m) {
			append(config, m);
		}
		void operator*=(const mat4& mat) {
			for (mat4& m : meshMatrices)m *= mat;
		}
		void append(const std::vector<GeometricConfig>& config, const std::vector<Mesh>& m) {
			for (uint i = 0; i < config.size(); i++) {
				meshes.push_back(TriMesh());
				meshes.back().init(m[config[i].mesh], config[i].material);
				meshMatrices.push_back(config[i].trans);
			}
			aabb.clear();
			for (uint i = 0; i < m.size(); i++)
				aabb.update(AABB(m[i])* meshMatrices[i]);

		}
		void append(const std::vector<GeometricConfig>& config, const std::vector<VAO>& m) {
			for (uint i = 0; i < config.size(); i++) {
				meshes.push_back(TriMesh());
				meshes.back().vao = m[i];
				meshMatrices.push_back(config[i].trans);
			}
		}
		void draw()const {
			Program* program = Program::activeProgram;
			mat4 mat = toMatrix();
			for (int i = 0; i < meshes.size(); i++) {
				program->setMat4("model", mat * meshMatrices[i]);
				meshes[i].material->bind("material");
				meshes[i].vao->draw();
			}
			if (currentAnimation != -1)
				animations[currentAnimation]->setMatrices(program);
		}
		void drawInstanced(uint count)const {
			Program* program = Program::activeProgram;
			mat4 mat = toMatrix();
			for (int i = 0; i < meshes.size(); i++) {
				program->setMat4("model", mat * meshMatrices[i]);
				meshes[i].material->bind("material");
				meshes[i].vao->drawInstanced(count);
			}
			if (currentAnimation != -1)
				animations[currentAnimation]->setMatrices(program);
		}
		void cube(Material* material, const mat4& trans) {
			append({ GeometricConfig(0,material,trans) }, { CUBE332MESH });
		}
		void cone(Material* material, const mat4& trans, int n = 25) {
			Mesh mesh;
			mesh.cone(n);
			append({ GeometricConfig(0,material,trans) }, std::vector<Mesh>{ mesh });
		}
		void cylinder(Material* material, const mat4& trans, int n = 25) {
			Mesh mesh;
			mesh.cylinder(n);
			append({ GeometricConfig(0,material,trans) }, std::vector<Mesh>{ mesh });
		}
		void axis(Material* material, float ra = 0.5f, float ha = 1.5f, float rp = 0.15f, float hp = 2.5f, int n = 25) {
			Mesh cone, cylinder;
			cone.cone(n), cylinder.cylinder(n);
			append({ GeometricConfig(0,material,translate4(vec3(0.0f,hp,0.0f)) * scale4(vec3(ra * 2.0f,ha,ra * 2.0f))),
				   GeometricConfig(1,material,scale4(vec3(rp * 2.0f,hp,rp * 2.0f))) }, std::vector<Mesh>{ cone, cylinder });
		}
		void axes(float ambient = 0.2f, float diffuse = 0.8f, float ra = 0.5f, float ha = 1.5f, float rp = 0.15f, float hp = 2.5f, int n = 25) {
			Mesh cone, cylinder;
			cone.cone(n), cylinder.cylinder(n);
			mat4 conet = translate4(vec3(0.0f, hp, 0.0f)) * scale4(vec3(ra * 2.0f, ha, ra * 2.0f)),
				cylindert = scale4(vec3(rp * 2.0f, hp, rp * 2.0f));
			mat4 x = Rotation(-radians(90.0f), VEC3Z).toMatrix(), y = identity4(), 
				z = Rotation(radians(90.0f), VEC3X).toMatrix();
			Material* matx = new Material(VEC3X * ambient, VEC3X * diffuse, vec3(1.0f), 8.0f),
				* maty = new Material(VEC3Y * ambient, VEC3Y * diffuse, vec3(1.0f), 8.0f),
				* matz = new Material(VEC3Z * ambient, VEC3Z * diffuse, vec3(1.0f), 8.0f);
			append({ GeometricConfig(0,matx,x * conet),
					GeometricConfig(1,matx,x * cylindert),
					GeometricConfig(0,maty,y * conet),
					GeometricConfig(1,maty,y * cylindert) ,
					GeometricConfig(0,matz,z * conet),
					GeometricConfig(1,matz,z * cylindert) }, std::vector<Mesh>{ cone, cylinder });
		}
	};

	class Cubemap {
	public:
		Cubemap() {
			id = 0;
		}
		Cubemap(const std::vector<const Image*>& images) {
			init(images);
		}
		Cubemap(const std::vector<std::string>& paths) {
			std::vector<const Image*> images(paths.size());
			for (uint i = 0; i < paths.size(); i++)
				images[i] = new Image(paths[i]);
			init(images);
		}
		Cubemap(const std::string& folder, const std::vector<std::string>& files) {
			std::vector<const Image*> images(files.size());
			const std::string prefix = folder + "/";
			for (uint i = 0; i < files.size(); i++)
				images[i] = new Image(prefix + files[i]);
			init(images);
		}
		void release() {
			glDeleteTextures(1, &id);
		}
		~Cubemap() {
			//release();
		}
		void bind()const {
			glBindTexture(GL_TEXTURE_CUBE_MAP, id);
		}
		static void wrapping(int param = GL_CLAMP_TO_EDGE) {
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, param);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, param);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, param);
		}
		static void filter(int param = GL_LINEAR) {
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, param);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, param);
		}
		void draw(const Camera& camera, Program* program = 0) {
			if (!program)program = &SKYBOX_PROGRAM.get();
			program->bind();
			program->setMat4("projection", camera.projection());
			program->setMat4("view", mat4(mat3(camera.view())));
			bind();
			CUBE332.get()->draw();
		}
	private:
		uint id;
		void init(const std::vector<const Image*>& images) {
			glGenTextures(1, &id);
			bind();
			for (uint i = 0; i < images.size(); i++)
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
					images[i]->width(), images[i]->height(), 0, GL_BGR, GL_UNSIGNED_BYTE, images[i]->data());
			filter();
			wrapping();
		}
	};
}

