#pragma once 
#include "Expression.h"

class RationalNumber {
public:
	RationalNumber() {}
	RationalNumber(ll x) :p(x),q(1ull) {}
	RationalNumber(int x) :p(x), q(1ull) {}
	RationalNumber(double number) {
		q = 1ull;
		while (number != double(ll(number))) {
			number *= 10.0; q *= 10ull;
		}
		p = ll(number);
		simplify();
	}
	RationalNumber(const string& x) {
		double number = stod(x);
		q = 1ull;
		while (number != double(ll(number))) {
			number *= 10.0; q *= 10ull;
		}
		p = ll(number);
		simplify();
	}
	RationalNumber(ll p,ull q):p(p),q(q) {
		simplify();
	}
	bool initialized()const {
		return p || q;
	}
	bool isInteger()const {
		return q == 1;
	}
	bool isFloat()const {
		return q > 1;
	}
	bool recurring()const {
		ull tmp = q;
		while (tmp>1) {
			if (!(tmp % 2))tmp /= 2;
			else if (!(tmp % 5))tmp /= 5;
			else return true;
		}
		return false;
	}
	ll numerator()const {
		return p;
	}
	ull denominator()const {
		return q;
	}
	string serialize()const {
		if (!q) {
			if (p > 0)return "inf";
			else if (p < 0)return "-inf";
			else return "<Number>";
		}
		else if (q == 1ull)return toString(p);
		return toString(p) + "/" + toString(q);
	}
	friend ostream& operator<< (ostream& out, const RationalNumber& number) {
		out << number.serialize();
		return out;
	}
	bool operator==(const RationalNumber& n)const {
		return (p == n.p) && (q == n.q);
	}
	bool operator!=(const RationalNumber& n)const {
		return (p != n.p) || (q != n.q);
	}
	bool operator>(const RationalNumber& n)const {
		const ll g = gcd(q, n.q), a = q / g, b = n.q / g;
		return p * b > n.p * a;
	}
	bool operator<(const RationalNumber& n)const {
		const ll g = gcd(q, n.q), a = q / g, b = n.q / g;
		return p * b < n.p * a;
	}
	bool operator>=(const RationalNumber& n)const {
		const ll g = gcd(q, n.q), a = q / g, b = n.q / g;
		return p * b > n.p * a;
	}
	bool operator<=(const RationalNumber& n)const {
		const ll g = gcd(q, n.q), a = q / g, b = n.q / g;
		return p * b < n.p * a;
	}
	RationalNumber operator+(const RationalNumber& n)const {
		const ll g = gcd(q, n.q), a = q / g, b = n.q / g;
		return RationalNumber(p*b+n.p*a,a*n.q);
	}
	RationalNumber operator-()const {
		return RationalNumber(-p,q);
	}
	RationalNumber operator-(const RationalNumber& n)const {
		const ll g = gcd(q, n.q), a = q / g, b = n.q / g;
		return RationalNumber(p * b - n.p * a, a * n.q);
	}
	RationalNumber operator*(const RationalNumber& n)const {
		return RationalNumber(p * n.p, q * n.q);
	}
	RationalNumber reverse()const {
		RationalNumber ret; ret.q = ull(p);
		if (p < 0)ret.p = -ll(q);
		else ret.p = ll(q);
		return ret;
	}
	RationalNumber operator/(const RationalNumber& n)const {
		return *this/n.reverse();
	}
	RationalNumber operator^(int pow)const {
		if(pow<0)return reverse()^(-pow);
		else return RationalNumber(fastpow(p,pow),fastpow(q,pow));
	}
	void operator+=(const RationalNumber& n) {
		const ll g = gcd(q, n.q), a = q / g, b = n.q / g;
		p = p * b + n.p * a; q = a * n.q;
	}
	void operator-=(const RationalNumber& n) {
		const ll g = gcd(q, n.q), a = q / g, b = n.q / g;
		p = p * b - n.p * a; q = a * n.q;
	}
	void operator*=(const RationalNumber& n) {
		p*=n.p; q*=n.q;
	}
	void operator/=(const RationalNumber& n) {
		*this *= n.reverse();
	}
	void operator^=(int pow) {
		if (pow < 0) {
			*this = reverse();
			fastpow(p, -pow), fastpow(q, -pow);
		}
		else {
			p=fastpow(p, pow), q=fastpow(q, pow);
		}
	}
	static ll gcd(ll x, ll y)
	{
		if (y)return gcd(y, x % y);
		else return x;
	}
	static ll lcm(ll x, ll y) {
		return x/ gcd(x, y)*y;
	}
	template<typename T1,typename T2>
	static T1 fastpow(T1 base,T2 pow) {
		T1 ans = 1;
		while (pow) {
			if (pow & 1)ans *= base;
			pow >>= 1;
			base*=base;
		}
		return ans;
	}
	double toDouble()const {
		return double(p) / double(q);
	}
	static RationalNumber infinity() {
		return RationalNumber(1,0);
	}
private:
	ll p = 0ll;
	ull q = 0ull;
	void simplify() {
		ll g = gcd(p, q);
		p /= g; q /= g;
	}
};

