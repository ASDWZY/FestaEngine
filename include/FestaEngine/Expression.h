#pragma once

#include "../common/common.h"
#include <stack>

#define stringIndex(str,begin,end) str.substr(begin,(end)-(begin))
//#define EXPRESSION_DEBUG



inline int isNumber(const string& str) {
	//positive only  0not number,2int,1float
	int ret = 2;
	bool pt = false, e = false;
	for (uint i = 0; i < str.size(); i++) {
		char s = str[i];
		if ('0' <= s && s <= '9') {
			continue;
		}
		else if (s == 'e') {
			ret = 1;
			if (i == 0 || i == str.size() - 1 || e)return 0;
			else e = true;
		}
		else if (s == 'f') {
			ret = 1;
			if (i < str.size() - 1)return 0;
		}
		else if (s == '.') {
			ret = 1;
			if (pt)return 0;
			pt = true;
		}
		else if (s == '-') {
			if (i != 0)return 0;
		}
		else return 0;
	}
	return ret;
}




class Expression {
public:
	string name;
	uint left=0, len=0;
	vector<Expression> params;
	static unordered_map<string, int> priorities;
	static unordered_map<string, bool> reserved;
	static unordered_map<string, int> operators;
	Expression() {}
	Expression(const string& expr) {
		deserialize(expr);
	}
	Expression(const string& name, int state,uint left,uint len) 
		:name(name), state(state),left(left),len(len) {}


	void deserialize(const string& expr) {
		stack<Expression> res;

		uint start = 0;
		for (uint i = 0; i < expr.size(); i++) {
			int p;
			Expression op("",-1,i,0);
			op.name.push_back(expr[i]);

			if (expr[i] == '(') {
				appendExpr(expr, res, start, i, -1);
				continue;
			}
			else if (expr[i] == '{') {
				appendExpr(expr, res, start, i, 1);
				//fold(res, op.name);
				res.push(Expression(";",-1,i+1,0));
				start = i + 1;
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
				return;
			}
			int tp = res.top().priority();
			if (expr[i] == ')' || expr[i] == '}') {
				continue;
			}
			if (expr[i] == ',') {
				if (res.top().isRealFunction()) {
					if (res.top().state == -1)continue;
				}
				else if (tp > p) {
					return;
				}
				else if (res.top().name == op.name && res.top().state == -1)continue;
			}
			else if (expr[i] == ';') {
				if (res.top().name == op.name && res.top().state == -1)continue;

			}
			else if (tp > p || res.top().state == -1) {
				if (operators[op.name] == 1)res.push(op);
				else {
					return;
				}
				continue;
			}
			//cout << "asd: " << op<<","<<tp << "," << res.top().state << endl;
			if (tp <= p&&res.top().state!=-1) {
				op.params.push_back(res.top());
				res.pop();
			}
			res.push(op);
		}
		appendExpr(expr, res, start, uint(expr.size()), 0);
		fold(res,";");
		if (res.empty())return;
		else if (res.size() != 1) {
			while (res.size()) {
				cout << "st " << res.top() << endl;
				res.pop();
			}
			return;
		}
		normalize2(res);
		while (res.top().state == -1) {
			finishFunc(res);
			normalize2(res);
		}
		*this = res.top();
		//if (name != ";")log.error(0, 0, "Missing a ;");
		
	}

