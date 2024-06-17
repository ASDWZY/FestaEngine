#pragma once

#define COMPILER_DEBUG

#include "Expression.h"
#include "command.h"

#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

using namespace Festa;

inline string getBaseType(const string& type) {
	if (!type.size())return string();
	if (type.back() == '&')return "&";
	else if (type.back() == '*')return "*";
	else return type;
}

static void initType_f(Command* cmd, Command::GlobalMemory* mem) {
	DataList p = cmd->getInput(0, mem);
	//if(p.size()==8)cout << "initt: " << p.size() << "," << mem->pool.size() << "," << p.get<addr_t>(0) << endl;
	cmd->value.pushReference({ p.size(),mem->pool.size()});
	mem->pool+=p;
}
static void copyDataList(Command* cmd, Command::GlobalMemory* mem) {
	DataList::Reference ptr1 = cmd->getInput(0, mem).getReference(0);
	cmd->value = cmd->getInput(1, mem);
	for (addr_t i = 0; i < ptr1.len; i++)
		mem->pool.bytes[ptr1.addr + i] = cmd->value.bytes[i];
}

class Compiler {
public:
	struct Variable {
		string type;
		addr_t addr=0;
		int space = -1;
		Variable() {}
		Variable(const string& type,addr_t addr,int space=-1)
		:type(type),addr(addr),space(space){

		}
	};
	typedef unordered_map<string, Variable> VarMap;
	struct DataType {
		addr_t size = 0;
		unordered_map<string, DataType*> children;
		DataType() {}
		DataType(uint size) :size(size) {}
		DataType(unordered_map<string, DataType*>& children) :children(children) {
			for (auto i : children)size += i.second->size;
		}
		DataType(VarMap& m, StringMapping<DataType>& types) {
			for (auto i : m) {
				DataType* ch = &types[m[i.first].type];
				children[i.first] = ch;
				//cout << "var: " << i.first << "," << ch->size << endl;
				size += ch->size;
			}
			//cout << "new type: " << size << endl;
		}
	};
	struct VAR {
		char vtype=-1;
		string type="void";
		VAR() {}
		VAR(const string& type) :type(type), vtype(VAR_VALUE) {}
		VAR(const string& type, char vtype) :type(type), vtype(vtype) {}
		string serialize()const {
			if (vtype == VAR_REF)return type + "&";
			else return type;
		}
	};
	struct Function {
		Command::PerformFunction func=0;
		VAR out;
		vector<VAR> invar;
		Command::FunctionDefination** def=0;
		addr_t off = 0;
		Function() {}
		Function(Command::PerformFunction func) :func(func) {}
		Function(Command::PerformFunction func,const VAR& out,const vector<VAR>& invar,addr_t off=0) 
			:func(func),out(out), invar(invar),off(off) {}
		string serialize(const string& name)const {
			string ret = out.type+" "+name; ret.push_back('(');
			for (uint i = 0; i < invar.size(); i++) {
				ret += invar[i].serialize();
				if (i != invar.size() - 1)ret.push_back(',');
			}
			ret.push_back(')');
			return ret;
		}
		string pattern()const {
			string ret;
			for (uint i = 0; i < invar.size(); i++) {
				ret.push_back('_');
				ret += invar[i].serialize();
			}
			return ret;
		}
	};
	struct LocalMemory {
		addr_t address = 1;
		int space = -1;
		VarMap variables;
		Variable* find(const string& name) {
			if (variables.find(name) == variables.end())return 0;
			else return &variables[name];
		}
	};
	typedef Command::GlobalMemory GlobalMemory;
	GlobalMemory* memory = 0;

	vector<VarMap> spaceVariables;
	unordered_map<string, vector<Function>> functions;
	StringMapping<DataType> types;
	unordered_map<string, int> typeSpaces;