class AlgebraExpression {
public:
	typedef std::function<AlgebraExpression(const vector<AlgebraExpression>&)> AlgebraFunction;
	struct AlgebraEnvironment {
		unordered_map<string, AlgebraFunction> env;
		unordered_map<string, bool> symbols;
		AlgebraEnvironment();
	};
	string f;
	vector<AlgebraExpression> children;
	static AlgebraEnvironment env;
	RationalNumber value;
	AlgebraExpression() {}
	AlgebraExpression(const RationalNumber& value):value(value) {}
	AlgebraExpression(double value) :value(value) {}
	AlgebraExpression(const string& expr) {
		Expression* e=nullptr;
		try { e = new Expression(expr); }
		catch(...){
			LOGGER.error("Unsupported AlgebraExpression: "+expr);
			return;
		}
		init(*e);
	}
	AlgebraExpression(const string& f,const vector<AlgebraExpression>& children)
		:f(f),children(children) {

	}
	AlgebraExpression operator()()const {
		if (env.env[f]) {
			vector<AlgebraExpression> c(children.size());
			for (uint i = 0; i < children.size(); i++)
				c[i] = children[i]();
			return getFunction(f)(c);
		}
		else return *this;
	}
	string serialize()const {
		if (!f.size())return toString(value);
		if (!children.size())return f;
		string ret;
		if (Expression::Operator(f)) {
			int p = Expression::Priority(f);
			for (uint i = 0; i < children.size();i++) {
				string tmp = children[i].serialize();
				int q = Expression::Priority(children[i].f);
				if ((i == 0 && q > p)||(i>0&&q>=p)) {
					ret.push_back('('); tmp.push_back(')');
				}
				ret += tmp;
				if (i < children.size() - 1)ret += f;
			}
			return ret;
		}
		ret += f+"(";
		for (uint i = 0; i < children.size(); i++) {
			ret += children[i].serialize();
			if (i != children.size() - 1)ret.push_back(',');
		}
		return ret + ")";
	}
	string functional()const {
		if (!f.size()) {
			ll p = value.numerator(), q = value.denominator();
			if (q <= 1) return value.serialize();
			string ret;
			if (value.recurring())ret += toString(float(p)) + "/" + toString(float(q));
			else ret += toString(value.toDouble());;
			return ret;
		}
		if (!children.size())return f;
		string ret,n=f;
		if (n == "^")n = "pow";
		if (Expression::Operator(n)) {
			int p = Expression::Priority(n);
			for (uint i = 0; i < children.size(); i++) {
				string tmp = children[i].serialize();
				int q = Expression::Priority(children[i].f);
				if ((i == 0 && q > p) || (i > 0 && q >= p)) {
					ret.push_back('('); tmp.push_back(')');
				}
				ret += tmp;
				if (i < children.size() - 1)ret += n;
			}
			return ret;
		}
		ret += n + "(";
		for (uint i = 0; i < children.size(); i++) {
			ret += children[i].serialize();
			if (i != children.size() - 1)ret.push_back(',');
		}
		return ret + ")";
	}
	friend ostream& operator<< (ostream& out, const AlgebraExpression& expr) {
		out << expr.serialize();
		return out;
	}
	AlgebraExpression operator+(const AlgebraExpression& expr)const {
		AlgebraExpression ret;
		if (f == "+")
			ret.children.insert(ret.children.end(), children.begin(), children.end());
		else ret.children.push_back(*this);
		if (expr.f == "+")
			ret.children.insert(ret.children.end(), expr.children.begin(), expr.children.end());
		else ret.children.push_back(expr);
		return ret;
	}
	AlgebraExpression operator-()const {
		if(f!="*")return AlgebraExpression("*", { -1.0, *this });
		AlgebraExpression ret; ret.f = "*";
		ret.children.push_back(-1);
		ret.children.insert(ret.children.end(), children.begin(), children.end());
		return ret;
	}
	bool isValue()const {
		if (!f.size())return true;
		for (uint i = 0; i < children.size(); i++)
			if (!children[i].isValue())return false;
		return true;
	}
	bool isSymbol()const {
		return env.symbols[f];
	}
	ull priority()const {
		return (ull) & (env.env.find(f)->second);
	}
	AlgebraExpression coefficient()const {
		if (isValue())
			return *this;
		else if (f == "*")return children[0].value;
		else return 1;
	}
	static AlgebraExpression Symbol(const string& symbol) {
		env.symbols[symbol] = true;
		env.env[symbol] = 0;
		return AlgebraExpression(symbol);
	}
	static AlgebraExpression Symbol(const string& symbol,const AlgebraExpression& value) {
		env.symbols[symbol] = true;
		setValue(symbol, value);
		return AlgebraExpression(symbol);
	}
	static void setValue(const string& name, const AlgebraExpression& value) {
		env.env[name] = AlgebraFunction(
			[=](const vector<AlgebraExpression>&)
			{return value; });
	}
	static bool find(const string& name) {
		return env.env.find(name) != env.env.end();
	}
	static void Function(const string& name, AlgebraFunction&& f) {
		env.env[name] = f;
	}
	static AlgebraFunction& getFunction(const string& name) {
		return env.env[name];
	}
	static AlgebraExpression getValue(const string& name) {
		return env.env[name]({});
	}
	static bool isSymbol(const string& name) {
		return env.symbols[name];
	}
	static void remove(const string& name) {
		env.env.erase(name);
	}
	static unordered_map<string, bool>& symbols() {
		return env.symbols;
	}
	static void clearEnvironment() {
		env = AlgebraEnvironment();
	}
private:
	void init(const Expression& expr);
};

