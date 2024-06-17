#pragma once

#include "../../common/common.h"
#include <stack>

#define stringIndex(str,begin,end) str.substr(begin,(end)-(begin))
//#define EXPRESSION_DEBUG

using namespace Festa;



inline int isNumber(const string& str) {
	//positive only  0not number,2int,1float
	int ret = 2;
	bool pt = false, e = false,f=false;
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
			if (i==0||i != (str.size() - 1)||f)return 0;
		}
		else if (s == '.') {
			ret = 1;
			if (pt)return 0;
			pt = true;
		}
		else return 0;
	}
	return ret;
}


class Expression {
public:
	string name;
	vector<Expression> params;
	Expression() {}
	Expression(const string& expr) {
		deserialize(expr);
	}
	Expression(const string& name, int state)
		:name(name), state(state){}
	static void init();
	void deserialize(const string& expr);
	string serialize()const;
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
	bool check()const {
		if (state == -1)return true;
		for (uint i = 0; i < size(); i++)
			if (params[i].check())return true;
		return false;
	}
	uint size()const {
		return uint(params.size());
	}
	Expression& operator[](uint index) {
		Expression& i = params[index];
		return i;
	}
	friend ostream& operator<< (ostream& out, const Expression& expr) {
		out << expr.serialize();
		return out;
	}
	bool empty()const {
		return size() == 0 && name.size() == 0;
	}
	bool isFunction()const {
		return bool(state);
	}
	bool isVariable()const;
	bool isRealFunction()const;
	static int Operator(const string& name);
	static int Priority(const string& name);
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
		if (s < e)p.push_back(stringIndex(expr, s, e));
	}
	static void appendExpr(const string& expr, stack<Expression>& st, uint& start, uint i, int lstate) {
		vector<string> p;
		split(p, expr, start, i);
		if (!p.size()) {
			if (lstate == -1) {
				Expression tmp("", -1);
				st.push(tmp);
			}
			return;
		}
		else if (p.size() == 1) {
			st.push(Expression(p[0], lstate));
			return;
		}
		st.top().state = lstate;
	}
	static void fold(stack<Expression>& st, const string& op);
	static void finishFunc(stack<Expression>& st) {
		Expression& e = st.top();
		if (e.state == 0)return;
		else if (int(e.size()) < Expression::Operator(e.name))return;
		e.state = 1;
#ifdef EXPRESSION_DEBUG
		cout << "finish: " << e << endl;
#endif
	}
	void normalize() {
		if (state != 1)return;
		else if (name == ",") {
			if (params.size() == 0)*this = Expression();
			else if (params.size() == 1)name.clear();
		}
	}
	void simplify() {
		if (state == 0) {
			if (isNumber(name))return;
			return;
		}
		for (Expression& child : params)child.simplify();
		if (name == "," || !name.size()) {//||name==";"
			if (params.size() == 0)*this = Expression();
			else if (params.size() == 1) {
				Expression tmp = params[0]; *this = tmp;
			}
			else if (!name.size())name = ",";
		}
	}
	int priority()const;
};