	Compiler() {}
	Compiler(GlobalMemory* mem):memory(mem) {
		init();
	}
	~Compiler() {
		
	}
	void addType(const string& name,const DataType& type) {
		types[name] = type;
		functions[name].push_back(Function(initType_f, VAR(name, VAR_REF), { VAR(name) }, type.size));
		functions["="].push_back(Function(copyDataList, VAR(name), { VAR(name,VAR_REF),VAR(name) }));
	}
	void compile(Expression& expr, LocalMemory* local);
	Command* compile(const string& code, LocalMemory*& local, bool initExpression, const CodeErrorLog& logger);
	void addFunction(const string& name, const Function& f) {
		functions[name].push_back(f);
	}
	addr_t getTypeLength(const string& name) {
		DataType* type = types.find(name);
		if (!type) {
			LOGGER.error("Unknown type_types: " + name);
			return 0;
		}
		else return type->size;
	}
private:
	Command* command=0;
	Command* bgc = 0;
	Function* bgf=0;
	VAR rvar;

	void init();
	VarMap* typeSpace(const string& type,LocalMemory* local) {
		if (!type.size())return &spaceVariables[local->space];
		if (typeSpaces.find(type) == typeSpaces.end()) {
			LOGGER.error("Unknown type_typeSpace: " + type);
			return 0;
		}
		return &spaceVariables[typeSpaces[type]];
	}
	void addVariable(const string& t, const string& tname, const string& vname, LocalMemory*& local);
	void addVariable(const string& type, Expression& expr,  LocalMemory*& local);
	template<typename T>
	void pushValue(const string& type, const T& val) {
		DataList list;
		list.push(val);
		command=new Command(list);
		rvar.type = type;
	}
	void pushVariable(const Variable& v,bool attrib);
	
	string funcLog(const string& name,const vector<VAR> &vars) {
		string ret = name; ret.push_back('(');
		for (uint i = 0; i < vars.size(); i++) {
			ret += vars[i].type;
			if (i != vars.size() - 1)ret.push_back(',');
		}
		ret.push_back(')');
		return ret;
	}
	void translateType(const VAR& v, LocalMemory*& local);
	Function* matchTrans(const string& name, const vector<VAR>& vars, Command** cmds, Command** trans, addr_t& off);
	Function* matchFunction(const string& name, const vector<VAR>& vars, Command** cmds, Command** trans, addr_t& off);
	void useFunction(const string& name, vector<Command*>& commands, vector<VAR>& vars,LocalMemory*& local);
	addr_t newVar(const string& vname,const string& base,const string& tname,LocalMemory* local,bool add) {
		addr_t len = getTypeLength(base);
		if (local->variables.find(vname) != local->variables.end())
			LOGGER.error("Redefined variable: " + vname);
		Variable v(tname, local->address, local->space);
		local->variables[vname] = v;
		spaceVariables[local->space][vname] = v;
		//cout << "new var: " << tname << " " << vname << "," << local->address <<" | "<<local->space << endl;
		if(add)local->address += len;
		return len;
	}
	void initVariable(const string& base,const string& type,Expression& init,LocalMemory*& local,const string& vname="") {
		compile(init, local);
		vector<Command*> cmd = {command};
		vector<VAR> vars = { rvar };
		useFunction(type, cmd, vars, local);
		if (vname.size()) {
			local->address -= getTypeLength(base);
			newVar(vname, base, type, local, true);
		}
	}
	bool compileElse(Expression& expr, LocalMemory*& local);
	Command* newSpace(LocalMemory*& local,bool cmd);
	void compileFunction(Expression& expr, LocalMemory*& local);
	void compilePtrAttrib(Expression& expr, LocalMemory*& local);
	void compileRefAttrib(Expression& expr, LocalMemory*& local);
};


