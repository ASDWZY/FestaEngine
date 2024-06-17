#pragma once

#include "../3rd/assimp/scene.h"
#include "../3rd/assimp/Importer.hpp"
#include "../3rd/assimp/postprocess.h"
#include <unordered_map>

#include "frame.h"
#include "common/transform.h"
#include "Program.h"

namespace Festa {

	struct PhongColor {
		vec3 ambient = vec3(0.0f), diffuse = vec3(0.0f), specular = vec3(0.0f);
		PhongColor() {
		}
		PhongColor(const vec3& ambient, const vec3& diffuse, const vec3& specular)
			:ambient(ambient), diffuse(diffuse), specular(specular) {}
		void load(const PhongColor& color) {
			ambient = color.ambient; diffuse = color.diffuse; specular = color.specular;
		}
	};
	struct Material :public PhongColor {
		float shininess;
		Texture diffuseMap;
		Texture specularMap;
		Material() {
			shininess = -1.0f;
		}
		Material(const vec3& ambient_, const vec3& diffuse_, const vec3& specular_, float shininess) :shininess(shininess) {
			ambient = ambient_; diffuse = diffuse_; specular = specular_;
		}
		Material(const PhongColor& color, float shininess) :shininess(shininess) {
			load(color);
		}
		Material(const Image& diffuseMap_, const vec3& specular_, float shininess)
			:shininess(shininess) {
			diffuseMap.init(diffuseMap_);
			specular = specular_;
		}
		Material(const Image& diffuseMap_, const Image& specularMap_, float shininess)
			:shininess(shininess) {
			diffuseMap.init(diffuseMap_);
			specularMap.init(specularMap_);
		}
		void release() {
			diffuseMap.release();
			specularMap.release();
		}
		~Material() {
			release();
		}
		void bind(const std::string& name, int diffuseMapID = 1, int specularMapID = 2,
			const std::string& diffuseMap_ = "diffuseMap", const std::string& specularMap_ = "specularMap",
			const std::string& ambient_ = "ambient", const std::string& diffuse_ = "diffuse",
			const std::string& specular_ = "specular", const std::string& shininess_ = "shininess")const {
			if (!Program::activeProgram)return;
			std::string prefix = name + ".";
			if (!diffuseMap) {
				Program::activeProgram->setVec3(prefix + ambient_, ambient);
				Program::activeProgram->setVec3(prefix + diffuse_, diffuse);
			}
			else {
				Program::activeProgram->setVec3(prefix + ambient_, vec3(-1.0f));
				Program::activeProgram->setVec3(prefix + diffuse_, vec3(-1.0f));
				diffuseMap.bind(prefix + diffuseMap_, diffuseMapID);
			}
			if (!specularMap) Program::activeProgram->setVec3(prefix + specular_, specular);
			else {
				Program::activeProgram->setVec3(prefix + specular_, vec3(-1.0f));
				specularMap.bind(prefix + specularMap_, specularMapID);
			}
			Program::activeProgram->setFloat(prefix + shininess_, shininess);
		}
	};


	struct DirLight :public PhongColor {
		vec3 dir;
		DirLight() { dir = vec3(0.0f); }
		DirLight(const vec3& ambient_, const vec3& diffuse_, const vec3& specular_) {
			ambient = ambient_; diffuse = diffuse_; specular = specular_;
			dir = vec3(0.0f, -1.0f, 0.0f);
		}
		DirLight(const vec3& dir, const vec3& ambient_, const vec3& diffuse_, const vec3& specular_) :dir(dir) {
			ambient = ambient_; diffuse = diffuse_; specular = specular_;
		}
		void rotate(float angle, const vec3& axis) {
			dir = glm::rotate(mat4(1.0f), angle, axis) * vec4(dir, 1.0f);
		}
		void rotate(float pitch, float yaw, float roll) {
			dir = eulerAngle4(pitch, yaw, roll) * vec4(dir, 1.0f);
		}
		//struct program defination(struct)
		void bind(const std::string& name, const std::string& dir_ = "dir",
			const std::string& ambient_ = "ambient", const std::string& diffuse_ = "diffuse",
			const std::string& specular_ = "specular")const {
			if (!Program::activeProgram)return;
			std::string prefix = name + ".";
			Program::activeProgram->setVec3(prefix + dir_, dir);
			Program::activeProgram->setVec3(prefix + ambient_, ambient);
			Program::activeProgram->setVec3(prefix + diffuse_, diffuse);
			Program::activeProgram->setVec3(prefix + specular_, specular);
		}
	};

