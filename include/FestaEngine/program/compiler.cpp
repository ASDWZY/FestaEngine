#include "compiler.h"

#define P1 4
#define P2 2

unordered_map<string, int> Expression::priorities = 
{	{"->",1},{"::",1},
	{"!",P2},{"&",P2}, {"*",P2},{"/",P2} ,{"+",P2 + 1},{"-",P2 + 1},
	{"~&",P2},{"~*",P2},
	{"=",P1+1}, {"<",P1},{">",P1},{"==",P1},{"<<",P1},
	{",",6},{")",6},{";",8},{"}",8},{"~v",7},{"~vi",P2+1},{"~f",7} };
unordered_map<string, bool> Expression::reserved = { {"struct",true},{"class",true} ,{"if",true}, {"else",true} ,{"return",true},{"const",true},{"static",true} };

//0: not
//2: 2
//1: (1 or 2) or 1
unordered_map<string, int> Expression::operators = 
{	{"+",1},{"-",1} ,{"*",1} ,{"/",2}, {"&",1},{"!",1} ,
	{"=",2}, {"<",2},{">",2},{"==",2},{"<<",2},
	{"->",2 },{".",2}, {"::",2},
};

unordered_map<string, bool> Expression::types;

ostream& operator<< (ostream& out, const Expression& expr) {
	out << expr.serialize();
	return out;
}

void Expression::init() {
	types.clear();
	types["int"] = true;
	types["void"] = true;
	types["bool"] = true;
	types["float"] = true;
	types["~&"] = true;
	types["~*"] = true;
}

static void addVariable_f(Command* cmd, Compiler::GlobalMemory* mem) {
	addr_t len = cmd->input<addr_t>(0,mem);
	mem->pool.extend(len);
}


static void commands_f(Command* cmd, Compiler::GlobalMemory* mem) {cmd->runCommands(mem); }
static void reference_f(Command* cmd, Compiler::GlobalMemory* mem) {
	DataList::Reference p = cmd->getInput(0, mem).getReference(0);
	for (addr_t i = 0; i < p.len; i++)
		cmd->value.bytes.push_back(mem->pool.bytes[p.addr+i]);
	if (p.len != 8)return;
	//cout << "ref: " << p.len << "," << p.addr <<","<<cmd->value.get<addr_t>(0) << endl;
	
}
static void address_f(Command* cmd, Compiler::GlobalMemory* mem) {
	DataList::Reference p = cmd->getInput(0, mem).getReference(0);
	cmd->value.push(p.addr);
	//cout << "address: " << p.len << "," << p.addr << endl;
}

template<typename Tr, typename T1, typename T2>
static void add_f(Command* cmd, Compiler::GlobalMemory* mem) {
	T1 v1 = cmd->input<T1>(0,mem);
	T2 v2 = cmd->input<T2>(1,mem);
	cmd->value.push(Tr(v1) + Tr(v2));
}

template<typename Tr, typename T1, typename T2>
static void sub_f(Command* cmd, Compiler::GlobalMemory* mem) {
	T1 v1 = cmd->input<T1>(0, mem);
	T2 v2 = cmd->input<T2>(1, mem);
	cmd->value.push(Tr(v1) - Tr(v2));
}

template<typename Tr, typename T1, typename T2>
static void mul_f(Command* cmd, Compiler::GlobalMemory* mem) {
	T1 v1 = cmd->input<T1>(0, mem);
	T2 v2 = cmd->input<T2>(1, mem);
	cmd->value.push(Tr(v1) * Tr(v2));
}

template<typename Tr, typename T1, typename T2>
static void div_f(Command* cmd, Compiler::GlobalMemory* mem) {
	T1 v1 = cmd->input<T1>(0, mem);
	T2 v2 = cmd->input<T2>(1, mem);
	cmd->value.push(Tr(v1) / Tr(v2));
}


template<typename T>
static void neg_f(Command* cmd, Compiler::GlobalMemory* mem) {
	T v = cmd->input<T>(0,mem);
	cmd->value.push(-v);
}

static void initReference_f(Command* cmd, Compiler::GlobalMemory* mem) {
	DataList::Reference val = cmd->getInput(0, mem).getReference(0);
	cmd->value.pushReference({ sizeof(val),mem->pool.size() });
	mem->pool.pushReference(val);
}