class Precompiler {
public:
	string ret;
	CodeErrorLog logger;
	Precompiler() {}
	Precompiler(const string& code) {
		logger = CodeErrorLog();
		logger.init(code);
		compile(code);
	}
	void compile(const string& code) {
		uint first = 0, line = 0;; int state = 0;// 0: non;1: //;2: /*
		vector<string> pattern;
		for (uint i = 0; i < code.size(); i++) {
			if (state == 2) {
				if (i != code.size() - 1&&code[i] == '*' && code[i + 1] == '/')state = 0, i++;
				continue;
			}
			if (i != code.size() - 1) {
				if (code[i] == '/' && code[i + 1] == '/')state = 1, i++;
				else if (code[i] == '/' && code[i + 1] == '*')state = 2, i++;
			}
			if (code[i] == '\n') {
				if(pattern.size())compile(pattern);
				pattern.clear(); 
				if(state==1)state=0;
				first = i + 1; 
				line++;
				ret.push_back('\n');
				continue;
			}
			if (state != 0)continue;
			else if ((code[i] == '\t' || code[i] == ' ')&& pattern.size()) {
				pattern.push_back(string());
				first++; continue;
			}
			else if (code[i] == '#') {
				if (i != first) {
					logger.error(i,1,"# isnot the first letter of the line");
					continue;
				}
				pattern.push_back(string());
				continue;
			}
			if(state==0) {
				if (pattern.size())pattern.back().push_back(code[i]);
				else ret.push_back(code[i]);
			}
		}
		if (pattern.size())compile(pattern);
		if (state == 2)LOGGER.error("missing */");
		while (ret.back() == '\0')ret.pop_back();
	}
private:
	unordered_map<string, bool> include;
	void compile(const vector<string>& pattern) {
		if (findSubstr(pattern[0],"include")!=-1) {
			uint stride = uint(string("include").size());
			string file;
			if (pattern.size() > stride) {
				//cout << pattern[0] << endl;
				assert(pattern.size()==1);
				file = stringIndex(pattern[0],stride,pattern[0].size());
			}
			else {
				assert(pattern.size() == 2);
				file = pattern[1];
			}
			if (file[0] != '"' || file.back() != '"') {
				LOGGER.error("include file missing ''");
				return;
			}
			file = stringIndex(file, 1, file.size() - 1);
			if (include[file]) {
				cout << "reinclude: " << file << endl;
				return;
			}
			cout << "include: " << file << endl;
			string tmp; readString(file, tmp);
			include[file] = true;
			compile(tmp);
		}
	}
	static ll findSubstr(const string& str, const string& sub) {
		ll i = 0;
		while (i < str.size()) {
			ll j = 0;
			while (j < sub.size() && str[i + j] == sub[j])j++;
			if (j == sub.size())return i;
			i += j+1;
		}
		return -1;
	}
};

template<typename T1, typename T2>
static void init_f(Command* cmd, Compiler::GlobalMemory* mem) {
	T1 val = cmd->input<T1>(0, mem);
	cmd->value.pushReference({ sizeof(T2),mem->pool.size() });
	//cout << "init: " << val << "->" << T2(val) <<","<<mem->pool.size()<< endl;
	mem->pool.push(T2(val));
}

struct EmbeddedVar {
	string type,name;
	EmbeddedVar() {}
	EmbeddedVar(const string& type, const string& name)
		:type(type), name(name) {}
	addr_t getLength(Compiler* compiler)const{
		return compiler->getTypeLength(type);
	}
};

struct EmbeddedFunction {
	string name;
	Command::PerformFunction f=0;
	vector<Compiler::VAR> in;
	Compiler::VAR out;
	addr_t off=0;
	EmbeddedFunction() {}
	EmbeddedFunction(const string& name,Command::PerformFunction f, const vector<Compiler::VAR>& in, const Compiler::VAR& out, addr_t off = 0)
	:name(name),f(f),in(in),out(out),off(off) {

	}
	void initFunction(const string& type,Command::PerformFunction f_, const vector<Compiler::VAR>& in_) {
		name = type;
		f = f_;
		in = in_;
		out = Compiler::VAR(type,VAR_REF);
		off = sizeof(addr_t);
	}
	string serialize()const {
		string ret=out.serialize()+" "+name+"(";
		for (Compiler::VAR var : in)ret += var.serialize();
		ret.push_back(')');
		return ret;
	}
};