	string serialize()const {
#ifdef EXPRESSION_DEBUG
		return functional();
#endif		if (state == 0)return name;
		string ret, tmp;
		int p = priority();
		if (reserved[name]) {
			ret += name;
			if (size() > 1) {
				ret.push_back('(');
				for (uint i = 0; i < size() - 1; i++) {
					ret += params[i].serialize();
					if (i != size() - 2)ret.push_back(',');
				}
				ret.push_back(')');
			}
			ret.push_back(' ');
			ret += params.back().serialize();
			return ret;
		}
		else if (name == ",") {
			//ret = "(";
			for (uint i = 0; i < params.size(); i++) {
				tmp = params[i].serialize();
				ret += tmp;
				if (i != params.size() - 1)ret.push_back(',');
			}
			if (state == -1)ret += ", ...";
			//ret.push_back(')');
			return ret;
		}
		else if (name == ";") {
			ret.push_back('{');
			for (uint i = 0; i < params.size(); i++)
				ret += params[i].serialize() + ";";
			if (state == -1)ret += "...";
			ret.push_back('}');
			return ret;
		}
		else if (operators[name]) {
			if (operators[name]==1) {
				if (!size() && state == -1)return name+"...";
				else if (size() == 1 && state == 1) {
					if (params[0].priority() >= p)return name+"(" + params[0].serialize() + ")";
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
	string functional()const {
		if (state == 0)return name;
		string ret, tmp;
		int p = priority();
		ret = name;
		ret.push_back('(');
		for (uint i = 0; i < params.size(); i++) {
			ret += params[i].functional();
			if (i != params.size() - 1)ret += ",";
		}
		if (state == -1)ret += "...";
		ret.push_back(')');
		return ret;
	}
	uint size()const {
		return uint(params.size());
	}
	Expression& operator[](uint index) {
		Expression& i = params[index];
		return i;
	}
	friend ostream& operator<< (ostream& out, const Expression& str);
	bool empty()const {
		return size() == 0 && name.size() == 0;
	}
	bool isFunction()const {
		return bool(state);
	}
	bool isVariable()const {
		return state == 0;
	}
	bool isRealFunction()const {
		return priorities[name] == 0 && !reserved[name];
	}
private:
	int state = 0;
	static void split(vector<string>& p, const string& expr, uint& start, uint i) {
		uint s = start, e = i;
		while (s < e && (expr[s] == ' ' || expr[s] == '\n' || expr[s] == '\t'))s++;
		while (s < e && e>0 && (expr[e - 1] == ' ' || expr[e - 1] == '\n' || expr[e - 1] == '\t'))e--;
		start = i + 1;
		if (s >= e || e <= 0)return;
		for (uint j = s + 1; j < e && s < e; j++) {
			if (expr[j] == ' ' || expr[j] == '\n' || expr[s] == '\t') {
				if (s < j)p.push_back(stringIndex(expr, s, j));
				s = j + 1;
			}
		}
		if(s<e)p.push_back(stringIndex(expr, s, e));
	}
	static void appendExpr(const string& expr, stack<Expression>& st, uint& start, uint i, int lstate) {
		vector<string> p;
		split(p, expr, start, i);
		if (!p.size()) {
			if (lstate == -1) {
				Expression tmp("",-1,i,0);
				st.push(tmp);
			}
			return;
		}
		else if (p.size() == 1) {
			st.push(Expression(p[0],lstate,i,uint(p[0].size())));
			return;
		}
		else if (lstate == 0 && st.size() && st.top().priority() == 0&&st.top().state==-1) {
			Expression e;
			e.name = "~vi", e.state = 1;
			uint off = i;
			for (string& s : p) {
				e.params.push_back(Expression(s,0,off,uint(s.size())));
				off += uint(s.size());
			}
			st.push(e);
			return;
		}
		while (p.size() && reserved[p[0]]) {
			st.push(Expression());
			st.top().name = p[0], st.top().state = -1;
			p.erase(p.begin());
		}
		if (p.size()) {
			Expression v;
			v.name = p.back();
			p.pop_back();
			if (p.size()) {
				Expression e("~",-1,i,0);
				if (lstate == 0)e.name.push_back('v');
				else if (lstate == -1)e.name.push_back('f');
				uint off = i;
				for (string& s : p) {
					e.params.push_back(Expression(s,0,off,uint(s.size())));
					off += uint(s.size());
				}
				st.push(e);
			}
			st.push(v);
		}
		st.top().state = lstate;
	}
	static void fold(stack<Expression>& st, const string& op) {
		uint num = 0;
		while (st.size()) {
			if (st.top().state == -1) {
				//cout << "check " << st.top() <<","<<op<< endl;
				bool rf = st.top().isRealFunction(); int p;
				if (rf)p = priorities[","];
				else p = priorities[st.top().name];
				if ((op == ")" && rf) ||
					(op == "}" && st.top().name == ";")) {
					finishFunc(st);
					normalize2(st);
					return;
				}
				else if (p < priorities[op]) {
					finishFunc(st);
					normalize2(st);
				}
				else return;
			}
			if (st.size() < 2)return;
			else if (num && (op == ")" || op == "}"))return;
			Expression child = st.top(); st.pop();
			child.normalize();
			Expression& father = st.top();
			if (father.state != -1) {
				st.push(child); return;
			}
			int p = father.priority();
			if (!p)p = priorities[","];//,fold params
			if (p <= priorities[op]) {
				if(!father.priority())num++;
#ifdef EXPRESSION_DEBUG
				cout << "fold:" << father << " <- " << child << " | " << op << endl;
#endif
				father.params.push_back(child);
			}
			else {
				st.push(child); return;
			}
			//if (op == ")" || op == "}")return;
		}

	}
	static void finishFunc(stack<Expression>& st) {
		Expression& e = st.top();
		if (e.state == 0)return;
		e.state = 1;
#ifdef EXPRESSION_DEBUG
		cout << "finish: " << e << endl;
#endif
		if (e.name == ";"&&st.size()>1) {
			Expression sen = e; st.pop();
			if (st.top().name == ";" || reserved[st.top().name]) {
				st.top().params.push_back(sen);
			}
			else {
				st.push(sen);
			}
		}
	}
	void normalize() {
		if (state != 1)return;
		else if (name == ",") {
			if (params.size() == 0)*this = Expression();
			else if (params.size() == 1)name.clear();
		}
	}
	static void normalize2(stack<Expression>& st) {
		if (!st.size()||st.top().state!=1)return;
		Expression& e = st.top();
		//;(definations,commands)
		if (e.name == "~f" || e.name == "if" || e.name == "else") {
			if (e.name == "else" && e[0].name == "if") {
				Expression t = e[0];
				e=t;
				e.name = "else if";
			}
			if (e.size() <= 1&&e.name!="else")return;
			if (e.name != "~f" && e.params.back().name != ";") {
				Expression o(";",1,0,0);
				o.params.push_back(e.params.back());
				e.params.back() = o;
			}
			Expression tmp = e; st.pop();
			if (!st.size() || st.top().name != ";") {
				st.push(Expression(";",-1,0,0));
			}
			st.top().params.push_back(tmp);
		}
	}
	int priority()const {
		if (state)return priorities[name];
		else return -1;
	}
};


unordered_map<string, int> Expression::priorities =
{ 
	{"=",5},
	{",",6},{")",6},{";",7}};
unordered_map<string, bool> Expression::reserved = { {"struct",true},{"class",true} ,{"if",true}, {"else",true} ,{"return",true},{"const",true},{"static",true} };

//0: not
//2: 2
//1: (1 or 2) or 1
unordered_map<string, int> Expression::operators =
{ {"+",1},{"-",1} ,{"*",1} ,{"/",2}, {"&",1},{"!",1} ,
	{"=",2}, {"<",2},{">",2},{"==",2},{"<<",2},
	{"->",2 },{".",2}, {"::",2},
};


ostream& operator<< (ostream& out, const Expression& expr) {
	out << expr.serialize();
	return out;
}