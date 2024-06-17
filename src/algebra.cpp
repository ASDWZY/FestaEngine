#include "include/FestaEngine/algebra/display.h"

#define P1 4
#define P2 2

static unordered_map<string, int> priorities =
{
	{"^",1}, {"*",P2},{"/",P2} ,{"+",P2 + 1},{"-",P2 + 1},
	{"=",P1 + 1}, {"<",P1},{">",P1},
	{",",6},{")",6} };

static unordered_map<string, int> operators =
{ {"^",2}, {"+",1},{"-",1} ,{"*",2} ,{"/",2},
	{"=",2}, {"<",2},{">",2}
};


void Expression::deserialize(const string& expr) {
	stack<Expression> res;
	res.push(Expression("", -1));

	uint start = 0;
	for (uint i = 0; i < expr.size(); i++) {
		int p;
		Expression op("", -1);
		op.name.push_back(expr[i]);
		if (expr[i] == '(') {
			appendExpr(expr, res, start, i, -1);
			continue;
		}
		if (!priorities[op.name])continue;
		appendExpr(expr, res, start, i, 0);
		while (i + 1 < expr.size()) {
			if (!priorities[op.name + expr[i + 1]])break;
			op.name.push_back(expr[++i]);
			++start;
		}
		p = priorities[op.name];
		fold(res, op.name);
		if (!res.size()) {
			LOGGER.error("MissingExpressionBefore " + op.name);
			return;
		}
		int tp = res.top().priority();
		if (expr[i] == ')')continue;
		if (expr[i] == ',') {
			if (res.top().isRealFunction() && res.top().state == -1)
				continue;
			else if (tp > p) {
				LOGGER.error("MissingExpressionBefore" + op.name + "\n" + res.top().serialize() + " | " + op.serialize());
				return;
			}
			else if (res.top().name == op.name && res.top().state == -1)continue;
		}
		else if (tp > p || res.top().state == -1) {
			if (operators[op.name] == 1)res.push(op);
			else {
				LOGGER.error("MissingExpressionBefore" + op.name);
				return;
			}
			continue;
		}
		if (tp <= p && res.top().state != -1) {
			op.params.push_back(res.top());
			res.pop();
		}
		res.push(op);
	}
	appendExpr(expr, res, start, uint(expr.size()), 0);
	fold(res, ",");
	finishFunc(res);
	res.top().simplify();
	if (res.size() != 1||res.top().check()) {
		LOGGER.error("ExpressionNotCompletedError");
		return;
	}
	*this = res.top();
}

string Expression::serialize()const {
#ifdef EXPRESSION_DEBUG
	return functional();
#endif		if (state == 0)return name;
	string ret, tmp;
	int p = priority();
	if (name == ",") {
		for (uint i = 0; i < params.size(); i++) {
			tmp = params[i].serialize();
			ret += tmp;
			if (i != params.size() - 1)ret.push_back(',');
		}
		if (state == -1)ret += ", ...";
		//ret.push_back(')');
		return ret;
	}
	else if (operators[name]) {
		if (operators[name] == 1) {
			if (!size() && state == -1)return name + "...";
			else if (size() == 1 && state == 1) {
				if (params[0].priority() >= p)return name + "(" + params[0].serialize() + ")";
				else return name + params[0].serialize();
			}
		}
		tmp = params[0].serialize();
		if (params[0].priority() > p)tmp = "(" + tmp + ")";
		if (state == -1)return tmp + name + "...";
		else {
			ret = params[1].serialize();
			//+ and * 
			// if(params[0].priority() > p||(params[1].priority()==p&&name!="+"&&name!="*"))
			if (params[1].priority() >= p && params[1].name != "," && params[1].name != ";")ret = "(" + ret + ")";
			return tmp + name + ret;
		}
	}
	if (state == 0)return name;
	ret = name;
	ret.push_back('(');
	for (uint i = 0; i < params.size(); i++) {
		ret += params[i].serialize();
		if (i != params.size() - 1)ret += ",";
	}
	if (state == -1)ret += "...";
	ret.push_back(')');
	return ret;
}

