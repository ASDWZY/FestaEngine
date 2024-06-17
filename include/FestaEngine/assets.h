#pragma once
#include "../../Festa.hpp"
#include "Expression.h"



namespace Festa {

	static std::string func2str(const std::string& name, const std::vector<std::string>& params) {
		string ret = name; ret.push_back('(');
		if (params.size()) {
			for (uint i = 0; i < params.size() - 1; i++) ret += params[i] + ",";
			ret += params.back();
		}
		ret.push_back(')');
		return ret;
	}

	static std::string vec3_str(const vec3& x) {
		return func2str("vec3", { toString(x.x),toString(x.y),toString(x.z) });
	}

	static string quat_str(const quat& x) {
		return func2str("quat", { toString(x.w),toString(x.x),toString(x.y),toString(x.z) });
	}

	static string Transform_str(const Transform& x) {
		return func2str("Transform", { vec3_str(x.pos.pos),quat_str(x.rot.rot),vec3_str(x.scaling.scaling) });
	}

	static string PhongColor_str(const PhongColor& x) {
		return func2str("PhongColor", { vec3_str(x.ambient),vec3_str(x.diffuse),vec3_str(x.specular) });
	}

	static string Light_str(const Light& x) {
		return func2str("Light", { toString(x.type),PhongColor_str(x),
			vec3_str(x.pos),vec3_str(x.dir),toString(x.p1),toString(x.p2),toString(x.p3) });
	}


	struct AssetListNode {
		string name, expr;
		AssetListNode* next = 0;
		AssetListNode() {}
		AssetListNode(const string& name, const string& expr)
			:name(name), expr(expr) {

		}
		void model(const string& name_, const string& path, const Transform& trans) {
			name = name_;
			expr = func2str("Model", { path,Transform_str(trans) });
		}
		void shader(const string& name_, const string& path) {
			name = name_;
			expr = func2str("Shader", { path });
		}
		void program(const string& name_, const vector<string>& shaders) {
			name = name_;
			expr = func2str("Program", shaders);
		}
		void light(const string& name_, const Light& light) {
			name = name_;
			expr = Light_str(light);
		}
		string code()const {
			return name + "=" + expr;
		}
	};

	class AssetList {
	public:
		AssetList() {}
		void push(const AssetListNode& node) {
			size_++;
			if (!head) {
				head = new AssetListNode(node);
				head->next = 0;
				tail = head;
				return;
			}
			tail->next = new AssetListNode(node);
			tail = tail->next;
			tail->next = 0;
		}
		AssetListNode*& back() {
			return tail;
		}
		uint size()const {
			return size_;
		}
		void save(const string& file) {
			ofstream f(file, ios::out);
			AssetListNode* node = head;
			while (node) {
				f << node->code() << ";\n";
				node = node->next;
			}
		}
		void del(AssetListNode* n) {
			cout << "delete: " << n << endl;
			if (!n)return;
			AssetListNode* node = head;
			size_--;
			int num = 0;
			while (node) {
				if (node->next == n) {
					cout << "del: " << num + 1 << endl;
					node->next = n->next;
				}
				node = node->next;
				num++;
			}
		}
	private:
		AssetListNode* head = 0;
		AssetListNode* tail = 0;
		uint size_ = 0;
	};
	//typedef int T;
	template<typename T>
	class AssetsMap {
	public:
		struct Dt { T* ptr = 0; AssetListNode* node = 0; };
		typedef unordered_map<string, Dt*> map_t;
		AssetsMap() {}
		~AssetsMap() {
			//clear();
		}
		auto begin()const {
			return m.begin();
		}
		auto end()const {
			return m.end();
		}
		T& operator[](const string& key) {
			return *(get(key)->ptr);
		}
		Dt*& get(const string& key) {
			Dt*& ret = m[key];
			if (!ret)ret = new Dt();
			return ret;
		}
		Dt*& find(const string& key) {
			return m[key];
		}
		uint size()const {
			return uint(m.size());
		}
		void clear() {
			for (auto& v : m) {
				if (v.second->ptr) {
					delete v.second->ptr;
					v.second->ptr = 0;
				}
				if (v.second->node) {
					delete v.second->node;
					v.second->node = 0;
				}
			}
			m.clear();
		}
		void erase(const string& name) {
			m.erase(name);
		}
		void rename(const string& before, const string& after) {
			Dt* d = m[before]; erase(before);
			d->node->name = after;
			m[after] = d;
		}
	private:
		map_t m;
	};