	struct PointLight :public PhongColor {
		vec3 pos = vec3(0.0f);
		float constant = 0.0f, linear = 0.0f, quadratic = 0.0f;
		PointLight() {}
		PointLight(const vec3& pos, const vec3& ambient_, const vec3& diffuse_, const vec3& specular_, float constant = 0.0f, float linear = 0.0f, float quadratic = 0.0f) :
			pos(pos), constant(constant), linear(linear), quadratic(quadratic) {
			ambient = ambient_; diffuse = diffuse_; specular = specular_;
		}
		void bind(const std::string& name, const std::string& pos_ = "pos",
			const std::string& ambient_ = "ambient", const std::string& diffuse_ = "diffuse", const std::string& specular_ = "specular",
			const std::string& constant_ = "constant", const std::string& linear_ = "linear", const std::string& quadratic_ = "quadratic")const {
			if (!Program::activeProgram)return;
			std::string prefix = name + ".";
			Program& p = *Program::activeProgram;
			p.setVec3(prefix + pos_, pos);
			p.setVec3(prefix + ambient_, ambient);
			p.setVec3(prefix + diffuse_, diffuse);
			p.setVec3(prefix + specular_, specular);
			p.setFloat(prefix + constant_, constant);
			p.setFloat(prefix + linear_, linear);
			p.setFloat(prefix + quadratic_, quadratic);
		}
	};

	struct SpotLight :public PhongColor {
		vec3 pos = vec3(0.0f), dir = vec3(0.0f);
		float cutOff = 2.0f, outerCutOff = 2.0f;
		SpotLight() {  }
		SpotLight(const vec3& pos, const vec3& dir, const vec3& ambient_, const vec3& diffuse_, const vec3& specular_, float delta, float outerDelta = -1.0f) :pos(pos), dir(dir) {
			cutOff = cosf(delta / 2.0f);//delta half the angle
			if (outerDelta > 0.0f)outerCutOff = cosf(outerDelta / 2.0f);
			else outerCutOff = 2.0f;
			ambient = ambient_; diffuse = diffuse_; specular = specular_;
		}
		void rotate(float angle, const vec3& axis) {
			dir = glm::rotate(mat4(1.0f), angle, axis) * vec4(dir, 1.0f);
		}
		void rotate(float pitch, float yaw, float roll) {
			dir = eulerAngle4(pitch, yaw, roll) * vec4(dir, 1.0f);
		}
		void bind(const std::string& name, const std::string& pos_ = "pos", const std::string& dir_ = "dir",
			const std::string& ambient_ = "ambient", const std::string& diffuse_ = "diffuse",
			const std::string& specular_ = "specular", const std::string& cutOff_ = "cutOff", const std::string& outerCutOff_ = "outerCutOff")const {
			if (!Program::activeProgram)return;
			std::string prefix = name + ".";
			Program::activeProgram->setVec3(prefix + pos_, pos);
			Program::activeProgram->setVec3(prefix + dir_, dir);
			Program::activeProgram->setVec3(prefix + ambient_, ambient);
			Program::activeProgram->setVec3(prefix + diffuse_, diffuse);
			Program::activeProgram->setVec3(prefix + specular_, specular);
			Program::activeProgram->setFloat(prefix + cutOff_, cutOff);
			Program::activeProgram->setFloat(prefix + outerCutOff_, outerCutOff);
		}
	};

	struct Light :public PhongColor {
		vec3 pos = vec3(0.0f), dir = vec3(0.0f);
		float p1 = 0.0f, p2 = 0.0f, p3 = 0.0f;
		int type = -1;
		Light() {}
		Light(const DirLight& l) {
			load(l);
			dir = l.dir;
			type = 1;
		}
		Light(const PointLight& l) {
			load(l);
			pos = l.pos;
			p1 = l.constant, p2 = l.linear, p3 = l.quadratic;
			type = 2;
		}
		Light(const SpotLight& l) {
			load(l);
			pos = l.pos, dir = l.dir;
			p1 = l.cutOff, p2 = l.outerCutOff;
			type = 3;
		}
		~Light() {

		}
		bool isDirLight()const {
			return type == 1;
		}
		bool isPointLight()const {
			return type == 2;
		}
		bool isSpotLight()const {
			return type == 3;
		}
		DirLight getDirLight()const {
			return DirLight(dir, ambient, diffuse, specular);
		}
		PointLight getPointLight()const {
			return PointLight(pos, ambient, diffuse, specular, p1, p2, p3);
		}
		SpotLight getSpotLight()const {
			return SpotLight(pos, dir, ambient, diffuse, specular, p1, p2);
		}
		void bind(const std::string& name)const {
			if (isDirLight())getDirLight().bind(name);
			else if (isPointLight())getPointLight().bind(name);
			else getSpotLight().bind(name);
		}
	};

#define FLOAT_SIZE 4
#define INT_SIZE 4

