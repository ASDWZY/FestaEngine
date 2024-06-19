#include "../common/common.h"
namespace Festa {
	namespace CodeAnalysis {
		enum ExpressionType {
			BACKGROUND,
			LINE_NUMBER,
			CURSOR,
			SELECTION,
			ERRORMARKER,

			TEXT,
			NUMBER,
			PUNCTUATION,
			STRING,

			COMMENT,
			KEYWORD,
			TYPENAME,
			MACRO,

			VAR_DEF,
			VAR_USAGE,
			FUNCTION_DEF,
			FUNCTION_USAGE,

			TYPE_MAX
		};
		inline bool isNumber(char x) {
			return '0' <= x && x <= '9';
		}
		inline bool isLetter(char x) {
			return ('a' <= x && x <= 'z') || ('A' <= x && x <= 'Z') || x == '_';
		}
		inline bool isDelimiter(char x) {
			return x == ' ' || x == '\n' || x == '\t';
		}
		inline bool isInName(char x) {
			return isNumber(x) || isLetter(x);
		}
		inline char matchToken(char x) {
			switch (x) {
			case '(':
				return ')';
			case '[':
				return ']';
			case '{':
				return '}';
			case '\'':
				return '\'';
			case '\"':
				return '\"';
			default:
				return 0;
			}
		}
		struct Message {
			ivec2 begin=ivec2(0), end=ivec2(0);
			std::string msg;
			Message() {}
			Message(const ivec2& _begin, const ivec2& _end, const std::string& _msg)
				:begin(_begin), end(_end), msg(_msg) {}
			bool empty()const {
				return !msg.size();
			}
		};
#define container_t std::unordered_map
		struct Namespace {
			enum Attrib {
				CONSTANT,
				STATIC,
				ATTRIB_MAX
			};
			enum ClassSpace {
				SPACE_STATIC,
				SPACE_PUBLIC,
				SPACE_PRIVATE,
				SPACE_PROTECTED
			};
			struct Type {
				std::string name;
				Type* ptr = 0;
				Type* ref = 0;
				BoolList attributes;
				Type() {
					attributes.resize(ATTRIB_MAX);
				}
			};
			struct VarDef {
				std::string expr;
				Type type;
			};
			struct FunctionDef {
				std::string expr;
				Type ret;
				BoolList attributes;
				container_t<std::string,VarDef> params;
				FunctionDef() {
					attributes.resize(ATTRIB_MAX);
				}
			};
			struct ClassDef {
				std::string expr;
				std::vector<Namespace> spaces;
				ClassDef() {
					spaces.resize(4);
				}
			};
			container_t<std::string,VarDef> vars;
			container_t<std::string,FunctionDef> functions;
			container_t<std::string,ClassDef> classes;
			container_t<std::string, Namespace> namespaces;

			Namespace() {}
			void init() {
				ClassDef& ptr=addType("*","");

			}
			ClassDef& addType(const std::string& name,const std::string& expr) {
				ClassDef& ret = classes[name];
				ret.expr = expr;
				return ret;
			}
			void pushVar(const std::string& name,const VarDef& var) {
				vars[name] = var;
			}
			void popVar(const std::string& name,bool erase=true) {
				if(erase)vars.erase(name);
			}
			void pushFunction(const std::string& name, const FunctionDef& func) {
				functions[name] = func;
				for (auto i = func.params.begin(); i!= func.params.end(); i++)
					pushVar(i->first, i->second);
			}
			void popFunction(const std::string& name,bool erase=true) {
				FunctionDef& func = functions[name];
				for (auto i = func.params.begin(); i!= func.params.end(); i++)
					pushVar(i->first, i->second);
				if(erase)functions.erase(name);
			}
			void pushNamespace(const std::string& name, const Namespace& space) {
				namespaces[name] = space;
				for (auto i = space.vars.begin(); i != space.vars.end(); i++)
					pushVar(i->first, i->second);
				for (auto i = space.functions.begin(); i != space.functions.end(); i++)
					pushFunction(i->first, i->second);
				for (auto i = space.classes.begin(); i != space.classes.end(); i++)
					pushClass(i->first, i->second);
				for (auto i = space.namespaces.begin(); i != space.namespaces.end(); i++)
					pushNamespace(i->first, i->second);
			}
			void popNamespace(const std::string& name,bool erase=true) {
				Namespace& space = namespaces[name];
				for (auto i = space.vars.begin(); i != space.vars.end(); i++)
					popVar(i->first);
				for (auto i = space.functions.begin(); i != space.functions.end(); i++)
					popFunction(i->first);
				for (auto i = space.classes.begin(); i != space.classes.end(); i++)
					popClass(i->first);
				for (auto i = space.namespaces.begin(); i != space.namespaces.end(); i++)
					popNamespace(i->first);
				if (erase)namespaces.erase(name);
			}
			void pushClass(const std::string& name, const ClassDef& cls) {
				classes[name] = cls;
				for(uint i=0;i<cls.spaces.size();i++)
					pushNamespace(name + "::"+toString(i), cls.spaces[i]);
			}
			void popClass(const std::string& name,bool erase=true) {
				ClassDef& cls = classes[name];
				for (uint i = 0; i < cls.spaces.size(); i++)
					popNamespace(name + "::" + toString(i));
				if (erase)classes.erase(name);
			}
			void clear() {
				vars.clear();
				functions.clear();
				classes.clear();
				namespaces.clear();
			}
		};