static void ptr2ref_f(Command* cmd, Compiler::GlobalMemory* mem) {
	addr_t addr = cmd->input<addr_t>(0,mem);
	addr_t len = cmd->input<addr_t>(1,mem);
	//cout << "ptr2ref: " << len << "," << addr << endl;
	cmd->value.pushReference({ len,addr });
}

static void ptrAttrib_f(Command* cmd, Compiler::GlobalMemory* mem) {
	addr_t addr = cmd->input<addr_t>(0,mem);
	DataList::Reference ref = cmd->getInput(1, mem).getReference(0);
	ref.addr += addr;
	cmd->value.pushReference(ref);
}

static void refAttrib_f(Command* cmd, Compiler::GlobalMemory* mem) {
	DataList::Reference fa = cmd->getInput(0, mem).getReference(0);
	DataList::Reference ch = cmd->getInput(1, mem).getReference(0);
	ch.addr += fa.addr;
	cmd->value.pushReference(ch);
}

static void attrib_f(Command* cmd, Compiler::GlobalMemory* mem) {
	DataList::Reference ref = cmd->getInput(0, mem).getReference(0);
	int space = cmd->input<int>(1,mem);
	//cout << "attrib: " << space<<" | "<<mem->spaceHead[space].top() << "," << ref.addr << endl;
	ref.addr += mem->spaceHead[space].top();
	cmd->value.pushReference(ref);
}

static void elimateReference_f(Command* cmd, Compiler::GlobalMemory* mem) {
	DataList::Reference val = cmd->getInput(0, mem).getReference(0);
	cmd->value.pushReference(mem->pool.getReference(val.addr));
}

static void enterSpace_f(Command* cmd, Compiler::GlobalMemory* mem) {
	mem->enterSpace(cmd->input<int>(0,mem));
}

static void this_f(Command* cmd, Compiler::GlobalMemory* mem) {
	//cout << "this_f: " <<mem->space.top()<<","<< mem->spaceHead[mem->space.top()].top() << endl;
	cmd->value.push(mem->spaceHead[mem->space.top()].top());
}

static void exitSpace_f(Command* cmd, Compiler::GlobalMemory* mem) {
	mem->exitSpace();
}

static void return_f(Command* cmd, Compiler::GlobalMemory* mem) {
	mem->retVal = cmd->getInput(0, mem);
	mem->exit++;
}

static void function_f(Command* cmd, Compiler::GlobalMemory* mem) {
	if (!(cmd->def) || !(*cmd->def)) {
		LOGGER.error("Using a undefined function");
		return;
	}
	DataList tmp;
	for (uint i = 0; i < cmd->size(); i++) {
		tmp += cmd->getInput(i, mem);
	}
	int space = (*cmd->def)->space;
	mem->enterSpace(space);
	mem->pool += tmp;
	(*cmd->def)->cmd->run(mem);
	(*cmd->def)->cmd->clear();

	mem->exitSpace();

	if (mem->retVal.size()&&mem->exit==0) {
		cmd->value = mem->retVal;
		mem->retVal.clear();
	}

}

static void if_f(Command* cmd, Compiler::GlobalMemory* mem) {
	bool val = cmd->input<bool>(0,mem);
	if (val)cmd->getInput(1, mem);
}

static void not_f(Command* cmd, Compiler::GlobalMemory* mem) {
	bool val = cmd->input<bool>(0, mem);
	cmd->value.push(!val);
}

static void and_f(Command* cmd, Compiler::GlobalMemory* mem) {
	if (!cmd->input<bool>(0, mem)) { cmd->value.push(false); return; }
	if (!cmd->input<bool>(1, mem)) { cmd->value.push(false); return; }
	cmd->value.push(true);
}

static void or_f(Command* cmd, Compiler::GlobalMemory* mem) {
	if (cmd->input<bool>(0, mem)) { cmd->value.push(true); return; }
	if (cmd->input<bool>(1, mem)) { cmd->value.push(true); return; }
	cmd->value.push(false);
}

template<typename T>
static void equal_f(Command* cmd, Compiler::GlobalMemory* mem) {
	T v1 = cmd->input<T>(0,mem);
	T v2 = cmd->input<T>(1,mem);
	cmd->value.push(bool(v1==v2));
}