typedef AlgebraExpression::AlgebraEnvironment AlgebraEnvironment;


struct Monomial {
	AlgebraExpression coefficient;
	vector<AlgebraExpression> mul;
	Monomial() {}
	Monomial(const AlgebraExpression& expr) {
		//cout << "monomial " << expr << endl;
		expand(expr);
		if (!coefficient.children.size())
			coefficient = 1;
		else if (coefficient.children.size() == 1)
			coefficient = coefficient.children[0];
		else coefficient.f = "*";
	}
	void init(const AlgebraExpression& expr) {
		mul = expr.children;
		coefficient = expr.children[0];
		mul.erase(mul.begin());
	}
	AlgebraExpression toExpression()const {
		if (!mul.size())return coefficient;
		AlgebraExpression ret;
		ret.f = "*"; 
		if(coefficient.value!=1)ret.children.push_back(coefficient);
		ret.children.insert(ret.children.end(), mul.begin(), mul.end());
		//cout << "mout " << ret << endl;
		if (ret.children.size() == 1)return ret.children[0];
		return ret;
	}

	void expand(const AlgebraExpression& expr) {
		if (expr.f == "*") {
			for (uint i = 0; i < expr.children.size(); i++)expand(expr.children[i]);
		}
		else if (expr.f == "-"&&expr.children.size()==1) {
			coefficient.children.insert(expr.children.begin(),-1);
			expand(expr.children[0]);
		}
		else {
			if (expr.isValue())coefficient.children.push_back(expr);
			else mul.push_back(expr);
		}
	}
};


struct Polymomial {
	vector<AlgebraExpression> terms;
	Polymomial() {}
	Polymomial(const AlgebraExpression& expr) {
		expand(expr);
	}
	void expand(const AlgebraExpression& expr) {
		if (expr.f == "+") 
			for (uint i = 0; i < expr.children.size(); i++)expand(expr.children[i]);
		else if (expr.f == "-" && expr.children.size() == 2) {
			expand(expr.children[0]);
			expand(-expr.children[1]);
		}
		else terms.push_back(expr);
	}
	AlgebraExpression toExpression()const {
		if (terms.size() == 1)return terms[0];
		return AlgebraExpression("+",terms);
	}
};