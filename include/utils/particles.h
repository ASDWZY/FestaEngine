#pragma once

#include "../common/common.h"
#include "../common/transform.h"

namespace Festa {
	struct Particle {
		Transform transform;
		vec3 vel=vec3(0.0f);
		float life=0.0f, lifetime=0.0f;
		Material* material = nullptr;
		void update(const vec3& acc,float dt) {
			if (dead())return;
			life += dt;
			vel += acc * dt;
			transform.pos.pos += vel * dt;
		}
		bool dead()const {
			return life >= lifetime;
		}
		void render() {
			Program::activeProgram->setMat4("model", transform.toMatrix());
			material->bind("material");
			RECT332.get()->draw();
		}
	};
	class ParticleSystemCPU {
	public:
		Program program;
		vector<Material*> materials;
		vector<Particle> particles;
		FrameTimer timer;
		ParticleSystemCPU() {}
		ParticleSystemCPU(Program* _program) {
			program = *_program;
		}
		~ParticleSystemCPU() {
			for (Material*& m : materials)SafeDelete(m);
		}
		virtual void initParticle(Particle& p)=0;
	};

	class FireCPU:public ParticleSystemCPU {
	public:
		float s,r, h, t;
		FireCPU() {
			
		}
		FireCPU(const Image& texture,int num,float s,float r,float h,float t)
		:s(s),r(r),h(h),t(t){
			materials.push_back(new Material(texture,vec3(0.0f),1.0f));
			particles.resize(num);
			program.init({STANDARD_VS.get().id,STANDARD_FS.get().id});
			for (Particle& p:particles) {
				initParticle(p);
			}
		}
		void initParticle(Particle& p) {
			p = Particle();
			p.material = materials[0];
			p.vel = vec3(0.0f, randf(0.0f, h/t), 0.0f);
			p.life = 0.0f;
			p.lifetime = randf(0.0f, t);
			p.transform.pos = vec3(normalf(0.0f,r),0.0f,normalf(0.0f,r));
			p.transform.scaling = randf(0.0f,s);
		}
		void render(const Camera& camera) {
			float dt = timer.interval();
			program.bind();
			camera.bind();
			for (Particle& p : particles) {
				if (p.dead())initParticle(p);
				p.update(vec3(0.0f), dt);
				p.transform.rot = Rotation(VEC3Z,-camera.front());
				p.render();
			}
		}
	};
}