inline void swapCommand(Command*& cmd, Command::PerformFunction f) {
	Command* tmp = cmd;
	cmd = new Command(f);
	cmd->commands.push_back(tmp);
}

static void _in_f(Command* cmd, Compiler::GlobalMemory* mem) {
	DataList::Reference ref = cmd->getInput(0, mem).getReference(0);
	//cout << "in_f: " << ref.len << "," << ref.addr << endl;
	while (mem->in.size() < ref.len){
		Sleep(0);//cout << mem->in.size()<<endl;
	}
	/*mutex mu; condition_variable condition;
	unique_lock<std::mutex> locker(mu);
	condition.wait(locker, [mem,ref] {return mem->in.size() >= ref.len; });*/
	//cout <<"mem: "<< mem->in.size() << endl;
	for (uint i = 0; i < ref.len; i++)
		mem->pool.bytes[ref.addr + i] = mem->in.pop();
}

static void _out_f(Command* cmd, Compiler::GlobalMemory* mem) {
	mem->out.push(cmd->getInput(0, mem));
	//cout << "out: " << mem->out.size() << endl;
}


void Compiler::init() {
	addType("*", DataType(sizeof(addr_t)));
	addType("&", DataType(sizeof(DataList::Reference)));
	addType("void", DataType(0));
	addType("bool", DataType(sizeof(bool)));
	addType("float", DataType(sizeof(float)));
	addType("int", DataType(sizeof(int)));

	functions["int"].push_back(Function(init_f<float,int>, VAR("int",VAR_REF), {VAR("float")}, sizeof(int)));
	functions["float"].push_back(Function(init_f<int, float>, VAR("float",VAR_REF), {VAR("int")}, sizeof(float)));
	functions["*"].push_back(Function(init_f<int,addr_t>,VAR("*",VAR_REF), { VAR("int") },sizeof(addr_t)));
	functions["bool"].push_back(Function(init_f<int, bool>, VAR("bool", VAR_REF), { VAR("int") },sizeof(bool)));
	functions["bool"].push_back(Function(init_f<addr_t, bool>, VAR("bool", VAR_REF), { VAR("*") }));

	functions["print"].push_back(Function([](Command* cmd, GlobalMemory* mem) {
		cout << "int: "<< cmd->getInput(0, mem).get<int>(0) << endl;
		}, VAR("void"), { VAR("int")}));
	functions["print"].push_back(Function([](Command* cmd, GlobalMemory* mem) {
		cout <<"float: "<< cmd->getInput(0, mem).get<float>(0) << endl;
		}, VAR("void"), { VAR("float")}));
	functions["print"].push_back(Function([](Command* cmd, GlobalMemory* mem) {
		cout << "*: " << cmd->getInput(0, mem).get<addr_t>(0) << endl;
		}, VAR("void"), { VAR("*") }));

	functions["exit"].push_back(Function([](Command* cmd, GlobalMemory* mem) {
		mem->exitCode = cmd->getInput(0, mem).get<int>(0);
		mem->exit = int(mem->space.size());
		}, VAR("void"), { VAR("int") }));

	functions["if"].push_back(Function(if_f, VAR("void"), { VAR("bool"),VAR("void")}));

	functions["+"].push_back(Function(add_f<float,float,float>, VAR("float"), {VAR("float"),VAR("float")}));
	functions["+"].push_back(Function(add_f<int,int,int>, VAR("int"), { VAR("int"),VAR("int") }));
	functions["+"].push_back(Function(add_f<addr_t,addr_t,addr_t>, VAR("*"), { VAR("*"),VAR("*") }));

	functions["-"].push_back(Function(sub_f<float, float, float>, VAR("float"), { VAR("float"),VAR("float") }));
	functions["-"].push_back(Function(sub_f<int, int, int>, VAR("int"), { VAR("int"),VAR("int") }));
	functions["-"].push_back(Function(sub_f<addr_t, addr_t, addr_t>, VAR("*"), { VAR("*"),VAR("*") }));

	functions["*"].push_back(Function(mul_f<float, float, float>, VAR("float"), { VAR("float"),VAR("float") }));
	functions["*"].push_back(Function(mul_f<int, int, int>, VAR("int"), { VAR("int"),VAR("int") }));

	functions["/"].push_back(Function(div_f<float, float, float>, VAR("float"), { VAR("float"),VAR("float") }));
	functions["/"].push_back(Function(div_f<int, int, int>, VAR("int"), { VAR("int"),VAR("int") }));

	functions["-"].push_back(Function(neg_f<float>, VAR("float"), { VAR("float")}));
	functions["-"].push_back(Function(neg_f<int>, VAR("int"), { VAR("int") }));

	functions["!"].push_back(Function(not_f, VAR("bool"), { VAR("bool")}));
	functions["=="].push_back(Function(equal_f<int>, VAR("bool"), { VAR("int"),VAR("int") }));
}