	struct Vertex {
		vec3 position, normal;
		vec2 texCoord;
		Vertex() {
			position = vec3(0.0f); normal = vec3(0.0f); texCoord = vec2(0.0f);
		}
		Vertex(vec3 position, vec3 normal, vec2 texCoord) :position(position), normal(normal), texCoord(texCoord) {}
		Vertex(float x, float y, float z, float nx, float ny, float nz, float u, float v) {
			position = vec3(x, y, z);
			normal = vec3(nx, ny, nz);
			texCoord = vec2(u, v);
		}
		Vertex(float* v) {
			position = vec3(*v, *(v + 1), *(v + 2));
			normal = vec3(*(v + 3), *(v + 4), *(v + 5));
			texCoord = vec2(*(v + 6), *(v + 7));
		}
	};

	struct Triangle {
		vec3 a, b, c;
		Triangle() {
			a = b = c = vec3(0.0f);
		}
		Triangle(const vec3& a, const vec3& b, const vec3& c) :a(a), b(b), c(c) {}
		vec3 normal()const {
			vec3 ac = c - a, ab = b - a;
			return cross(ac, ab);
		}
		const float* getptr()const {
			return &a.x;
		}
	};

	struct AttribFormat {
		std::vector<uint> format;
		uint stride = 0;
		AttribFormat() { stride = 0; }
		AttribFormat(const std::string& s) {
			init(s);
		}
		AttribFormat(const char* s) {
			init(s);
		}
		void init(const std::string& s) {
			format.resize(s.size());
			for (uint i = 0; i < s.size();i++) {
				format[i]=uint(s[i] - '0');
				stride += format[i];
			}
		}
		AttribFormat(const std::vector<uint>& format) :format(format) {
			for (uint i : format)stride += i;
		}
		uint size()const {
			return (uint)format.size();
		}
		uint operator[](uint index)const {
			return format[index];
		}
		void append(uint size) {
			format.push_back(size);
			stride += size;
		}
		AttribFormat operator+(const AttribFormat& other)const {
			AttribFormat res = *this;
			res += other;
			return res;
		}
		void operator+=(const AttribFormat& other) {
			stride += other.stride;
			format.insert(format.end(), other.format.begin(), other.format.end());
		}
		bool operator==(const AttribFormat& other)const {
			if (stride != other.stride || size() != other.size())return false;
			for (uint i = 0; i < size(); i++)if (format[i] != other.format[i])return false;
			return true;
		}
		bool operator!=(const AttribFormat& other)const {
			return !(*this == other);
		}
		std::string toString()const {
			std::string ret;
			for (uint i : format)ret.push_back('0' + i);
			return ret;
		}
	};

	struct Mesh {
		std::vector<float> vertices;
		std::vector<uint> indices;
		AttribFormat format;
		Mesh() {}
		Mesh(const std::vector<float>& vertices, const AttribFormat& format) :vertices(vertices), format(format) {}
		Mesh(const std::vector<float>& vertices, const std::vector<uint>& indices, const AttribFormat& format) :vertices(vertices), indices(indices), format(format) {}
		Mesh(std::vector<Vertex>& v) {
			float* start = &(v[0].position.x);
			vertices = std::vector<float>(start, start + v.size() * 8);
			format = "332";
		}
		Mesh(std::vector<Vertex>& v, std::vector<uint>& indices) :indices(indices) {
			float* start = &(v[0].position.x);
			vertices = std::vector<float>(start, start + v.size() * 8);
			format = "332";
		}
		void circle(int n = 25);
		void cone(int n = 25);
		void cylinder(int n = 25);
		void operator+=(const Mesh& mesh) {
			if (mesh.format != format)
				LOGGER.error("Different mesh attrib format: " + format.toString() + " != " + mesh.format.toString());
			vertices.insert(vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
			indices.insert(indices.begin(), mesh.indices.begin(), mesh.indices.end());
		}
		Mesh operator+(const Mesh& mesh)const {
			Mesh res = Mesh(*this);
			res += mesh;
			return res;
		}
	};
	extern Mesh RECT2MESH, RECT332MESH, CUBE332MESH, CUBELINEMESH;

