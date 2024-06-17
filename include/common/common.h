#pragma once

#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)

#define GLEW_STATIC 1
#include "../3rd/GL/glew.h"
#define GLFW_INCLUDE_NONE
#include "../3rd/GL/glfw3.h"
#include "../3rd/GL/GL.h"

#ifndef UNICODE
#define UNICODE
#endif
#include <Windows.h>

#include <map>
#include <stack>
#include <queue>
#include <list>
#include <functional>

#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

#include <cmath>

#include "../3rd/glm/glm.hpp"
#include "../3rd/glm/gtc/matrix_transform.hpp"
#include "../3rd/glm/gtc/type_ptr.hpp"
#include "../3rd/glm/gtx/euler_angles.hpp"
#include "../3rd/glm/gtx/quaternion.hpp"
#include "../3rd/glm/gtc/quaternion.hpp"
#include "../3rd/glm/common.hpp"

#include "String.h"


#define identity4() mat4(1.0f)

#define printvec2(v) Festa::printvec<vec2,2>(v)
#define printvec3(v) Festa::printvec<vec3,3>(v)
#define printvec4(v) Festa::printvec<vec4,4>(v)
#define printmat4(m) Festa::printmat<mat4,4>(m)

#define getTime() glfwGetTime()
//#define vector int

namespace Festa {
	typedef glm::mat4 mat4;
	typedef glm::mat3 mat3;
	typedef glm::mat2 mat2;
	typedef glm::vec4 vec4;
	typedef glm::vec3 vec3;
	typedef glm::vec2 vec2;
	typedef glm::ivec2 ivec2;
	typedef glm::quat quat;
	const float PI = glm::pi<float>();
	
	inline float distance(const vec3& a, const vec3& b) {
		return glm::length(a - b);
	}
	inline float radians(float degree) {
		return degree * PI / 180.0f;
	}
	inline float degree(float radians) {
		return radians * 180.0f / PI;
	}

	inline mat4 translate4(const vec3& pos) {
		return glm::translate(mat4(1.0f), pos);
	}
	inline mat4 eulerAngle4(const vec3& eulerAngle) {
		return glm::eulerAngleXYZ(eulerAngle.x, eulerAngle.y, eulerAngle.z);
	}
	inline mat4 eulerAngle4(float pitch, float yaw, float roll) {
		return glm::eulerAngleXYZ(pitch, yaw, roll);
	}
	inline mat4 rotate4(float angle, const vec3& axis) {
		return glm::rotate(mat4(1.0f), angle, axis);
	}
	inline mat4 scale4(const vec3& scaling) {
		return glm::scale(mat4(1.0f), scaling);
	}
	template<typename T>
	inline T sgn(T x) {
		const T z = T(0);
		if (x > z)return T(1);
		else if (x < z)return T(-1);
		else return z;
	}

	inline double roundf(double x, int prec) {
		int tmp = prec;
		while (tmp--)x *= 10.0;
		x = double(int(x + 0.5));
		tmp = prec;
		while (tmp--)x /= 10.0;
		return x;
	}
	inline double randf() {
		return double(rand()) / double(RAND_MAX);
	}
	inline double randf(double low, double high) {
		return low + randf() * (high - low);
	}
	inline double normalf(double u,double sigma) {
		double u1 = randf(), u2 = randf();
		return u+sigma*sqrt(-2.0*log(u1))*cos(2.0*PI*u2);
	}
	inline int randint() {
		return rand();
	}
	inline int randint(int low, int high) {
		return low + int(randf() * double(high - low));
	}

	template<typename T, int size>
	inline void printvec(const T& vec) {
		for (int i = 0; i < size; i++)std::cout << vec[i] << " ";
		std::cout << std::endl;
	}