void Compiler::addVariable(const string& t, const string& tname, const string& vname, LocalMemory*& local) {
	addr_t stride = newVar(vname,t,tname,local,true);
	
	Command* ret = new Command(addVariable_f);
	DataList list;
	list.push(stride);
	ret->commands.push_back(new Command(list));
	command = ret;
	rvar = VAR("void");
}


void Compiler::addVariable(const string& type, Expression& expr, LocalMemory*& local) {
	string t = type, tname = type;
	const Expression* e;
	bool init = false, ref = false;
	if (expr.name == "=")e = &expr.params[0], init = true;
	else e = &expr;
	if (e->name == "~*")t = "*";
	else if (e->name == "~&")t = "&";
	while (e->name[0] == '~') {
		if (e->name[1] == '&') {
			if (ref)LOGGER.error("Cannot get the reference of a renference");
			ref = true;
		}
		else {
			if (ref)LOGGER.error("Cannot get the pointer of a reference");
		}
		tname.push_back(e->name[1]);
		e = &e->params[0];
	}
	if (init)initVariable(t, tname, expr.params[1], local, e->name);
	else addVariable(t, tname, e->name, local);
}

bool Compiler::compileElse(Expression& expr,LocalMemory*& local) {
	bool elseif = (expr.name == "else if");
	if (expr.name != "else" && !elseif)return false;
	rvar = VAR("void");
	if (!bgc || bgc->func != if_f) {
		LOGGER.error("Missing if before else"); return true;
	}
	if (!expr.size()) {
		if (!elseif)bgc = 0;
		return true;
	}
	Command* val = bgc->commands[0];Command* ifc = bgc;
	Command* ret = new Command(if_f);
	swapCommand(val, not_f);
	ret->commands.push_back(val);
	if (elseif) {
		expr.name = "if"; compile(expr, local);
		ret->commands.push_back(command);
		bgc = ifc;
		swapCommand(bgc->commands[0], or_f);
		bgc->commands[0]->commands.push_back(command->commands[0]);
	}
	else {
		compile(expr[0], local);
		ret->commands.push_back(command);
		bgc = 0;
	}
	command = ret;
	rvar = VAR("void");
	return true;
}

void Compiler::pushVariable(const Variable& v,bool attrib) {
	string type = v.type, base = getBaseType(v.type);
	DataList list;list.pushReference(DataList::Reference({ getTypeLength(base),v.addr }));
	Command* ref = new Command(list);
	if (attrib) {
		command = new Command(attrib_f);
		command->commands.push_back(ref);
		DataList splist; splist.push(v.space);
		command->commands.push_back(new Command(splist));
	}
	else command = ref;
	//cout << "push: " << type << endl;
	rvar = VAR(type, VAR_REF);
}

Command* Compiler::newSpace(LocalMemory*& local,bool cmd) {
	local = new LocalMemory(*local);
	local->address = 0;
	local->space=memory->spaceHead.size();
	memory->spaceHead.push_back(stack<addr_t>());
	spaceVariables.push_back(VarMap());
	if (!cmd)return 0;

	Command* ret = new Command(enterSpace_f);
	pushValue("int", local->space);
	ret->commands.push_back(command);
	swapCommand(ret, commands_f);
	return ret;
}