	class _VAO {
	public:
		_VAO() {
			vao = nformat = vcount = icount = 0;
		}
		_VAO(const Mesh& mesh) {
			init(mesh);
		}
		void init(const Mesh& mesh) {
			vcount = (uint)mesh.vertices.size();
			icount = (uint)mesh.indices.size();
			glGenVertexArrays(1, &vao); bind();
			vbo.init(vcount * FLOAT_SIZE, &mesh.vertices[0]);
			if (icount)ebo.init(icount * INT_SIZE, &mesh.indices[0], GL_ELEMENT_ARRAY_BUFFER);
			nformat = mesh.format.size();
			uint stride = mesh.format.stride, addr = 0;

			vcount /= stride;
			stride *= FLOAT_SIZE;
			for (uint i = 0; i < nformat; i++) {
				vertexAttrib(i, mesh.format[i], stride, addr);
				addr += mesh.format[i] * FLOAT_SIZE;
			}
		}
		void release() {
			glDeleteVertexArrays(1, &vao);
		}
		~_VAO() {
			release();
		}
		static void vertexAttrib(uint index, int size, int stride, uint addr, uint type = GL_FLOAT) {
			glVertexAttribPointer(index, size, type, GL_FALSE, stride, (void*)ull(addr));
			glEnableVertexAttribArray(index);
		}
		static void attribFormat(const AttribFormat& format) {
			uint stride = format.stride * FLOAT_SIZE, addr = 0;
			for (uint i = 0; i < format.size(); i++) {
				vertexAttrib(i, format[i], stride, addr);
				addr += format[i] * FLOAT_SIZE;
			}
		}
		void bind()const {
			glBindVertexArray(vao);
		}
		static void unbind() {
			glBindVertexArray(0);
		}
		void instanceBuffer(GLBuffer buffer, const std::string& format) {
			bind();
			buffer.bind(GL_ARRAY_BUFFER);
			uint size = 0;
			for (int i = 0; i < format.size(); i++)size += uint(format[i] - '0');
			size *= FLOAT_SIZE;
			uint addr = 0;
			for (int i = 0; i < format.size(); i++) {
				int len = format[i] - '0';
				vertexAttrib(nformat + i, len, size, addr);
				glVertexAttribDivisor(nformat + i, 1);
				addr += len * FLOAT_SIZE;
			}
			nformat += uint(format.size());
		}
		void draw(uint mode = GL_TRIANGLES)const {
			bind();
			if (icount)glDrawElements(mode, icount, GL_UNSIGNED_INT, 0);
			else glDrawArrays(mode, 0, vcount);
		}
		void drawInstanced(uint count, uint mode = GL_TRIANGLES)const {
			bind();
			if (icount)glDrawElementsInstanced(mode, icount, GL_UNSIGNED_INT, 0, count);
			else glDrawArraysInstanced(mode, 0, vcount, count);
		}
		bool hasIndices()const {
			return icount;
		}
	private:
		uint vao, vcount, icount, nformat;
		GLBuffer vbo, ebo;
	};

	typedef amr_ptr<_VAO> VAO;



	class Meshes {
	public:
		struct BoneData {
			uint id = 0;
			mat4 offset = mat4(1.0f);
		};

		struct MeshWithMaterial {
			Mesh mesh;const Material* material;
		};
		struct TriMesh {
			Mesh mesh;
			const Material* material = 0;
			uint boneid = 0;
			std::string name;
		};
		std::vector<TriMesh> meshes;
		std::unordered_map<std::string, BoneData> bones;
		Meshes() {}
		Meshes(const std::vector<Mesh>& m, const std::vector<const Material*>& mats) {
			uint size = uint(m.size());
			if (mats.size() != size)
				LOGGER.error("Different count of meshes and materials: num_mesh=" + toString(size) + ", num_materials=" + toString(mats.size()));
			meshes.resize(size);
			for (uint i = 0; i < size; i++) {
				meshes[i] = TriMesh({ m[i], mats[i],0,"mesh_" + toString(i) });
			}
		}
		Meshes(const std::vector<MeshWithMaterial>& m) {
			uint size = uint(m.size());
			meshes.resize(size);
			for (uint i = 0; i < size; i++) {
				meshes[i] = TriMesh({ m[i].mesh,m[i].material,0, "mesh_" + toString(i) });
			}
		}
	};
	inline mat4 convertMatrix(const aiMatrix4x4& from)
	{
		mat4 to;
		to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
		to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
		to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
		to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
		return to;
	}


