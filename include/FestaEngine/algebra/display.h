#pragma once

#include "algebra.h"
#include "../../frame.h"
#include "../../Font.h"

struct Function2D {
	typedef std::function<float(const vector<uint>&)> Function_t;
	struct Functions {
		float x = 0.0f, y = 0.0f;
		vector<Function2D> f;
		unordered_map<string, Function_t> fmap;
		Functions();
	};
	float value=0.0f;
	vector<uint> children;
	Function_t f=0;
	string name;
	static Functions functions;
	Function2D() {}
	Function2D(const AlgebraExpression& expr);
	float operator()() {
		if (f)return value = f(children);
		else return value;
	}
	static void input(float x, float y) {
		functions.x = x; functions.y = y;
	}
};

class AlgebraImageGenerator {
public:
	FreetypeFont font;
	vec2 border;
	AlgebraImageGenerator() {}
	AlgebraImageGenerator(const string& fontfile,const vec2& border = vec2(0.0f))
		:border(border){
		font.init(fontfile);
	}
	void clear() {
		commands.clear();
		box = Box();
	}
	Image* generate() {
		Image* ret = new Image();
		int x = int(border.x), y = int(border.y);
		ret->mat = cv::Mat(box.h + y * 2, box.w + x * 2, CV_8UC3, cv::Scalar(255, 255, 255));
		int line = box.top - box.y + y;
		for (Command& cmd : commands)cmd.render(font, ret, line);
		clear();
		return ret;
	}
	void put(Image* img,int x,int y) {
		x += int(border.x), y += int(border.y);
		int line = box.top - box.y + y;
		for (Command& cmd : commands)cmd.render(font, img, line);
		clear();
	}
	void add(const AlgebraExpression& expr,int fontsize) {
		int x = int(border.x)+box.w;
		box.update(generateCommands(expr, fontsize, x, 0));
	}
	void add(const string& expr,int fontsize) {
		int x = int(border.x)+box.w;
		addCommand(expr,fontsize,x,0,box);
	}
private:
	typedef FreetypeFont::Character Char;
	struct Command {
		vector<Char> characters;
		int fontsize = 0;
		int x = 0, y = 0, w = 0, h = 0, top = 0;
		Command() {}
		Command(FreetypeFont& font, const string& text, int fontsize, int x, int y)
			:fontsize(fontsize), x(x), y(y), w(0), h(0), top(0) {
			wstring expr = string2wstr(text);
			font.setSize(fontsize);
			for (uint i = 0; i < expr.size(); i++)
				add(font.loadChar(expr[i]));
			//cout << "new cmd " << text << "|"<<fontsize<<"," << w << "," << h << "," << top << endl;
		}
		void add(Char& ch) {
			//cout << "ch " << ch.w() << "," << ch.h() << endl;
			w += ch.advance;
			h = max(h, ch.h());
			top = max(top, ch.top);
			characters.push_back(ch);
		}
		void render(FreetypeFont& font, Image* img, int line) {
			int x0 = x;
			for (Char& ch : characters) {
				font.putChar(img, ch, vec2(x0, line + y));
				x0 += ch.advance;
			}
		}
	};
	struct Box {
		int x = 0, y = 0, w = 0, h = 0, top = 0;
		Box() {}
		Box(const Command& cmd)
			:x(cmd.x), y(cmd.y), w(cmd.w), h(cmd.h), top(cmd.top) {

		}
		Box(int x, int y)
			:x(x), y(y) {
		}
		void update(const Box& box) {
			w = max(w, box.x - x + box.w);
			top = max(top - y, box.top - box.y) + y;
			int b0 = bottom(), b1 = box.bottom();
			h = top + max(y + b0, box.y + b1) - y;
		}
		int bottom()const {
			return h - top;
		}
	};
	Box box;
	vector<Command> commands;
	void addCommand(const string& str, int fontsize, int x, int y, Box& box) {
		commands.push_back(Command(font, str, fontsize, x, y));
		box.update(commands.back());
	}
	Box generateCommands(const AlgebraExpression& expr, int fontsize, int x, int y) {
		Box ret(x, y);
		//cout << "gen " << expr << "," << expr.f << endl;
		if (!expr.children.size() && !expr.f.size())
			return rationalNumber(expr.value, fontsize, x, y);
		if (!Expression::Operator(expr.f)) {
			ret = symbol(expr.f, fontsize, x, y);
			if (expr.isSymbol())return ret;
			addCommand("(", fontsize, x + ret.w, y, ret);
			for (uint i = 0; i < expr.children.size(); i++)
				ret.update(generateCommands(expr.children[i], fontsize, x+ret.w, y));
			addCommand(")", fontsize, x + ret.w, y, ret);
			return ret;
		}
		if (expr.f == "+") {
			for (uint i = 0; i < expr.children.size(); i++) {
				ret.update(generateCommands(expr.children[i], fontsize, x + ret.w, y));
				if (i < expr.children.size() - 1)addCommand("+", fontsize, x + ret.w, y, ret);
			}
			return ret;
		}
		else if (expr.f == "*") {
			for (uint i = 0; i < expr.children.size(); i++) {
				ret.update(generateCommands(expr.children[i], fontsize, x + ret.w, y));
				//if (i < expr.children.size() - 1)addCommand("¡¤", fontsize, x + ret.w, y, ret);
			}
			return ret;
		}
		else if (expr.f == "/") {
			return fraction(expr.children[0],expr.children[1],fontsize,x,y);
		}
		else if (expr.f == "^") {
			return power(expr.children[0], expr.children[1], fontsize, x, y);
		}
		else {
			ret.update(generateCommands(expr.children[0], fontsize, x + ret.w, y));
			addCommand(expr.f, fontsize, x+ret.w, y, ret);
			ret.update(generateCommands(expr.children[1], fontsize, x + ret.w, y));
		}
		//cout << "gen " <<expr<<"|"<< ret.w << "," << ret.h() << endl;
		return ret;
	}
	Box symbol(const string& expr, int fontsize, int x, int y) {
		uint pos = uint(expr.find('_'));
		Box box(x, y);
		if (pos >= expr.size())addCommand(expr, fontsize, x, y, box);
		else {
			addCommand(stringIndex(expr, 0, pos), fontsize, x, y, box);
			box.update(symbol(stringIndex(expr, pos + 1, expr.size()),
				int(fontsize * 0.4), x + box.w, y + box.bottom()));
		}
		return box;
	}
	Box power(const AlgebraExpression& p, const AlgebraExpression& q, int fontsize, int x, int y) {
		Box box=generateCommands(p,fontsize,x,y);
		Box b=generateCommands(q, int(fontsize * 0.4), x + box.w, y - box.top);
		b.y -= b.bottom();
		box.update(b);
		return box;
	}
	Box fraction(const AlgebraExpression& p, const AlgebraExpression& q, int fontsize, int x, int y) {
		fontsize = int(fontsize * 0.8);
		uint i = uint(commands.size());
		Box bp = generateCommands(p, fontsize, x, 0);
		uint j = uint(commands.size());
		Box bq = generateCommands(q, fontsize, x, 0);
		int off = font.loadChar('I').h() / 2;
		int len = max(bp.w, bq.w), st = font.loadChar('_').w(), size = 0;
		Box line(x, y - off); string tmp;
		while (size < len) {
			size += st;
			tmp.push_back('_');
		}
		addCommand(tmp, fontsize, x, y - off, line);
		//line.y += line.bottom(); //line.top = line.h/2;
		//cout << "line " << line.top << "," << line.bottom() << endl;
		int yc = commands.back().y;
		uint k = i;
		for (; k < j; k++) {
			commands[k].x += (line.w - bp.w) / 2;
			commands[k].y += yc - line.top - bp.bottom();
		}
		cout << yc + bq.top << endl;
		for (; k < commands.size() - 1; k++) {
			commands[k].x += (line.w - bq.w) / 2;
			commands[k].y += yc + line.bottom() + bq.top;
		}
		bp.y += yc - line.top;
		bq.y += yc + line.bottom() + bq.top;
		line.update(bp); line.update(bq);
		//cout << "asd " << line.h<<","<<line.top << endl;
		return line;
	}
	Box rationalNumber(const RationalNumber& n, int fontsize, int x, int y) {
		Box box(x, y);
		ll p = n.numerator(), q = n.denominator();
		if (q <= 1) addCommand(n.serialize(), fontsize, x, y, box);
		else {
			if (p < 0) addCommand("-", fontsize, x, y, box);
			if (n.recurring())box.update(fraction(RationalNumber(p), RationalNumber(q), fontsize, x, y));
			else addCommand(toString(n.toDouble()), fontsize, x + box.w, y, box);
		}
		return box;
	}
};