void Expression::fold(stack<Expression>& st, const string& op) {
	uint num = 0;
	while (st.size()) {
		if (st.top().state == -1) {
			//cout << "check " << st.top() <<","<<op<< endl;
			bool rf = st.top().isRealFunction(); int p;
			if (rf)p = priorities[","];
			else p = priorities[st.top().name];
			if (op == ")" && rf) {
				finishFunc(st);
				return;
			}
			else if ((p < priorities[op]) ||
				(p <= priorities[op] && !rf)) {
				finishFunc(st);
			}
			else return;
		}
		if (st.size() < 2)return;
		else if (num && op == ")")return;
		Expression child = st.top(); st.pop();
		child.normalize();
		Expression& father = st.top();
		if (father.state != -1) {
			st.push(child); return;
		}
		int p = father.priority();
		if (!p)p = priorities[","];//,fold params
		if (p <= priorities[op]) {
			if (!father.priority())num++;
#ifdef EXPRESSION_DEBUG
			cout << "fold:" << father << " <- " << child << " | " << op << endl;
#endif
			father.params.push_back(child);
		}
		else {
			st.push(child); return;
		}
	}

}

int Expression::priority()const {
	if (state)return priorities[name];
	else return -1;
}

bool Expression::isVariable()const {
	return state == 0;
}
bool Expression::isRealFunction()const {
	return priorities[name] == 0;
}
int Expression::Operator(const string& name) {
	return operators[name];
}
int Expression::Priority(const string& name) {
	return priorities[name];
}


AlgebraEnvironment AlgebraExpression::env;

static AlgebraExpression add_f(const vector<AlgebraExpression>& inp) {
	bool num; RationalNumber n = 0;
	for (uint i = 0; i < inp.size(); i++) {
		if (!inp[i].isValue()) { num = false; break; }
		else n += inp[i].value;
	}
	Polymomial p(AlgebraExpression("+", inp));
	return p.toExpression();
}
static AlgebraExpression mul_f(const vector<AlgebraExpression>& inp) {
	bool num; RationalNumber n=1;
	for (uint i = 0; i < inp.size(); i++) {
		if (!inp[i].isValue()) { num = false; break; }
		else n *= inp[i].value;
	}
	if (num)return n;
	Monomial m(AlgebraExpression("*",inp));
	return m.toExpression();
}
static AlgebraExpression eq_f(const vector<AlgebraExpression>& inp) {
	if (inp[0].isValue() && inp[1].isValue()) {
		RationalNumber st = int(abs(inp[0].value.toDouble() - inp[1].value.toDouble()) < EPS_FLOAT);
		return st;
	}
	return AlgebraExpression("=", inp);
}
AlgebraEnvironment::AlgebraEnvironment(){
	env["+"] = add_f;
	env["*"] = mul_f;
	env["="] = eq_f;
	env["^"] = 0;
	env["sqrt"] = 0;
	env["sin"] = 0;
	env["cos"] = 0;
	env["tan"] = 0;
	env["exp"] = 0;
	env["abs"] = 0;
	env["log2"] = 0;
	env["asin"] = 0;
	env["acos"] = 0;
	env["atan"] = 0;
	env["log"] = 0;
}

void AlgebraExpression::init(const Expression& expr) {
	if (!expr.params.size() && isNumber(expr.name)) {
		value = RationalNumber(expr.name);
		return;
	}
	uint st=0,n=uint(expr.name.size());
	if (!n) {
		LOGGER.error("Empty AlgebraExpression Error");
		return;
	}
	while (st < n) {
		string str,t; bool num = false;
		for (uint i=st; i < expr.name.size(); i++) {
			str.push_back(expr.name[i]);
			if (isNumber(str))num = true, t = str, st = i + 1;
			else if (find(str))t=str,st=i+1,num=false;
		}
		if(!t.size()) {
			LOGGER.error("Unknown function or variable: " + expr.name);
			return;
		}
		if (num)children.push_back(RationalNumber(t));
		else children.push_back(AlgebraExpression(t, {}));
	}
	f = "*";
	AlgebraExpression& e = children.back();
	if (expr.size()&&isSymbol(e.f)) {
		if (expr.size()!=1) {
			LOGGER.error("Unsupport format: " + expr.serialize());
			return;
		}
		AlgebraExpression tmp; tmp.init(expr.params[0]);
		children.push_back(tmp);
		return;
	}
	e.children.resize(expr.size());
	for (uint i = 0; i < expr.size(); i++)
		e.children[i].init(expr.params[i]);
	if (children.size() == 1) *this = AlgebraExpression(e);
	if (f == "*"||f=="-") {
		Monomial m(*this);
		*this = m.toExpression();
	}
	else if (f == "+") {
		Polymomial p(*this);
		*this = p.toExpression();
	}
}