	class Bone {
	public:
		struct PosKey {
			Position pos;
			double time=0.0;
		};
		struct RotKey {
			Rotation rot;
			double time=0.0;
		};
		struct ScaKey {
			Scaling sca;
			double time=0.0;
		};
		template<typename T>
		struct Key { T trans; double time = 0.0; };
		std::vector<PosKey> poskeys;
		std::vector<RotKey> rotkeys;
		std::vector<ScaKey> scakeys;
		std::string name;
		Transform trans;
		Bone() {
			reset();
		}
		Bone(const aiNodeAnim* anim, const std::string& name) :name(name) {
			reset();
			poskeys.resize(anim->mNumPositionKeys);
			rotkeys.resize(anim->mNumRotationKeys);
			scakeys.resize(anim->mNumScalingKeys);
			for (uint i = 0; i < anim->mNumPositionKeys; i++) {
				aiVector3D pos = anim->mPositionKeys[i].mValue;
				poskeys[i]={ Position(pos.x,pos.y,pos.z), anim->mRotationKeys[i].mTime };
			}
			for (uint i = 0; i < anim->mNumRotationKeys; i++) {
				aiQuaternion rot = anim->mRotationKeys[i].mValue;
				rotkeys[i]={ Rotation(quat(rot.w, rot.x, rot.y, rot.z)), anim->mRotationKeys[i].mTime };
			}
			for (uint i = 0; i < anim->mNumScalingKeys; i++) {
				aiVector3D sca = anim->mScalingKeys[i].mValue;
				scakeys[i]={ Scaling(sca.x,sca.y,sca.z), anim->mRotationKeys[i].mTime };
			}
		}
		void reset() {
			posIndex = rotIndex = scaIndex = 0;
		}
		Position getPos(uint index) {
			if (index >= poskeys.size())return Position();
			return poskeys[index].pos;
		}
		Rotation getRot(uint index) {
			if (index >= rotkeys.size())return Rotation();
			return rotkeys[index].rot;
		}
		Scaling getSca(uint index) {
			if (index >= scakeys.size())return Scaling();
			return scakeys[index].sca;
		}

		Transform update(float time) {
			const uint numPos = uint(poskeys.size()), numRot = uint(rotkeys.size()), numSca = uint(scakeys.size());
			while (posIndex < numPos && poskeys[posIndex].time < time)posIndex++;
			while (rotIndex < numRot && rotkeys[rotIndex].time < time)rotIndex++;
			while (scaIndex < numSca && scakeys[scaIndex].time < time)scaIndex++;

			if (posIndex >= numPos)trans.pos = getPos(numPos - 1);
			else if (posIndex == 0)trans.pos = getPos(0);
			else trans.pos = interpolate(getPos(posIndex - 1), getPos(posIndex), getPosDelta(posIndex, time));

			if (rotIndex >= numRot)trans.rot = getRot(numRot - 1);
			else if (rotIndex == 0)trans.rot = getRot(0);
			else trans.rot = interpolate(getRot(rotIndex - 1), getRot(rotIndex), getRotDelta(rotIndex, time));

			if (scaIndex >= numSca)trans.scaling = getSca(numRot - 1);
			else if (scaIndex == 0)trans.scaling = getSca(0);
			else trans.scaling = interpolate(getSca(scaIndex - 1), getSca(scaIndex), getScaDelta(scaIndex, time));

			return trans;
		}

	private:
		uint posIndex, rotIndex, scaIndex;
		float getPosDelta(uint index, float time) {
			float pre = float(poskeys[index - 1].time), post = float(poskeys[index].time);
			return (time - pre) / (post - pre);
		}
		float getRotDelta(uint index, float time) {
			float pre = float(rotkeys[index - 1].time), post = float(rotkeys[index].time);
			return (time - pre) / (post - pre);
		}
		float getScaDelta(uint index, float time) {
			float pre = float(scakeys[index - 1].time), post = float(scakeys[index].time);
			return (time - pre) / (post - pre);
		}
	};
}