	template<typename T, int size>
	inline void printmat(const T& mat) {
		// rx,ry,rz
		// +  +  +
		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++)std::cout << mat[i][j] << " ";
			std::cout << std::endl;
		}
	}

	template<typename T>
	void SafeDelete(T*& ptr) {
		if (ptr!=nullptr) { 
			try {
				delete ptr;
				//free(ptr);
				ptr = nullptr;
			}
			catch (...) {}
			 
		}
	}

	template<typename T>
	class amr_ptr {
	public:
		T* ptr = nullptr;
		amr_ptr() {}
		amr_ptr(const T& val) :ptr(new T(val)) {}
		amr_ptr(const amr_ptr<T>& p) {
			if (p.ptr != nullptr)ptr = new T(*p.ptr);
		}
		void operator=(const amr_ptr<T>& p) {
			SafeDelete(ptr);
			if (p.ptr != nullptr)ptr = new T(*p.ptr);
		}
		~amr_ptr() {
			release();
		}
		void release() {
			//cout << "amr " << ptr << endl;
			SafeDelete(ptr);
		}
		amr_ptr operator+(int x)const {
			return amr_ptr(ptr + x);
		}
		void operator+=(int x) {
			ptr += x;
		}
		T& operator*()const {
			return *ptr;
		}
		T& operator[](int x)const {
			return *(ptr + x);
		}
		T* operator->()const {
			return ptr;
		}
		operator bool()const {
			return ptr;
		}

	};

	typedef BOOL(APIENTRY* PFNWGLSWAPINTERVALFARPROC)(int);
	extern PFNWGLSWAPINTERVALFARPROC wglSwapIntervalEXT;



	struct GLBuffer {
		uint id;
		GLBuffer() {
			id = 0;
		}
		GLBuffer(ll size, const void* data, uint target = GL_ARRAY_BUFFER, uint usage = GL_STATIC_DRAW) {
			init(size, data, target, usage);
		}
		void init(ll size, const void* data, uint target = GL_ARRAY_BUFFER, uint usage = GL_STATIC_DRAW) {
			glGenBuffers(1, &id);
			glBindBuffer(target, id);
			glBufferData(target, size, data, usage);
		}
		void release() {
			glDeleteBuffers(1, &id);
		}
		~GLBuffer() {
			release();
		}
		void bind(uint target) {
			glBindBuffer(target, id);
		}
		static void unbind(uint target) {
			glBindBuffer(target, 0);
		}
	};

	template<typename T>
	class StringMapping {
	public:
		typedef std::unordered_map<std::string, T*> map_t;
		StringMapping() {}
		~StringMapping() {
			clear();
		}
		auto begin()const {
			return m.begin();
		}
		auto end()const {
			return m.end();
		}
		T& operator[](const std::string& key) {
			T& ret = *get(key);
			return ret;
		}
		T*& get(const std::string& key) {
			T*& ret = m[key];
			if (!ret)ret = new T();
			return ret;
		}
		T*& find(const std::string& key) {
			T*& ret = m[key];
			return ret;
		}
		uint size()const {
			return uint(m.size());
		}
		void clear() {
			for (auto& v : m) {
				if (v.second)delete v.second;
			}
			m.clear();
		}
	private:
		map_t m;
	};

	template<typename T, typename ID_t = uint>
	class BufferGenerator {
	public:
		BufferGenerator() {}
		void gen(ID_t& id) {
			if (q.size()) {
				id = q.front(); q.pop();
				buffers[q - 1] = T();
			}
			else {
				id = ID_t(buffers.size() + 1);
				buffers.push_back(T());
			}
		}
		void del(ID_t& id) {
			q.push(id);
			id = 0;
		}
		T& buffer(ID_t id)const {
			return buffers[id - 1];
		}
		uint numBuffers()const {
			return buffers.size() - q.size();
		}
	private:
		std::list<T> buffers;
		std::queue<ID_t> q;
	};

	struct Version2 {
		int major = 0, minor = 0;
		Version2() {}
		Version2(double version) {
			init(version);
		}
		Version2(const std::string& version) {
			init(stringTo<double>(version));
		}
		void init(double version) {
			major = int(version);
			minor = int(version * 10.0) % 10;
		}
		double toDouble()const {
			return double(major) + double(minor) / 10.0;
		}
		std::string toStr()const {
			return toString(toDouble());
		}
		std::string glFormat(const std::string& post="")const {
			if (post.size())return "#version " + toString(major * 100 + minor * 10) + " "+post;
			else return "#version " + toString(major * 100 + minor * 10);
		}
		friend std::ostream& operator<< (std::ostream& out, const Version2& version) {
			out << version.toStr();
			return out;
		}
		bool operator==(const Version2& x)const {
			return x.major == major && x.minor == minor;
		}
		bool operator!=(const Version2& x)const {
			return x.major != major || x.minor != minor;
		}
	};
	template<typename T>
	struct VirtualValue {
		typedef std::function<void(const T&)> Setf;
		typedef std::function<T()> Getf;
		Setf set; Getf get;
		VirtualValue(){}
		VirtualValue(Setf setf,Getf getf):set(setf),get(getf) {
			
		}
		void operator=(const T& value) {
			set(value);
		}
		operator T()const {
			return get();
		}
	};

	class BoolList {
	public:
		struct iterator {
			BoolList* obj = 0;
			uint i = 0;
			iterator() {}
			iterator(BoolList* obj,uint i) :obj(obj),i(i) {

			}
			iterator operator++() {
				i++;
				return *this;
			}
			VirtualValue<bool> operator*() {
				return (*obj)[i];
			}
			bool operator!=(const iterator& it)const {
				return i < obj->size();
			}
		};
		BoolList() {}
		BoolList(uint s) {
			resize(s);
		}
		void set(uint i, bool t) {
			if (t)data[i / 8] |= (1 << (i % 8));
			else data[i / 8] = ~((~data[i / 8]) | (1 << (i % 8)));
		}
		bool get(uint i)const {
			return data[i / 8] & (1 << (i % 8));
		}
		VirtualValue<bool> operator[](uint i) {
			return VirtualValue<bool>(
				[=](bool t) {set(i, t); },
				[=]() {return get(i); }
			);
		}
		VirtualValue<bool> back() {
			return (*this)[msize - 1];
		}
		iterator begin() {
			return iterator(this, 0);
		}
		iterator end()const {
			return iterator();
		}
		void expand(uint s) {
			resize(size()+s);
		}
		void resize(uint s) {
			msize = s;
			if (s > capacity())
				data.resize(data.size() + uint(ceil(double(s) / 8.0)));
		}
		uint capacity()const {
			return uint(data.size()) * 8u;
		}
		uint size()const {
			return msize;
		}
		void append(bool x) {
			expand(1);
			back() = x;
		}
		BoolList operator+(const BoolList& x)const {
			BoolList ret(msize+x.size());
			uint j = 0;
			for (uint i = 0; i < msize; i++)ret[j++] = get(i);
			for (uint i = 0; i < x.size(); i++)ret[j++] = x.get(i);
			return ret;
		}
		void operator+=(const BoolList& x) {
			uint j = msize;
			expand(x.size());
			for (uint i = 0; i < x.size(); i++)set(j + i, x.get(i));
		}
	private:
		std::vector<uchar> data;
		uint msize = 0;
	};
	template<typename T>
	//typedef int T;
	class OrderedDict {
	public:
		typedef std::map<std::string, uint> map_t;
		struct iterator {
			OrderedDict* obj=0;
			uint i = 0;
			iterator() {}
			iterator(const OrderedDict* _obj,uint _i) :obj((OrderedDict*)((void*)_obj)) {
				i = _i;
			}
			bool operator!=(const iterator& it)const {
				return i!=it.i;
			}
			std::pair<const std::string&, T&> operator*()const {
				if (obj->c[i].str)return { *obj->c[i].str,obj->c[i].data };
				else return  {"",obj->c[i].data };
			}
			iterator operator++() {
				i++;
				return *this;
			}
		};
		OrderedDict() {}
		bool find(const std::string& key)const {
			return m.find(key) != m.end();
		}
		T& insert(const std::string& key, const T& value) {
			if (find(key)) {
				return (*this)[key]=value;
			}
			m[key] = uint(c.size());
			c.push_back({ &(m.find(key)->first),value });
			return c.back().data;
		}
		void erase(const std::string& key) {
			map_t::iterator it = m.find(key);
			if (it==m.end())return;
			uint pos = it->second;
			for (uint i = pos + 1; i < size();i++)
				m[*c[i].str]--;
			c.erase(c.begin()+pos);
			m.erase(key);
		}
		void erase(const iterator& it) {
			erase((*it).first);
		}
		void rename(const std::string& oldName, const std::string& newName) {
			insert(newName, (*this)[oldName]);
			erase(oldName);
		}
		T& operator[](const std::string& key) {
			if (!find(key))insert(key,T());
			return c[m[key]].data;
		}
		T operator[](const std::string& key)const {
			if (!find(key)) { LOGGER.error("Key Error: " + key); return T(); }
			return c[((OrderedDict*)((void*)this))->m[key]].data;
		}
		T& operator[](uint i) {
			return c[i].data;
		}
		T operator[](uint i)const {
			return c[i].data;
		}
		const std::string& key(uint i)const {
			return *c[i].str;
		}
		iterator begin()const {
			return iterator(this,0);
		}
		iterator end()const {
			return iterator(this, uint(c.size()));
		}
		uint size()const {
			return uint(c.size());
		}
		void clear() {
			c.clear();
			m.clear();
		}
		OrderedDict copy()const {
			return *this;
		}
		OrderedDict operator+(const OrderedDict& x)const {
			OrderedDict res = copy();
			for (auto i : x)res[i.first] = i.second;
		}
		OrderedDict operator+=(const OrderedDict& x) {
			for (auto i : x)insert(i.first, i.second);
		}
	//private:
		struct Node {
			const std::string* str;
			T data;
		};
		std::vector<Node> c;
		map_t m;
	};
