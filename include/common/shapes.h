#pragma once

#include "transform.h"
#include "../mesh.h"

namespace Festa {
	class TriangleMesh {
	public:
		struct Triangle {
			vec3 vertices[3];
		};
		std::vector<Triangle> mesh;
		TriangleMesh() {}
		TriangleMesh(const Mesh& m) {
			init(m);
		}
		TriangleMesh(const Meshes& meshes) {
			for (Meshes::TriMesh m : meshes.meshes)init(m.mesh);
		}
		uint numVertices()const {
			return uint(mesh.size() * 3);
		}
		uint numTriangles()const {
			return uint(mesh.size());
		}
		const float* ptr()const {
			return &(mesh[0].vertices[0].x);
		}
		Mesh toMesh()const {
			return Mesh(std::vector<float>(ptr(), ptr() + numVertices() * 3), "3");
		}
		void operator+=(const TriangleMesh& other) {
			mesh.insert(mesh.end(), other.mesh.begin(), other.mesh.end());
		}
		TriangleMesh operator+(const TriangleMesh& other)const {
			TriangleMesh res = TriangleMesh(*this);
			res += other;
			return res;
		}
	private:
		void init(const Mesh& m) {
			uint pre = uint(mesh.size()), stride = m.format.stride;
			if (m.indices.size()) {
				uint vcount = uint(m.indices.size());
				mesh.resize(pre + vcount / 3);
				for (uint i = 0; i < vcount / 3; i++) {
					for (uint j = 0; j < 3; j++) {
						uint index = m.indices[i * 3 + j] * stride;
						mesh[pre + i].vertices[j] = vec3(m.vertices[index], m.vertices[index + 1], m.vertices[index + 2]);
					}
				}
			}
			else {
				uint vcount = uint(m.vertices.size()) / stride;
				mesh.resize(pre + vcount / 3);
				for (uint i = 0; i < vcount / 3; i++) {
					for (uint j = 0; j < 3; j++) {
						uint index = (i * 3 + j) * stride;
						mesh[pre + i].vertices[j] = vec3(m.vertices[index], m.vertices[index + 1], m.vertices[index + 2]);
					}
				}
			}
		}
	};

	class AABB {
	public:
		vec3 min = vec3(INFINITY), max = vec3(-INFINITY);
		AABB() {}
		AABB(const vec3& size) {
			max = size * 0.5f;
			min = -max;
		}
		AABB(const vec3& min, const vec3& max)
			:min(min), max(max) {}
		AABB(const std::vector<vec3>& vertices) {
			update(vertices);
		}
		AABB(const Mesh& mesh) {
			update(mesh);
		}
		AABB(const Meshes& meshes) {
			update(meshes);
		}
		void clear() {
			vec3 min = vec3(INFINITY), max = vec3(-INFINITY);
		}
		bool empty()const {
			return min.x >= INFINITY;
		}
		void updateX(float x) {
			if (x < min.x)min.x = x;
			if (x > max.x)max.x = x;
		}
		void updateY(float y) {
			if (y < min.y)min.y = y;
			if (y > max.y)max.y = y;
		}
		void updateZ(float z) {
			if (z < min.z)min.z = z;
			if (z > max.z)max.z = z;
		}
		void update(float x, float y, float z) {
			updateX(x); updateY(y); updateZ(z);
		}
		void update(const vec3& v) {
			update(v.x, v.y, v.z);
		}
		void update(const std::vector<vec3>& vertices) {
			for (uint i = 0; i < vertices.size(); i++) update(vertices[i]);
		}
		void update(const Mesh& mesh) {
			uint stride = mesh.format.stride, size = uint(mesh.vertices.size());
			for (uint i = 0; i < size; i += stride)
				update(mesh.vertices[i], mesh.vertices[i + 1], mesh.vertices[i + 2]);
		}
		void update(const Meshes& meshes) {
			for (uint i = 0; i < meshes.meshes.size(); i++)update(meshes.meshes[i].mesh);
		}
		void update(const AABB& aabb) {
			update(aabb.min); update(aabb.max);
		}
		void translate(const Position& pos) {
			min += pos.pos; max += pos.pos;
		}
		void rotate(const Rotation& rot) {
			*this *= rot.toMatrix();
		}
		void scale(const Scaling& scaling) {
			min *= scaling.scaling; max *= scaling.scaling;
		}
		vec3 size()const {
			return max - min;
		}
		vec3 center()const {
			return (min + max) / 2.0f;
		}
		std::vector<vec3> vertices()const {
			return {
				vec3(min.x,min.y,min.z),vec3(min.x,min.y,max.z),
				vec3(min.x,max.y,min.z),vec3(min.x,max.y,max.z),
				vec3(max.x,min.y,min.z),vec3(max.x,min.y,max.z),
				vec3(max.x,max.y,min.z),vec3(max.x,max.y,max.z),
			};
		}
		void operator*=(const mat4& trans) {
			std::vector<vec3> v = vertices();
			clear();
			for (vec3 x : v) {
				vec3 y = trans * vec4(x, 1.0f);
				update(y.x, y.y, y.z);
			}
		}
		AABB operator*(const mat4& trans)const {
			AABB res;
			std::vector<vec3> v = vertices();
			for (vec3 x : v) {
				vec3 y = trans * vec4(x, 1.0f);
				res.update(y.x, y.y, y.z);
			}
			return res;
		}
		Transform cubeTransform()const {
			return Transform(center(), Rotation(), size());
		}

	};