		typedef Namespace CppFileSource;
		class CppExpression {
		public:
			Message error;
			std::string str;
			std::list<CppExpression> params;
			CppExpression() {}
			void parse(const std::string& expr,CppFileSource &src) {

			}
			Namespace::Type check(CppFileSource& src) {
				return Namespace::Type();
			}
			std::string serialize()const {
				std::string ret;
				return ret;
			}

		};
		class CppAnalysis {
		public:
			std::list<Message>* errors=0;
			struct Grammar {
				Grammar() {
					
				}
			};
			static Grammar grammar;
			std::unordered_map<std::string, std::string> macros;// macro processing(struct marco:func)
			ull size=0,i=0, lineStart = 0;
			uint line = 0;
			bool lineEmpty = true;
			std::string tmp;

			Namespace* current=0;
			int spacename = -1;

			CppAnalysis() {}
			CppAnalysis(std::list<Message>& _errors) :errors(&_errors) {}
			void parse(const std::string& str, CppFileSource& source) {
				current = &source;
				size = str.size();
				char comment = 0;
				for (i = 0; i < size; i++) {
					char ch = str[i];
					if (i + 1 < size) {
						if (!comment && ch == '/' && (str[i + 1] == '/' || str[i + 1] == '*'))
							comment = str[i + 1];
						else if (comment!='/'&&ch == '*' && str[i + 1] == '/') {
							if (!comment) {
								ivec2 pos = getPos();
								addError("Unmatched token \"*/\"", pos, ivec2(pos.x+2,pos.y));
							}
						}
					}
					if (ch == '\n') {
						newLine();
						if (comment == '/')comment = 0;
					}
					else if (ch == '#') {
						parsePrecommand(str);
					}
					else if (ch == ';') {
						//except in for / in macro
						parseExpr();
					}
					else if (ch == '{') {
						createSpace(source);
						parseExpr();
						cout << "create space " << spacename << endl;
					}
					else if (ch == '}') {
						if (!exitSpace(source)) {
							addError("Unmatched token \"}\"",
								getPos(),getPos());
						}
					}
					else if (tmp.size()||(ch != ' ' && ch != '\t')) {
						tmp.push_back(ch);
						lineEmpty = false;
					}
				}
				if (comment == '*')
					addError("Please input \"*/\"", getPos(), getPos());
			}
		private:
			ivec2 getPos() {
				return ivec2(i - lineStart, line);
			}
			void parsePrecommand(const std::string& str) {
				bool err = !lineEmpty;
				std::list<Message> params(1);
				params.front().begin = ivec2(i - lineStart, line);
				char isStr = 0;
				i++;
				while (i < size) {
					// process as expr
					bool space = str[i] == ' ' || str[i] == '\t';
					if (!space&&params.size() <= 2 && isInName(str[i - 1]) && !isInName(str[i])) {
						//cout << "new:" << str[i - 1] << ":" << str[i] << endl;
						params.back().end = getPos();
						params.push_back(Message());
						params.back().begin = getPos();
					}
					if (!isStr && (str[i] == '\'' || str[i] == '\"'||str[i]=='<'))
						isStr = str[i];
					if (str[i] == '\n') {
						params.back().end = getPos();
						newLine();
						if (i && str[i - 1] == '\\')params.back().msg.pop_back();
						else break;
					}
					else if (params.size() <= 2 && space) {
						params.back().end = getPos();
						params.push_back(Message());
						params.back().begin = getPos();
					}
					else params.back().msg.push_back(str[i]);

					if ((isStr == str[i] && (str[i] == '\'' || str[i] == '\"'))||
						(isStr=='<'&&str[i]=='>')) {
						while (params.size()>2 && params.back().msg[0] != isStr) {
							Message back = params.back();
							params.pop_back();
							params.back().msg += back.msg;
							params.back().end = back.end;
						}
						isStr = 0;
					}
					i++;
				}
				if (err)
					addError("Cannot use \"#\" here", params.front().begin,
						params.front().begin);
				else
					precompile(params);
			}
			void parseExpr() {
				tmp.clear();
			}
			void createSpace(CppFileSource& source) {
				current = &source.namespaces[toString(++spacename)];
			}
			bool exitSpace(CppFileSource& source) {
				if (spacename == -1)return false;
				source.namespaces.erase(toString(spacename--));
				current = &source.namespaces[toString(spacename)];
				return true;
			}
			void precompile(std::list<Message>& params) {
				for (auto i : params)cout << i.msg << endl;
				Message cmd = params.front(); params.pop_front();
				if (!cmd.msg.size()) {
					addError("Please input preprocessing command",cmd.begin,cmd.end);
					return;
				}
				ivec2 end = params.back().end;
				if (cmd.msg == "define") {
					
					return;
				}
				else if (cmd.msg=="include") {
					uint isize = string("include").size();
					if (params.size()!=1||params.front().msg.size()<=2) {
						addError("Please input filename", params.front().begin,
							params.front().end);
						return;
					}
					std::string& file = params.front().msg;
					char c = file[0];
					if (c != '\"' && c != '<') {
						addError("Please input filename", params.front().begin, params.front().end);
						return;
					}
					else if ((c=='\"'&&file.back()!='\"')||
							(c=='<'&&file.back()!='>')) {
						addError("Unmatched token \""+toString(c)+"\"", params.front().begin, params.front().end);
						return;
					}
					cout << "include file: " << file << endl;
				}
				else {
					addError("Unknown preprocessing command: "+cmd.msg, cmd.begin, cmd.end);
				}
			}
			void newLine() {
				line++; lineStart = i+1; lineEmpty = true;
			}
			void addError(const string& msg, const ivec2& begin) {
				errors->emplace_back(Message({ begin,ivec2(i-lineStart,line),msg}));
			}
			void addError(const string& msg,const ivec2& begin,const ivec2& end) {
				errors->emplace_back(Message({begin,end,msg}));
			}
		};
		CppAnalysis::Grammar CppAnalysis::grammar;

