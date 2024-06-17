#pragma once

#include "../common/transform.h"
#include "../common/shapes.h"

static Festa::vec3 projection(const Festa::vec3 & a, const Festa::vec3 & dir) {
	return glm::dot(a, dir) / (dir.x*dir.x+dir.y*dir.y+dir.z*dir.z) * dir;
}

namespace Festa {
	typedef Ray Force3;

	inline void drawForce(const Force3& force) {
		GeometricModel axis; 
		_Material* mat=new _Material(vec3(0.0f),vec3(1.0f,0.0f,0.0f),vec3(0.1f),1.0f);
		axis.axis(mat);
		axis.scale(1.0f/axis.aabb.size().y);
		axis.rotate(Rotation(VEC3Y, force.dir));
		axis.translate(force.ori);
		axis.draw();
	}
	/*struct Torque {
		vec3 F=vec3(0.0f), arm=vec3(0.0f);

	};*/
	class Object3 {
	public:
		float mass = 0.0f, I = 0.0f;
		float elasticity = 0.0f,frication=0.0f;
		vec3 fl = vec3(0.0f),Torque=vec3(0.0f);
		vec3 vel = vec3(0.0f);
		Rotation alpha;

		vec3 center = vec3(0.0f);
		Rotation rot;
		Object3() {

		}
		Object3(float mass,float I):mass(mass),I(I) {

		}
		void applyForce(const vec3& force) {
			fl += force;
		}
		void applyForce(const Force3& force) {
			applyForce(force.ori - center,force.dir);
		}
		void clearForce() {
			fl = vec3(0.0f); Torque = vec3(0.0f);
		}
		void clear() {

		}
		//constant dt in the world
		void update(float dt) {
			if (mass <= EPS_FLOAT)return;
			vel += fl * dt/mass;
			center += vel * dt;
			//printvec3(Torque);
			const float l = length(Torque);
			if (l>=EPS_FLOAT) {
				//cout << l / I * dt * dt;
				alpha += Rotation(l/I*dt*dt, Torque / l);
				rot += alpha;
			}
			//rot += alpha;
			clearForce();
		}
		bool collision(float required,const vec3& pos,const vec3& normal,const Object3& obj) {
			const vec3 dir = pos - center;
			//drawForce(Force3(center, dir));
			const float len = length(dir);
			if (len > required)return false;
			center -= dir/len*(required-len);
			const vec3 v=projection(vel,dir);
			vel -= v * (1 + obj.elasticity);

			const vec3 arm = dir / len * required;
			const vec3 Fp = projection(fl,normal);
			const float f = 1.0f;//obj.friction
			vec3 F = -f * (fl - Fp);
			if (glm::dot(dir, Fp) > 0.0f)F -= Fp;
			//drawForce(Force3(center,fl));
			//drawForce(Force3(center+arm, -Fp - f * (fl - Fp)));
			applyForce(arm,F);
			return true;
		}
		Position getPosition()const {
			return center;
		}
		Rotation getRotation()const {
			return rot;
		}
	private:
		void applyForce(const vec3& arm,const vec3& force) {
			const vec3 F = projection(force, arm);
			Torque += cross(arm,force - F);
			fl += F;
			//alpha += Rotation(l / I * dt * dt, Torque / l);
		}
	};


}