	class Assets {
	public:
		struct Value {
			void* ptr;
			string type;
			template<typename T>
			T to() {
				return *(T*)ptr;
			}
		};
		AssetsMap<Model> models;
		AssetsMap<Shader> shaders;
		AssetsMap<Program> programs;
		AssetsMap<Light> lights;
		AssetList list;
		Assets() {}
		Assets(const string& file) {
			load(file);
		}
		void clear() {
			*this = Assets();
		}
		void load(const string& file) {
			clear();
			vector<string> lines;
			readLines(file, lines);
			for (string& line : lines) {
				Expression expr(line);
				list.push(AssetListNode(expr[0][0].name, expr[0][1].serialize()));
				compile(expr[0][0].name, expr[0][1]);
			}
		}
		void add(const AssetListNode& node) {
			list.push(node);
			compile(node.name, Expression(node.expr));
		}
		void save(const string& file) {
			ofstream f(file, ios::out);
			save(f, models);
			save(f, shaders);
			save(f, programs);
			save(f, lights);
			f.close();
		}
		template<typename T>
		void save(ofstream& f, AssetsMap<T>& m) {
			for (auto& i : m) {
				f << i.second->node->code() << ";\n";
			}
		}
		void compile(const string& name, const Expression& expr) {
			//cout << "compile: " << name << expr << endl;
			Value val = compile(expr);
			if (val.type == "Model")setMap(name, val, models);
			else if (val.type == "Shader")setMap(name, val, shaders);
			else if (val.type == "Program")setMap(name, val, programs);
			else if (val.type == "Light")setMap(name, val, lights);
		}
		void renderModels()const {
			for (auto& i : models)i.second->ptr->draw();
		}
		void bindLights()const {
			string LightNames[3] = { "dirLights","pointLights","spotLights" };
			uint num[3] = { 0,0,0 };
			for (uint i = 0; i < lights.size(); i++) {
				for (uint j = 0; j < 3; j++) {
					int type = j + 1;
					string name = LightNames[j] + "[" + toString(i) + "]";
					if (type == 1)DirLight().bind(name);
					else if (type == 2)PointLight().bind(name);
					else SpotLight().bind(name);
				}
			}

			for (auto& i : lights) {
				int type = i.second->ptr->type;
				i.second->ptr->bind(LightNames[type - 1] + "[" + toString(num[type - 1]++) + "]");
			}
		}
	private:
		//typedef int T;
		template<typename T>
		void setMap(const string& name, const Value& value, AssetsMap<T>& m) {
			m.get(name)->ptr = (T*)value.ptr;
			m.get(name)->node = list.back();
		}
		Value compile(const Expression& expr) {
			if (expr.isVariable()) {
				if (isNumber(expr.name))return Value({ new float(stof(expr.name)),"float" });
				else return Value({ new string(expr.name),"string" });
			}
			vector<Value> p;
			for (uint i = 0; i < expr.size(); i++)
				p.push_back(compile(expr.params[i]));
			void* ptr = 0;
			if (expr.name == "vec3") {
				ptr = new vec3(p[0].to<float>(), p[1].to<float>(), p[2].to<float>());
			}
			else if (expr.name == "quat") {
				ptr = new quat(p[0].to<float>(), p[1].to<float>(), p[2].to<float>(), p[3].to<float>());
			}
			else if (expr.name == "Transform") {
				ptr = new Transform(p[0].to<vec3>(), p[1].to<quat>(), p[2].to<vec3>());
			}
			else if (expr.name == "Model") {
				ptr = new Model(p[0].to<string>());
				((Model*)ptr)->setTransform(p[1].to<Transform>());
			}
			else if (expr.name == "Shader") {
				ptr = new Shader();
				((Shader*)ptr)->load(p[0].to<string>());
			}
			else if (expr.name == "Program") {
				ptr = new Program();
				vector<uint> ids;
				for (Value& v : p) {
					ids.push_back(shaders[v.to<string>()].id);
				}
				if (ids.size())((Program*)ptr)->init(ids);
			}
			else if (expr.name == "PhongColor") {
				ptr = new PhongColor(p[0].to<vec3>(), p[1].to<vec3>(), p[2].to<vec3>());
			}
			else if (expr.name == "Light") {
				Light* l = new Light();
				l->type = int(p[0].to<float>());
				l->load(p[1].to<PhongColor>());
				l->pos = p[2].to<vec3>();
				l->dir = p[3].to<vec3>();
				l->p1 = p[4].to<float>();
				l->p2 = p[5].to<float>();
				l->p3 = p[6].to<float>();
				ptr = l;
			}
			for (uint i = 0; i < expr.size(); i++)
				delete p[i].ptr;
			return Value({ ptr, expr.name });
		}
	};
}