void Compiler::compileFunction(Expression& expr,LocalMemory*& local) {
	Command::FunctionDefination* def = 0;
	Expression& f = expr[1];
	Expression* sen = 0;
	Function func(function_f, expr[0].name, {});
	if (f.params.back().name == ";") {
		sen = new Expression(f.params.back());
		f.params.pop_back();
		newSpace(local, false);
	}
	for (Expression& v : f.params) {
		if (v.name != "~vi") {
			LOGGER.error("Invalid function input: " + v.serialize());
			return;
		}
		if (sen)newVar(v[1].name, getBaseType(v[0].name), v[0].name, local,true);
		func.invar.push_back(VAR(v[0].name));
	}
	string pattern = func.pattern();
	Function* same = 0;
	for (Function& tf : functions[f.name]) {
		if (tf.pattern() == pattern) {
			same = &tf; break;
		}
	}
	if (same) {
		if (same->func != function_f || (*same->def)) {
			LOGGER.error("Redefined function: "+func.serialize(f.name)); 
			return;
		}
	}
	else {
		functions[f.name].push_back(func);
		functions[f.name].back().def = new Command::FunctionDefination * (def);
	}

	if (sen) {
		def = new Command::FunctionDefination();
		def->space = local->space;

		def->cmd = new Command(commands_f);
		bgf = &func;
		for (Expression& u : sen->params) {
			compile(u, local);
			if (command)def->cmd->commands.push_back(command);
		}
		bgf = 0;
	}

	if (same) {
		(*same->def) = def;
		cout << "def_same: " << func.serialize(f.name) << endl;
	}
	else {
		*(functions[f.name].back().def) = def;
		//cout << "def: " << func.serialize(f.name) << endl;
	}
	command = 0;
	rvar = VAR("void");
}

void Compiler::compilePtrAttrib(Expression& expr,LocalMemory*& local) {
	if (expr.size() != 2) {
		LOGGER.error("Function -> has 2 parameters"); return;
	}
	Command* ret = new Command(ptrAttrib_f);
	compile(expr[0], local);
	string base = getBaseType(rvar.type); rvar.type.pop_back();
	if (base != "*" || (rvar.type.size() && rvar.type.back() == '*')) {
		cout << "got "<<expr[0] << "," << expr[0].functional() << endl;
		LOGGER.error("Function (->).param[0] must be a pointer of a refernece");
		return;
	}
	if (rvar.vtype == VAR_REF)swapCommand(command, reference_f);
	ret->commands.push_back(command);
	VarMap& vars = *typeSpace(rvar.type, local);

	if (!expr[1].isVariable()) {
		LOGGER.error("Function (->).param[1] must be an attrib of " + rvar.type);
		return;
	}
	if (vars.find(expr[1].name) == vars.end()) {
		LOGGER.error("Unknown attrib of " + rvar.type);
		return;
	}
	pushVariable(vars[expr[1].name], false);
	ret->commands.push_back(command);
	command = ret;
}

void Compiler::compileRefAttrib(Expression& expr, LocalMemory*& local) {
	if (expr.size() != 2) {LOGGER.error("Function . has 2 parameters"); return;}
	Command* ret = new Command(refAttrib_f);
	compile(expr[0], local);
	string base = getBaseType(rvar.type);
	if (base == "*" || rvar.vtype!=VAR_REF) {
		cout << expr[0] << "," << expr[0].functional() << endl;
		LOGGER.error("Function (.).param[0] must be a reference of a value, got " + rvar.serialize());
		return;
	}
	ret->commands.push_back(command);
	VarMap& vars = *typeSpace(rvar.type, local);

	if (!expr[1].isVariable()) {
		LOGGER.error("Function (.).param[1] must be an attrib of " + rvar.type);
		return;
	}
	if (vars.find(expr[1].name) == vars.end()) {
		LOGGER.error("Unknown attrib of " + rvar.type);
		return;
	}
	pushVariable(vars[expr[1].name], false);
	ret->commands.push_back(command);
	command = ret;
}

