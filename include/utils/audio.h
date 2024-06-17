#pragma once

#include "../common/shapes.h"
#include "../3rd/irrKlang/irrKlang.h"

using namespace irrklang;

#define vec3irr(t) vec3df(t.x,t.y,t.z)
#define irrvec3(t) vec3(t.X,t.Y,t.Z)


namespace Festa {
	class Sound {
	public:
		static ISoundEngine* engine;

		ISoundEffectControl* fx;
		bool sound3d;
		//virtual value
		VirtualValue<bool> paused;
		Sound() { sound = 0; fx = 0; sound3d = false; }
		Sound(const std::string& file, bool effects = false, bool looped = false){
			init(file, effects, looped);
		}
		Sound(const std::string& file, const vec3& pos, bool effects = false, bool looped = false) {
			init(file, pos, effects, looped);
		}
		void init(const std::string& file, bool effects = false, bool looped = false);
		void init(const std::string& file, const vec3& pos, bool effects = false, bool looped = false);
		void release()const {
			if(sound)sound->drop();
		}
		~Sound() {
			release();
		}
		bool isFinished()const {
			return sound->isFinished();
		}
		void finish()const {
			sound->stop();
		}
		void reload();
		void play() {
			if (isFinished())reload();
			sound->setIsPaused(false);
		}
		void pause()const {
			sound->setIsPaused(true);
		}
		void setIsLooped(bool value)const {
			sound->setIsLooped(value);
		}
		void setPos(const vec3& pos)const {
			sound->setPosition(vec3irr(pos));
		}
		void setDistance(float min, float max)const {
			//min distance: play at the maxium volume
			sound->setMinDistance(min);
			sound->setMaxDistance(max);
		}
		bool isPaused() {
			return sound->getIsPaused();
		}
		bool isLooped()const {
			return sound->getIsPaused();
		}
		vec3 pos()const {
			vec3df v = sound->getPosition();
			return irrvec3(v);
		}
		void setVolume(float volume)const {
			sound->setVolume(volume);
		}
		float getVolume()const {
			return sound->getVolume();
		}
		//[-1,1]
		void setPan(float pan)const {
			sound->setPan(pan);
		}
		float getPan()const {
			return sound->getPan();
		}
		static void setListener(const Camera& camera);
	private:
		ISound* sound;
		std::string file;
	};

};
