#pragma once
#include "common.h"
#include <cmath>

#define EPS_FLOAT 0.001f
#define VEC3X vec3(1.0f,0.0f,0.0f)
#define VEC3Y vec3(0.0f,1.0f,0.0f)
#define VEC3Z vec3(0.0f,0.0f,1.0f)
#define EQUAL(a,b) (abs(a-b)<EPS_FLOAT)

namespace Festa {
	inline quat rotation(float angle, const vec3& axis) {
		const float a = angle / 2.0f, s = sinf(a);
		return quat(cosf(a), axis.x * s, axis.y * s, axis.z * s);
	}

	inline quat RotationBetweenVectors(vec3 start, vec3 dest) {
		start = normalize(start);
		dest = normalize(dest);

		float cosTheta = dot(start, dest);
		vec3 rotationAxis;

		if (cosTheta < -1.0f + EPS_FLOAT) {
			rotationAxis = cross(vec3(0.0f, 0.0f, 1.0f), start);
			if (length(rotationAxis) < 0.01) // bad luck, they were parallel, try again!
				rotationAxis = cross(vec3(1.0f, 0.0f, 0.0f), start);

			rotationAxis = normalize(rotationAxis);
			return rotation(PI, rotationAxis);
		}

		rotationAxis = cross(start, dest);

		float s = sqrtf((1 + cosTheta) * 2);
		float invs = 1 / s;

		return quat(
			s * 0.5f,
			rotationAxis.x * invs,
			rotationAxis.y * invs,
			rotationAxis.z * invs
		);

	}

	struct Rotation {
		quat rot = quat(1.0f, 0.0f, 0.0f, 0.0f);
		Rotation() {}
		Rotation(const quat& rot) :rot(rot) {}
		Rotation(float angle, const vec3& axis) {
			const float a = angle * 0.5f, s = sinf(a);
			rot = quat(cosf(a), axis.x * s, axis.y * s, axis.z * s);
		}
		Rotation(const vec3& from, const vec3& to) {
			rot = RotationBetweenVectors(from, to);
		}
		Rotation(float roll, float pitch, float yaw) {
			//Rotation rx(roll, VEC3X),ry(pitch, VEC3Y),rz(yaw, VEC3Z);
			rot = (Rotation(roll, VEC3X) + Rotation(pitch, VEC3Y) + Rotation(yaw, VEC3Z)).rot;
			//rot = (rz+ry+rx).rot;
		}
		vec3 toEulerAngle() {
			vec3 euler;

			// roll (x-axis rotation)
			float sinr_cosp = 2 * (rot.w * rot.x + rot.y * rot.z);
			float cosr_cosp = 1 - 2 * (rot.x * rot.x + rot.y * rot.y);
			euler.x = atan2f(sinr_cosp, cosr_cosp);

			// pitch (y-axis rotation)
			float sinp = 2 * (rot.w * rot.y - rot.z * rot.x);
			//if (std::abs(sinp) >= 1)
				//euler.y = std::copysignf(PI / 2, sinp); // use 90 degrees if out of range
			//else
			euler.y = asinf(sinp);

			// yaw (z-axis rotation)
			float siny_cosp = 2 * (rot.w * rot.z + rot.x * rot.y);
			float cosy_cosp = 1 - 2 * (rot.y * rot.y + rot.z * rot.z);
			euler.z = atan2f(siny_cosp, cosy_cosp);

			return euler;
		}
		mat4 toMatrix()const {
			return glm::toMat4(rot);
		}
		void rotate(const Rotation& other) {
			rot = other.rot * rot;
		}
		Rotation operator+(const Rotation& other)const {
			return Rotation(other.rot * rot);
		}
		void operator+=(const Rotation& other) {
			rotate(other);
		}
		bool operator==(const Rotation& other)const {
			return abs(glm::dot(rot, other.rot) - 1.0f) < EPS_FLOAT;
		}
		Rotation operator-()const {
			return quat(rot.w, -rot.x, -rot.y, -rot.z);
		}
		vec3 operator*(const vec3& v)const {
			return rot * v;
		}
		Rotation operator*(float v)const {
			return Rotation(glm::angle(rot) * v, glm::axis(rot));
		}
		void operator*=(float v) {
			*this = Rotation(glm::angle(rot) * v, glm::axis(rot));
		}
	};