void Compiler::compile(Expression& expr, LocalMemory* local) {
	//cout << "compile: " << expr << "," << local->space << endl;
	if (compileElse(expr, local)) return;
	rvar = VAR("void");
	command = 0;
	if (expr.isVariable()) {
		if (expr.name == "this") {
			command = new Command(this_f);
			rvar = VAR("*");
			return;
		}
		else if (expr.name == "true") {
			pushValue("bool", true);
			return;
		}
		else if (expr.name == "false") {
			pushValue("bool", false);
			return;
		}
		Variable* v = local->find(expr.name);
		int num;
		if (v) pushVariable(*v,true);
		else if (num = isNumber(expr.name)) {
			if (num == 2) pushValue("int", stoi(expr.name));
			else pushValue("float", float(stod(expr.name)));
		}
		else LOGGER.error("Unknown variable: " + expr.name);
		return;
	}
	else if (expr.name == "~v"||expr.name=="~vi") {
		if (expr.params[1].name == ",") {
			Command* ret = new Command(commands_f);
			for (Expression& e : expr[1].params) {
				addVariable(expr[0].name, e,local);
				ret->commands.push_back(command);
			}
			command = ret;
		}
		else addVariable(expr[0].name,expr[1], local);
		return;
	}
	else if (expr.name == "~f") {
		compileFunction(expr,local);
		return;
	}
	else if (expr.name == ";") {
		Command* ret=newSpace(local,true);

		for (Expression& e : expr.params) {
			compile(e, local);
			if(command)ret->commands.push_back(command);
		}
		ret->commands.push_back(new Command(exitSpace_f));
		command = ret;
		return;
	}
	else if (expr.name == "&"&& expr.size() == 1) {
		Command* ret = new Command(address_f);
		compile(expr[0],local);
		if (rvar.vtype != VAR_REF)
			LOGGER.error("Cannot get the address of a non-reference var");
		ret->commands.push_back(command);
		command = ret;
		rvar = VAR(rvar.type+"*");
		return;
	}
	else if (expr.name == "*"&& expr.size() == 1) {
		Command* ret = new Command(ptr2ref_f);
		compile(expr[0],local);
		if (rvar.type.back()!='*')
			LOGGER.error("Cannot get the reference that a non-pointer var pointed");
		if (rvar.vtype == VAR_REF)swapCommand(command, reference_f);
		rvar.type.pop_back();
		string type = rvar.type;
		ret->commands.push_back(command);
		pushValue<addr_t>("*",getTypeLength(getBaseType(rvar.type)));
		ret->commands.push_back(command);
		command = ret;
		rvar = VAR(type, VAR_REF);
		return;
	}
	else if (expr.name == "->") {compilePtrAttrib(expr,local);return;}
	else if (expr.name == ".") {compileRefAttrib(expr, local); return;}
	else if (expr.name == "return") {
		if (expr.size() > 1) {
			LOGGER.error("return only 1 param");
			return;
		}
		if (!bgf) {
			LOGGER.error("return must be in the function.");
			return;
		}
		Command* ret = new Command(return_f);
		compile(expr[0], local);
		translateType(bgf->out,local);
		ret->commands.push_back(command);
		command = ret;
		rvar = VAR("void");
		return;
	}
	else if (expr.name == "struct"||expr.name=="class") {
		if (expr.size()!=1 || expr[0].size()!=1||expr[0][0].name!=";") {
			LOGGER.error("Invalid class defination: " + expr.serialize());
			return;
		}
		string& type = expr[0].name;
		compile(expr[0][0],local);
		int space = memory->spaceHead.size()-1;
		cout << "class: " << space << " | " << expr << "," << type << endl;
		typeSpaces[type] = space;
		VarMap& map = spaceVariables[space];
		addType(type,DataType(map,types));
		command = 0; rvar = VAR("void");
		return;
	}
	vector<VAR> vars;
	vector<Command*> commands;
	for (Expression e : expr.params) {
		compile(e, local);
		commands.push_back(command);
		vars.push_back(rvar);
	}
	if (expr.name == "_in" && vars.size() == 1 && vars[0].vtype == VAR_REF) {
		command = new Command(_in_f);
		command->commands.push_back(commands[0]);
		rvar = VAR("void");
		return;
	}
	else if (expr.name == "_out" && vars.size() == 1) {
		command = new Command(_out_f);
		if (vars[0].vtype == VAR_REF)swapCommand(commands[0], reference_f);
		command->commands.push_back(commands[0]);
		rvar = VAR("void");
		return;
	}
	useFunction(expr.name,commands,vars,local);
	if (expr.name == "if")bgc = command;
}