	class SphereShape {
	public:
		vec3 center = vec3(0.0f); float radius = 0.0f;
		SphereShape() {}
		SphereShape(const vec3& center, float radius) :center(center), radius(radius) {}
		SphereShape(const vec3& c, const vec3& p) {
			init(c, p);
		}
		SphereShape(const AABB& aabb) {
			init(aabb.center(), aabb.min);
		}
		void translate(const Position& pos) {
			center += pos.pos;
		}
		void scale(float scaling) {
			radius *= scaling;
		}

		void scale(const Scaling& scaling) {
			scale(std::max(std::max(scaling.scaling.x, scaling.scaling.y), scaling.scaling.z));
		}
		void scale_ori(const Scaling& scaling) {
			float s = std::max(std::max(scaling.scaling.x, scaling.scaling.y), scaling.scaling.z);
			radius *= s; center *= s;
		}
		void init(const vec3& c, const vec3& p) {
			center = c;
			radius = length(center - p);
		}
		void operator*=(const mat4& mat) {
			init(vec4(center, 1.0f) * mat, vec4(center.x + radius, center.y, center.z, 1.0f) * mat);
		}
		SphereShape operator*(const mat4& mat)const {
			SphereShape res;
			res.init(vec4(center, 1.0f) * mat, vec4(center.x + radius, center.y, center.z, 1.0f) * mat);
			return res;
		}
	};

	struct Ray {
		vec3 ori, dir;
		Ray() { ori = dir = vec3(0.0f); }
		Ray(const vec3& ori, const vec3& dir) :ori(ori), dir(dir) {}
		void init(const vec3& start, const vec3& end) {
			ori = start;
			dir = normalize(end - start);
		}
	};

	struct PlaneShape {
		vec3 ori, normal;
		PlaneShape() { ori = normal = vec3(0.0f); }
		PlaneShape(const vec3& ori, const vec3& normal) 
			:ori(ori), normal(normal) {}
		float distance(const vec3& pos)const {
			return glm::dot(ori - pos,-normal);
		}
		vec3 perpendicular(const vec3& pos)const {
			return -normal * glm::dot(ori - pos, -normal);
		}
	};

	struct Viewport {
		int x, y, w, h;
		Viewport() { x = y = w = h = 0; }
		template<typename T>
		Viewport(T _w, T _h) :x(0), y(0), w(int(_w)), h(int(_h)) {}
		template<typename T>
		Viewport(T _x, T _y, T _w, T _h) 
			:x(int(_x)), y(int(_y)), w(int(_w)), h(int(_h)) {}
		void apply()const {
			glViewport(x, y, w, h);
		}
		mat4 getMatrix()const {
			float w2 = float(w) / 2.0f, h2 = float(h) / 2.0f;
			return mat4(
				w2, 0.0f, 0.0f, 0.0f,
				0.0f, h2, 0.0f, 0.0f,
				0.0f, 0.0f, 0.5f, 0.0f,
				float(x) + w2, float(y) + h2, 0.5f, 1.0f
			);
		}
		vec3 ndc2screen(const vec3& pos)const {
			float w2 = float(w) / 2.0f, h2 = float(h) / 2.0f;
			return vec3(pos.x * w2 + float(x) + w2, pos.y * h2 + float(y) + h2, pos.z * 0.5f + 0.5f);
		}
		vec3 screen2ndc(const vec3& pos)const {
			float w2 = float(w) / 2.0f, h2 = float(h) / 2.0f;
			return vec3((pos.x - float(x) - w2) / w2, (pos.y - float(y) - h2) / h2, (pos.z - 0.5f) * 2.0f);
		}
		vec2 size()const {
			return vec2(float(w), float(h));
		}
		vec2 pos()const {
			return vec2(float(x), float(y));
		}
		void load() {
			glGetIntegerv(GL_VIEWPORT, &x);
		}
	};