class FunctionGraph2D {
public:
	struct Function {
		Program* p=0;
		vec3 color;
	};
	float blockMinWidth=0.0,blockMaxWidth=0.0,oriBlockWidth=0.0;
	uint numSmallBlocks = 5;
	vec2 ori=vec2(0.0f); float scaling = 1.0f;
	bool gpuMode=false;
	Window* window=0;
	Texture* gridTexture=0;
	FBO fbo; Texture* graph = 0;
	vector<Function> functions;
	unordered_map<string, float> variables;
	static ShaderSource VS, GRID_FS, GRAPH_FS;
	static ProgramSource GRID_PROGRAM, GRAPH_PROGRAM;
	FunctionGraph2D() {}
	FunctionGraph2D(Window* window,float blockMaxWidth,float blockMinWidth,float oriBlockWidth,uint numSmallBlocks=5)
		:blockMaxWidth(blockMaxWidth),blockMinWidth(blockMinWidth), oriBlockWidth(oriBlockWidth),
		numSmallBlocks(numSmallBlocks),window(window){
		ull w = window->viewport.w, h = window->viewport.h;
		//fbo.init();
	}
	~FunctionGraph2D() {
		for (Function& f : functions)SafeDelete(f.p);
	}
	void generate() {
		float width = oriBlockWidth * scaling, f = 1.0f;
		while (true) {
			if (width * f > blockMaxWidth)f *= 0.5f;
			else if (width * f < blockMinWidth)f *= 2.0f;
			else break;
		}
		int sw = int(width * f / numSmallBlocks);
		int bw = sw * numSmallBlocks;
		int w = window->viewport.w, h = window->viewport.h;
		//fbo.init(w, h);
		//graph.ptr = new _Texture(fbo);
		//fbo.bind();
		//fbo.unbind();
		if (gpuMode) {
			Program& p = GRID_PROGRAM.get();
			p.bind();
			p.setVec2("size", vec2(w, h));
			p.setVec2("ori", ori * vec2(w, -h));
			p.setInt("bw", bw);
			p.setInt("sw", sw);
			RECT2.get()->draw();
		}
		else if (gridTexture)drawTexture(*gridTexture);

		//renderGraphGPU(w,h,f);
		for (Function& func: functions) {
			if (!func.p)continue;
			func.p->bind();
			func.p->setVec2("INP.size", vec2(w, h)/float(bw)*f);
			func.p->setVec2("INP.ori", ori * vec2(w, -h) / float(bw)*f);
			func.p->setVec3("INP.color", func.color);
			func.p->setFloat("INP.f", f);
			func.p->setFloat("INP.eps", 0.35f);
			for (auto& i : variables) {
				//cout << "set " << i.first << "=" << i.second;
				func.p->setFloat(i.first, i.second);
			}
			RECT2.get()->draw();
		}
		//FBO::unbind();
	}
	void render() {
		generate();
		//if (graph)drawTexture(graph);
		if (fbo.ID()) {
			drawTexture(Texture(fbo));
		}
	}
	void generateGridTexture(FreetypeFont& font,uint lineWidth=1) {
		float width = oriBlockWidth * scaling, f = 1.0f;
		while (true) {
			if (width * f > blockMaxWidth)f *= 0.5f;
			else if (width * f < blockMinWidth)f *= 2.0f;
			else break;
		}
		uint sw = uint(width * f / numSmallBlocks);
		uint bw = sw * numSmallBlocks;
		int w = window->viewport.w, h = window->viewport.h;
		Image img(vec3(1.0f), w, h);
		for (int x = 0; x < w; x++) {
			if (x < 0 || x >= w)break;
			for (int y = 0; y < h; y++) {
				if (y<0 || y>h)break;
				uint dx = abs(int(ori.x*w +w / 2.0f) - x), dy = abs(int(ori.y*h+h / 2.0f) - y);
				uchar c;
				if (dx <= lineWidth || dy <= lineWidth)c = 120;
				else if (dx % bw <= lineWidth || dy % bw <= lineWidth)c = 200;
				else if (dx % sw <= lineWidth || dy % sw <= lineWidth)c = 240;
				else continue;
				img.get(x, y, 0) = c;
				img.get(x, y, 1) = c;
				img.get(x, y, 2) = c;
			}
		}
		font.setSize(30);
		for (int i = 1;; i++) {
			int x = int(ori.x * w + w / 2.0f) + i*bw, y = int(ori.y * h + h / 2.0f);
			if (x >= w)break;
			string text = toString(i*f);
			FreetypeFont::Box box = font.getBox(text);
			x -= box.w / 2; y += box.top;
			font.putText(&img,text,vec2(x,y));
		}
		for (int i = -1;; i--) {
			int x = int(ori.x * w + w / 2.0f) + i * bw, y = int(ori.y * h + h / 2.0f);
			if (x < 0)break;
			string text = toString(i * f);
			FreetypeFont::Box box = font.getBox(text);
			x -= box.w / 2; y += box.top;
			font.putText(&img, text, vec2(x, y));
		}
		for (int i = 1;; i++) {
			int x = int(ori.x * w + w / 2.0f), y = int(ori.y * h + h / 2.0f)-i*bw;
			if (y < 0 )break;
			string text = toString(i * f);
			FreetypeFont::Box box = font.getBox(text);
			y = y - box.bottom() + box.h / 2;
			font.putText(&img, text, vec2(x, y));
		}
		for (int i = -1;; i--) {
			int x = int(ori.x * w + w / 2.0f), y = int(ori.y * h + h / 2.0f) - i * bw;
			if (y >= h)break;
			string text = toString(i * f);
			FreetypeFont::Box box = font.getBox(text);
			y=y-box.bottom()+box.h/2;
			font.putText(&img, text, vec2(x, y));
		}
		font.setSize(50);
		int x = int(ori.x * w + w / 2.0f), y = int(ori.y * h + h / 2.0f);
		FreetypeFont::Box box = font.getBox("O");
		x -= box.w; y += box.top;
		font.putText(&img, "O", vec2(x, y));

		SafeDelete(gridTexture);
		gridTexture = new Texture(img);

	}
	Program* graphProgram(const string& left,const string& right) {
		string uniform;
		for (auto& i : variables)
			uniform += "uniform float "+i.first+";";
		Shader s(GL_FRAGMENT_SHADER,
			"#version 330 core\n"
			"struct INPUT{vec2 size;vec2 ori;vec3 color;float f;float eps;};"
			"uniform INPUT INP;"+uniform+
			"in vec2 pos;"
			"out vec4 FragColor;"
			"bool eq(float x,float y){return (pow(abs(x-y)/INP.f,0.3f)<INP.eps);};"

			"void main(){"
			"	vec2 coord=pos*INP.size*0.5f-INP.ori;"
			"	float x=coord.x;float y=coord.y;"
			"	if(eq(" + left + "," + right + "))FragColor=vec4(INP.color,1.0f);else FragColor=vec4(0.0f);}");
			return new Program({VS.get().id,s.id});
	}
	void scale(float x) {
		scaling *= x;
	}
	void translate(const vec2& pos) {
		ori += pos/vec2(window->viewport.w,window->viewport.h);
	}
	void mode(bool gpu) {
		gpuMode = gpu;
	}
	bool isOnGPU()const {
		return gpuMode;
	}
};