		class CppSources {
		public:
			struct CppFile {
				CppFileSource src;
				std::list<Message> errors;
				std::set<uint> fathers;
				bool find(uint file)const {
					return fathers.find(file) != fathers.end();
				}
			};
			OrderedDict<CppFile> files;
			CppSources() {

			}
			CppFile& addFile(const std::string& path,const std::string& code) {
				if (files.find(path))return files[path];
				CppFile& file=files.insert(path, CppFile());
				parseFile(file, code);
				return file;
			}
			CppFile& addFile(const std::string& path) {
				if (files.find(path))return files[path];
				CppFile& file = files.insert(path, CppFile());
				std::string code; readString(path, code);
				parseFile(file, code);
				return file;
			}
			void includeFile(const std::string& father, const std::string& filename) {
				if (!files.find(father))return;
				addFile(filename).fathers.insert(files.index(father));
			}
			CppFile& applyChanges(const std::string& file) {
				if (!files.find(file))return addFile(file);
				std::string code; readString(file, code);
				CppFile& f = files[file];
				parseFile(f, code);
				for (uint father : f.fathers)
					applyChanges(files.key(father));
				return f;
			}
			void deleteFile(const std::string& path) {
				if (!files.find(path))return;

			}
			static void parseFile(CppFile& file, const std::string& code) {
				file.errors.clear();
				file.src.clear();
				CppAnalysis analysis(file.errors);
				analysis.parse(code,file.src);
			}
		};
	}
}