void Compiler::translateType(const VAR& var,LocalMemory*& local) {
	if (var.vtype == VAR_VALUE && rvar.vtype == VAR_REF) {
		swapCommand(command, reference_f);
	}
	else if (var.vtype != rvar.vtype||(var.type!=rvar.type&&var.vtype==VAR_REF)) {
		LOGGER.error("Caanot translate "+rvar.serialize()+" to "+var.serialize());
		return;
	}
	if (var.type != rvar.type) {
		vector<Command*> cmds = { command };
		vector<VAR> vars = { rvar };
		useFunction(var.type, cmds, vars, local);
	}
	else rvar = var;
}

Compiler::Function* Compiler::matchTrans(const string& name, const vector<VAR>& vars, Command** cmds, Command** trans,addr_t& off) {
	//cout << "match function: " << funcLog(name, vars) << endl;
	if (functions.find(name) == functions.end())
		LOGGER.error("Unknown function: " + funcLog(name, vars));
	for (Function& func : functions[name]) {
		if (func.invar.size() != vars.size())continue;
		bool aq = true;
		for (uint i = 0; i < func.invar.size(); i++) {
			trans[i] = cmds[i];
			if (func.invar[i].vtype == VAR_VALUE && vars[i].vtype == VAR_REF) {
				trans[i] = new Command(reference_f);
				trans[i]->commands.push_back(cmds[i]);
			}
			if (func.invar[i].type != vars[i].type ||
				(func.invar[i].vtype != vars[i].vtype && func.invar[i].vtype != VAR_VALUE)) {
				aq = false; break;
			}
		}
		if (aq) {
			off += func.off;
			return &func;
		}
	}
	return 0;
}

Compiler::Function* Compiler::matchFunction(const string& name, const vector<VAR>& vars, Command** cmds, Command** trans, addr_t& off) {

	Function* ret = matchTrans(name,vars,cmds,trans,off);
	if (ret)return ret;
	for (Function& func : functions[name]) {
		if (func.invar.size() != vars.size())continue;
		bool e = false;
		addr_t stride = 0;
		for (uint i = 0; i < func.invar.size(); i++) {
			trans[i] = cmds[i];
			if (func.invar[i].vtype == VAR_VALUE && vars[i].vtype == VAR_REF) {
				swapCommand(trans[i],reference_f);
			}
			if (func.invar[i].type != vars[i].type) {
				if (func.invar[i].vtype == VAR_REF) {
					e = true; break;
				}
				//const reference,reference,value
				Command* cmd;
				Function* tmp = matchTrans(func.invar[i].type, { VAR(vars[i].type) }, trans + i, &cmd,stride);
				if (!tmp) { e = true; break; }
				swapCommand(trans[i], tmp->func);
				swapCommand(trans[i], reference_f);
			}
		}
		if (e)continue;
		if (ret) {
			LOGGER.error("More than 1 suitable functions: " + funcLog(name, vars));
			return 0;
		}
		ret = &func;
		off += stride;
	}
	return ret;
}



void Compiler::useFunction(const string& name, vector<Command*>& commands, vector<VAR>& vars,LocalMemory*& local) {
	string fun = name,outname;
	
	if (fun.back() == '*') {
		outname = fun;
		fun = "*";
	}
	else if (fun.back() == '&') {
		fun.pop_back();
		if(vars.size()!=1||vars[0].type!=fun||vars[0].vtype!=VAR_REF) {
			cout << vars[0].type << "," << int(vars[0].vtype) << endl;
			LOGGER.error("(Reference init)Cannot find a suitable overloading for: " + funcLog(name, vars));
			return;
		}
		command = new Command(initReference_f);
		command->commands.push_back(commands[0]);
		rvar = VAR("&",VAR_REF);
		return;
	}
	for (uint i = 0; i < vars.size(); i++) {
		string& t = vars[i].type;
		if (t.back() == '&') {
			t.pop_back();
			swapCommand(commands[i], elimateReference_f);
		}
		if (t.back() == '*') t = "*";
	}

	vector<Function*> trans(vars.size());
	vector<Command*> cmds(commands.size());
	addr_t off=0;
	Function* func = matchFunction(fun, vars, &commands[0],&cmds[0],off);
	if (!func) {
		LOGGER.error("Cannot find a suitable overloading for: " + funcLog(name, vars));
		return;
	}
	local->address += off;
	command = new Command(func->func,func->def);
	command->commands = cmds;
	rvar = func->out;
	//cout << "using func: " << func->serialize(name) << "," << off << endl;
}