Function2D::Functions Function2D::functions;
Function2D::Functions::Functions() {
	fmap["+"] = [this](const vector<uint>& inp) {
		float x = 0.0f;
		for (uint i = 0; i < inp.size(); i++)x += f[inp[i]]();
		return x;
		};
	fmap["*"]= [this](const vector<uint>& inp) {
		float x = 1.0f;
		for (uint i = 0; i < inp.size(); i++)x *= f[inp[i]]();
		return x;
		};
	fmap["="] = [this](const vector<uint>& inp) {
		return float(fabsf(f[inp[0]]() - f[inp[1]]())<=EPS_FLOAT);
		};
	fmap["x"]= [this](const vector<uint>& inp) {
		return x;
		};
	fmap["y"] = [this](const vector<uint>& inp) {
		return y;
		};
}

Function2D::Function2D(const AlgebraExpression& expr) {
	name = expr.f;
	if (!expr.f.size()) {
		value = float(expr.value.toDouble());
		return;
	}
	else f = functions.fmap[expr.f];
	for (uint i = 0; i < expr.children.size(); i++) {
		functions.f.push_back(Function2D(expr.children[i]));
		children.push_back(functions.f.size()-1);
	}
}


ShaderSource FunctionGraph2D::VS(GL_VERTEX_SHADER,
	"#version 330 core\n"
	"layout(location=0)in vec2 Pos;"
	"out vec2 pos;"
	"void main(){"
	"   gl_Position=vec4(Pos,0.0f,1.0f);\n"
	"	pos=Pos;\n"
	"}"),
	FunctionGraph2D::GRID_FS(GL_FRAGMENT_SHADER,
		"#version 330 core\n"
		"uniform vec2 size;\n"
		"uniform vec2 ori;\n"
		"uniform int bw;uniform int sw;\n"
		"in vec2 pos;\n"
		"out vec4 FragColor;\n"
		//"int ABS(int x){if(x>=0)return x;else return -x;}"
		"void main(){\n"
		"	vec2 d=ori-pos*size*0.5f;\n"
		"	int dx = abs(int(d.x)), dy = abs(int(d.y));"
		"	float c = 1.0;\n"
		"	if (dx <=1 || dy <=1)c = 0.47;"
		"	else if (dx % bw <=1 || dy % bw <=1)c = 0.784;"
		"	else if (dx % sw <=1 || dy % sw<=1)c = 0.941; "
		"	FragColor=vec4(c,c,c,1.0);\n"
		"}"
	), FunctionGraph2D::GRAPH_FS(GL_FRAGMENT_SHADER,
		"#version 330 core\n"
		"struct Function{int num;int type;float value;int ch[4];};"
		"struct Node{int f,i;};"

		"uniform vec2 size;"
		"uniform vec2 ori;"
		"uniform vec3 color;"
		"uniform int rootFunc;"

		"const int N=100;"
		"uniform Function functions[N];"
		"Node stack[N];int top=-1;"

		"in vec2 pos;"
		"out vec4 FragColor;"

		//"void push(Node x){stack[++top]=x;}"
		"float eq(float x,float y){if(abs(x-y)<2.0f)return 1.0f;else return 0.0f;}"

		"void main(){"
		"	vec2 coord=pos*size*0.5f-ori;"
		//"	if(eq(coord.x,coord.y)>0.0f)FragColor=vec4(1.0,1.0,0.0,1.0f);"
		//"	else FragColor=vec4(0.0);return;"
		"	Node node;node.f=rootFunc;node.i=0;stack[++top]=node;"
		"	while(top>-1){"
		"		Function f=functions[stack[top].f];int i=stack[top].i;"
		"		if(i<(f.num-1)){"
		"			stack[top].i++;Node node;node.f=f.ch[i+1];node.i=0;stack[++top]=node;"
		"		}else{"
		"			if(f.type==0)f.value=coord.x;"
		"			else if(f.type==1)f.value=coord.y;"
		"			else if(f.type==-1)f.value=eq(functions[f.ch[0]].value,functions[f.ch[1]].value);"
		"			"
		"			top--;}"
		"	}"
		"	if(functions[rootFunc].value>0.0f)FragColor=vec4(color,1.0f);"
		"	else FragColor=vec4(0.0f);"
		"}"
	);
ProgramSource FunctionGraph2D::GRID_PROGRAM({FunctionGraph2D::VS,FunctionGraph2D::GRID_FS}),
FunctionGraph2D::GRAPH_PROGRAM({ FunctionGraph2D::VS,FunctionGraph2D::GRAPH_FS });