	struct CameraShape {
		float zNear, zFar, left, right, bottom, top;
		bool frustum = true;
		CameraShape() { zNear = zFar = left = right = bottom = top = 0; }
		CameraShape(float left, float right, float bottom, float top, float zNear, float zFar, bool frustum = true) :
			left(left), right(right), bottom(bottom), top(top), zNear(zNear), zFar(zFar), frustum(frustum) {}
		CameraShape(int w, int h, float zNear, float zFar, bool frustum = true) :zNear(zNear), zFar(zFar), frustum(frustum) {
			float w2 = float(w) / 2.0f, h2 = float(h) / 2.0f;
			left = -w2; right = w2;
			bottom = -h2; top = h2;
		}
		CameraShape(float fovy, float aspect, float zNear, float zFar) {
			perspective(fovy, aspect, zNear, zFar);
		}
		CameraShape(const Viewport& viewport, float fovy, float zNear, float zFar) {
			perspective(fovy, float(viewport.w) / float(viewport.h), zNear, zFar);
		}
		CameraShape(const Viewport& viewport, float zNear, float zFar) :zNear(zNear), zFar(zFar) {
			frustum = false;
			float w2 = float(viewport.w) / 2.0f, h2 = float(viewport.h) / 2.0f;
			left = -w2; right = w2;
			bottom = -h2; top = h2;
		}
		void perspective(float fovy, float aspect, float zNear, float zFar) {
			frustum = true;
			this->zNear = zNear; this->zFar = zFar;
			float h2 = zNear * tanf(fovy / 2.0f), w2 = h2 * aspect;
			left = -w2; right = w2;
			bottom = -h2; top = h2;
		}
		mat4 getMatrix()const {
			if (frustum)return glm::frustum(left, right, bottom, top, zNear, zFar);
			else return glm::ortho(left, right, bottom, top, zNear, zFar);
		}
	};

	class Camera {
	public:
		vec3 pos = vec3(0.0f);
		float roll = 0.0f, pitch = 0.0f, yaw = 0.0f;
		CameraShape shape;
		Camera() {}
		Camera(const CameraShape& shape) :shape(shape) {}
		mat4 view()const {
			return eulerAngle4(-roll, -pitch, -yaw) * translate4(-pos);
		}
		mat4 projection()const {
			return shape.getMatrix();
		}
		void setEulerAngle(const vec3& eulerAngle) {
			roll = eulerAngle.x; pitch = eulerAngle.y; yaw = eulerAngle.z;
		}
		vec3 getEulerAngle()const {
			return vec3(roll,pitch,yaw);
		}
		Rotation getRotation()const {
			return Rotation(roll,pitch,yaw);
		}
		void translate(const Position& position) {
			pos += position.pos;
		}
		vec3 direction(const vec3& dir)const {
			return eulerAngle4(roll, pitch, yaw) * vec4(dir.x, dir.y, dir.z, 1.0f);
		}
		vec3 right()const {
			return direction(VEC3X);
		}
		vec3 up()const {
			return direction(VEC3Y);
		}
		vec3 front()const {
			return direction(vec3(0.0f, 0.0f, -1.0f));
		}
		void glProjection()const {
			glMatrixMode(GL_PROJECTION);
			mat4 mat = projection();
			glLoadMatrixf(&mat[0][0]);
		}
		void glModelView(const mat4& model)const {
			glMatrixMode(GL_MODELVIEW);
			mat4 mat = view() * model;
			glLoadMatrixf(&mat[0][0]);
		}
		void glView()const {
			glMatrixMode(GL_MODELVIEW);
			mat4 mat = view();
			glLoadMatrixf(&mat[0][0]);
		}
		vec4 world2clip(const vec3& pos)const {
			return projection() * (view() * vec4(pos, 1.0f));;
		}
		vec3 world2ndc(const vec3& pos)const {
			vec4 v = world2clip(pos);
			return vec3(v) / v.w;
		}
		vec3 world2screen(const vec3& pos, const Viewport& viewport)const {
			return viewport.ndc2screen(world2ndc(pos));
		}
		bool visible(const vec3& pos)const {
			vec3 v = world2ndc(pos);
			return !(v.x < -1.0f || v.x>1.0f ||
				v.y < -1.0f || v.y>1.0f ||
				v.z < -1.0f || v.z>1.0f);
		}
		bool visible(const AABB& aabb)const {
			std::vector<vec3> ver = aabb.vertices();
			for (vec3& v : ver)if (visible(v))return true;
			return false;
		}
		void bind(const std::string& projection_ = "projection", const std::string& view_ = "view")const {
			if (!Program::activeProgram)return;
			Program::activeProgram->setMat4(projection_, projection());
			Program::activeProgram->setMat4(view_, view());
		}
		void rotate(const vec3& ori,const vec3& eulerAngle) {
			roll += eulerAngle.x; pitch += eulerAngle.y; yaw += eulerAngle.z;
			pos = rotateOri(pos, ori, Rotation(eulerAngle.x,eulerAngle.y,eulerAngle.z));
		}
	};
}