#define CHECK_STACK if(!st.size()){formatError("empty stack");return;}
	class JsonData {
	public:
		typedef std::map<std::string, uint> map_t;
		struct iterator {
			JsonData* obj = 0;
			//map_t::iterator i;
			uint i;
			iterator() {}
			iterator(const JsonData* _obj,uint _i) :obj((JsonData*)((void*)_obj)) {
				i = _i;
			}

			bool operator!=(const iterator& it)const {
				return i != it.i;
			}
			std::pair<const std::string&, JsonData&> operator*()const {
				return { obj->c[i].str,*obj->c[i].x};
			}
			iterator operator++() {
				i++;
				return *this;
			}
		};
		struct Node {
			amr_ptr<JsonData> x;
			std::string str;
		};
		std::string data;
		//std::vector<JsonData> c;
		std::vector<Node> c;
		map_t m;
		char type = 0;
		JsonData() {}
		JsonData(const char* str) {
			data = std::string(str); type = '\'';
		}
		JsonData(const std::string& str) {
			data = str; type = '\'';
		}
		JsonData(char x) { init(x); type = '\''; }
		JsonData(int x) { init(x); }
		JsonData(uint x) { init(x); }
		JsonData(ll x) { init(x); }
		JsonData(ull x) { init(x); }
		JsonData(float x) { init(x); }
		JsonData(double x) { init(x); }
		static JsonData Dict() {
			JsonData res;
			res.type = '{';
			return res;
		}
		static JsonData List() {
			JsonData res;
			res.type = '[';
			return res;
		}
		template<typename T>
		void init(const T& x) {
			data = toString(x);
		}
		template<typename T>
		VirtualValue<T> to() {
			return VirtualValue<T>(
				[this](const T& value) {data = toString(value); },
				[this] {return stringTo<T>(data); });
		}
		operator std::string()const {
			return data;
		}
		std::string& toStr() {
			return data;
		}
		iterator begin()const {
			return iterator(this,0);
		}
		iterator end()const {
			return iterator(this,size());
		}
		JsonData& operator[](const std::string& key) {
			if (!isDict())LOGGER.error("Only a dict can be indexed by string\nGot "+serialize()+" ["+key+"]");
			else if (!find(key))LOGGER.error("KeyError: " + key);
			return *c[m[key]].x;
		}
		JsonData operator[](const std::string& key)const {
			if (!isDict())LOGGER.error("Only a dict can be indexed by string\nGot " + serialize() + " [" + key + "]");
			else if (!find(key))LOGGER.error("KeyError: " + key);
			return *c[((JsonData*)(void*)this)->m[key]].x;
		}
		JsonData& operator[](uint index) {
			if (!isList())LOGGER.error("Only a list can be indexed by int\nGot " + serialize() + " [" + toString(index) + "]");
			else if (index >= c.size())LOGGER.error("IndexError: " + toString(index));
			return *c[index].x;
		}
		JsonData operator[](uint index)const {
			if (!isList())LOGGER.error("Only a list can be indexed by int\nGot " + serialize() + " [" + toString(index) + "]");
			else if (index >= c.size())LOGGER.error("IndexError: " + toString(index));
			return *c[index].x;
		}
		void append(const JsonData& x) {
			c.emplace_back(Node{ x,""});
		}
		JsonData& insert(const std::string& key, const JsonData& value) {
			if (find(key))
				return (*this)[key] = value;
			m[key] = uint(c.size());
			c.emplace_back(Node{ value,(m.find(key)->first) });
			return *c.back().x;
		}
		void erase(const std::string& key) {
			map_t::iterator it = m.find(key);
			if (it == m.end())return;
			uint pos = it->second;
			it++;
			for (; it != m.end(); it++) {
				it->second--;
			}
			c.erase(c.begin() + pos);
			m.erase(key);
		}
		void erase(const iterator& it) {
			erase((*it).first);
		}
		void rename(const std::string& oldName, const std::string& newName) {
			insert(newName, (*this)[oldName]);
			erase(oldName);
		}
		uint size()const {
			return uint(c.size());
		}
		std::string serialize()const {
			if (isString())return "\"" + data + "\"";
			else if (isValue())return data;
			if (isList()) {
				std::string ret = "[";
				for (uint i = 0; i < c.size(); i++) {
					ret += c[i].x->serialize() + ",";
				}
				if (size())ret.back() = ']';
				else ret += "]";
				return ret;
			}

			std::string ret = "{";
			for (auto& i : m) {
				ret += "\"" + i.first + "\":" + c[i.second].x->serialize() + ",";
			}
			if (size())ret.back() = '}';
			else ret += "}";
			return ret;
		}
		friend std::ostream& operator<< (std::ostream& out, const JsonData& data) {
			out << data.serialize();
			return out;
		}
		bool isDict()const {
			return type == '{';
		}
		bool isList()const {
			return type == '[';
		}
		bool isValue()const {
			return type != '{' && type != '[';
		}
		bool isString()const {
			return type == '\'' || type == '\"';
		}

		void deserialize(const std::string& str) {
			JsonLoader loader(str);
			*this = *loader.st.top();
		}
		void load(const std::string& file) {
			reset();
			std::string str;
			readString(file, str);
			deserialize(str);
		}
		void reset() {
			data.clear();
			m.clear();
			c.clear();
			type = 0;
		}
		void clear() {
			m.clear();
			c.clear();
		}
		bool find(const std::string& key)const {
			return m.find(key) != m.end();
		}
		JsonData copy()const {
			return *this;
		}
		JsonData operator+(const JsonData& x)const {
			JsonData ret = copy();
			for (auto i : x)ret.insert(i.first, i.second);
			return ret;
		}
		void operator+=(const JsonData& x) {
			for (auto i : x)insert(i.first, i.second);
		}
		
	private:
		struct JsonLoader {
			std::stack<JsonData*> st;
			std::stack<std::string> keys;
			bool topDone = true;
			bool error = false;
			const std::string* strp = 0;
			std::string data;
			JsonLoader() {}
			JsonLoader(const std::string& str) :strp(&str) {
				for (char s : str) {
					if (s == '\"' || s == '\'') {
						CHECK_STACK;
						if (st.top()->type == s) {
							st.top()->data = data;
							data.clear();
							topDone = true;
							continue;
						}
						else if (!st.top()->isString()) {
							st.push(new JsonData()); st.top()->type = s;
							topDone = false;
							continue;
						}
					}
					if (st.size() && st.top()->isString() && !topDone) {
						data.push_back(s);
						continue;
					}
					else if (s == '\n' || s == ' ' || s == '\t')continue;
					else if (s == '{' || s == '[') {
						st.push(new JsonData()); st.top()->type = s;
						topDone = false;
					}
					else if (s == ':') {
						CHECK_STACK;
						if (!st.top()->isString()) { formatError("key must be str"); return; }
						JsonData* tmp = st.top(); st.pop();
						keys.push(tmp->data);
						if (!st.top()->isDict()) { formatError("key:value must be in a dict"); return; }
					}
					else if (s == ',') {
						if (end())return;
					}
					else if (s == '}' || s == ']') {
						if (end())return;
						if (!pairOp(st.top()->type, s)) {
							formatError(std::string("unmatched op: ") + st.top()->type + " and " + s);
							return;
						}
						topDone = true;
					}
					else data.push_back(s);
				}
			}
			bool end() {
				JsonData* tmp;
				if (data.size()) {
					tmp = new JsonData();
					tmp->data = data;
					data.clear();
				}
				else {
					if (!topDone)return false;
					tmp = st.top(); st.pop();
				}
				if (!st.size()) {
					formatError("empty stack");
					return true;
				}
				JsonData& top = *st.top();
				top.c.push_back({ *tmp,"" });

				SafeDelete(tmp);
				if (top.isList()) {
					//cout << "push " << st.top()->c.back() << " | " << *st.top() << endl;
					return false;
				}
				else if (!keys.size()) {
					formatError("missing key in dict");
					return true;
				}
				top.m[keys.top()] = uint(st.top()->c.size()) - 1;
				top.c.back().str = (top.m.find(keys.top())->first);
				//cout << "push " << keys.top() << ":" << st.top()->c.back() << " | "<<*st.top()<<endl;
				keys.pop();
				return false;
			}
			void formatError(const std::string& info) {
				LOGGER.error("JsonData::FormatError: " + info);
				error = true;
			}
			static bool pairOp(char a, char b) {
				switch (a) {
				case '{':
					return b == '}';
				case '[':
					return b == ']';
				default:
					return false;
				}
			}
		};
	};

#define FESTA_LEFT_TOP 4
#define FESTA_ORIGIN 1
#define FESTA_CENTER 2
#define FESTA_LEFT_BOTTOM 3

	struct WindowPosition {
		vec2 pos = vec2(0.0f);
		int type = -1;
		WindowPosition() {}
		WindowPosition(const vec2& pos, int type = FESTA_LEFT_TOP)
			:pos(pos), type(type) {}
		WindowPosition tolt()const {
			switch (type) {
			case FESTA_LEFT_TOP:
				return *this;
			case FESTA_CENTER:
				return WindowPosition();
			case FESTA_LEFT_BOTTOM:
				return WindowPosition();
			}
		}
		WindowPosition to(int type_)const {
			return WindowPosition();
		}
	};

	template<typename T>
	struct UpdatingValue {
		T prev, now;
		UpdatingValue() {}
		UpdatingValue(const T& x) {
			now = x;
		}
		UpdatingValue(const T& _prev, const T& _now)
			:prev(_prev), now(_now) {}
		void operator=(const UpdatingValue<T>& x) {
			prev = x.prev; now = x.now;
		}
		template<typename t>
		void operator=(const t& x) = delete;
		void update(const T& x) {
			prev = now;
			now = x;
		}
	};
}