	struct Position {
		vec3 pos = vec3(0.0f);
		Position() {}
		Position(const vec3& pos) :pos(pos) {}
		Position(float x, float y, float z) :pos(vec3(x, y, z)) {}
		mat4 toMatrix()const {
			return translate4(pos);
		}
		void rotate(const vec3& center, const Rotation& rot) {
			pos -= center;
			rotate(rot);
			pos += center;
		}
		void rotate(const Rotation& rot) {
			pos = rot.rot * pos;
		}
		void translate(const Position& other) {
			pos += other.pos;
		}
		Position operator+(const Position& other)const {
			return pos + other.pos;
		}
		void operator+=(const Position& other) {
			translate(other);
		}
		bool operator==(const Position& other)const {
			return EQUAL(pos.x, other.pos.x) && EQUAL(pos.y, other.pos.y) && EQUAL(pos.z, other.pos.z);
		}
		Position operator-()const {
			return -pos;
		}
		vec3 operator*(const vec3& v)const {
			return v + pos;
		}
	};

	struct Scaling {
		vec3 scaling = vec3(1.0f);
		Scaling() {}
		Scaling(const vec3& scaling) :scaling(scaling) {}
		Scaling(float x, float y, float z) :scaling(vec3(x, y, z)) {}
		Scaling(float s) :scaling(vec3(s)) {}
		mat4 toMatrix()const {
			return scale4(scaling);
		}
		void scale(const Scaling& other) {
			scaling *= other.scaling;
		}
		Scaling operator+(const Scaling& other)const {
			return scaling * other.scaling;
		}
		void operator+=(const Scaling& other) {
			scale(other);
		}
		bool operator==(const Scaling& other)const {
			return EQUAL(scaling.x, other.scaling.x) && EQUAL(scaling.y, other.scaling.y) && EQUAL(scaling.z, other.scaling.z);
		}
		Scaling operator-()const {
			return vec3(1.0f / scaling.x, 1.0f / scaling.y, 1.0f / scaling.z);
		}
		vec3 operator*(const vec3& v)const {
			return v * scaling;
		}
	};

	struct Transform {
		Position pos;
		Rotation rot;
		Scaling scaling;
		Transform() {}
		Transform(const Position& pos, const Rotation& rot, const  Scaling& scaling) :pos(pos), rot(rot), scaling(scaling) {}
		void translate(const Position& other) {
			pos += other;
		}
		void rotate(const Rotation& other) {
			rot += other;
		}
		void scale(const Scaling& other) {
			scaling += other;
		}
		mat4 toMatrix()const {
			return pos.toMatrix() * rot.toMatrix() * scaling.toMatrix();
		}
		void operator=(const Transform& other) {
			pos = other.pos; rot = other.rot; scaling = other.scaling;
		}
		void transform(const Transform& other) {
			pos += other.pos; rot += other.rot; scaling += other.scaling;
		}
		void operator+=(const Transform& other) {
			transform(other);
		}
		Transform operator+(const Transform& other)const {
			return Transform(pos + other.pos, rot + other.rot, scaling + other.scaling);
		}
	};

	inline Position interpolate(const Position& a, const Position& b, float t) {
		return glm::mix(a.pos, b.pos, t);
	}

	inline Rotation interpolate(const Rotation& a, const Rotation& b, float t) {
		return glm::slerp(a.rot, b.rot, t);
	}

	inline Scaling interpolate(const Scaling& a, const Scaling& b, float t) {
		return glm::mix(a.scaling, b.scaling, t);
	}

	inline Transform interpolate(const Transform& a, const Transform& b, float t) {
		return Transform(interpolate(a.pos, b.pos, t), interpolate(a.rot, b.rot, t), interpolate(a.scaling, b.scaling, t));
	}

	inline vec3 rotateOri(const vec3& x,const vec3& ori,const Rotation& rot) {
		vec3 ret = x - ori;
		ret = rot*ret;
		return ret + ori;
	}
}