struct EmbeddedClass {
	string name;
	vector<EmbeddedVar> members;
	EmbeddedClass() {}
	EmbeddedClass(const string& name, const vector<EmbeddedVar>& members)
		:name(name), members(members) {}
};

class EmbeddedProgram {
public:
	typedef Command::PerformFunction Function;
	typedef Compiler::VAR VAR;
	Compiler* compiler = 0;
	EmbeddedProgram() {
		Expression::init();
		compiler=new Compiler(&memory);
		local = new Compiler::LocalMemory();
	}
	EmbeddedProgram(const string& code) {
		Expression::init();
		compiler = new Compiler(&memory);
		local = new Compiler::LocalMemory();
		compile(code);
	}
	~EmbeddedProgram() {
		if (cmd) {
			cmd->release();
		}
		if (compiler)delete compiler;
	}
	void compile(const string& code,bool clearCompiler=true) {
		clear();
		Precompiler precompiler(code);
		//cout << "precompiler: " << precompiler.ret << endl;
		cmd = compiler->compile(precompiler.ret, local, false,precompiler.logger);
		if (clearCompiler) {
			delete compiler; delete local;
			compiler = 0; local = 0;
		}
		Expression::init();
	}
	void importVariable(const EmbeddedVar& var) {
		
	}
	void importClass(const EmbeddedClass& c) {
		Expression::types[c.name] = true;
		compiler->addType(c.name, Compiler::DataType(sizeof(addr_t)));
		
		int space = memory.spaceHead.size();
		local->space = space;
		memory.spaceHead.push_back(stack<addr_t>());
		compiler->spaceVariables.push_back(Compiler::VarMap());
		compiler->typeSpaces[c.name] = space;
		Compiler::VarMap& vars = compiler->spaceVariables.back();

		addr_t off = 0;
		for (uint i = 0; i < c.members.size(); i++) {
			vars[c.members[i].name] = Compiler::Variable(c.members[i].type,off,space);
			off+=c.members[i].getLength(compiler);
		}

	}
	void importFunction(const EmbeddedFunction& function) {
		compiler->addFunction(function.name,Compiler::Function(function.f,function.out,function.in,function.off));
	}
	void run(bool clearMemory=true) {
		if(clearMemory)clear();
		cmd->run(&memory);
	}
	void inThread(bool clearMemory = true) {
		if (clearMemory)clear();
		threadState = 0;
		thread([this] {
			while (threadState!=0)Sleep(0);
			threadState = 1;
			cmd->run(&memory); 
			threadState = 2;
			}).detach();
	}
	int exitCode()const {
		return memory.exitCode;
	}
	void clear() {
		memory.clear();
		memory.pool.push(uchar(0));
	}
	template<typename T>
	void operator<<(const T& val) {
		/*if (threadState == 0) {
			cout << "wait for starting\n";
			unique_lock<std::mutex> locker(mu);
			con.wait(locker, [this] {return threadState == 1; });
			cout << "start";
		}
		else if (threadState == 2)
			LOGGER.error("The embeddedProgram has been already finished.");
			*/
		memory.in.push(val);
		cout << "push " << memory.in.size() << endl;
		DataList p;
		for (uint i = 0; i < sizeof(T); i++) {
			//p.bytes.push_back(memory.in.pop());
		}
		//cout << p.get<float>(0) << "," << p.get<float>(4) << "," << p.get<float>(8) << endl;
	}

	template<typename T>
	void operator>>(T& x) {
		//while (memory.out.size() < sizeof(T)) Sleep(0); 
		unique_lock<std::mutex> locker(mu);
		con.wait(locker, [this] {return memory.out.size() >= sizeof(T); });
		x = memory.out.get<T>(0);
	}
private:
	Compiler::GlobalMemory memory;
	CodeErrorLog* logger=0;
	Compiler::LocalMemory* local = 0;
	Command* cmd=0;

	std::mutex mu;
	atomic<int> threadState = 0;
	condition_variable con;
};
