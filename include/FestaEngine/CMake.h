#include "../common/common.h"
#include <stdio.h>
using namespace Festa;
using namespace std;

#define KEY_CMAKE_VERSION "CMake minimum version"

string execCommand(const string& cmd){
	char buffer[1024] = { '\0'};
	FILE* p = _popen(cmd.c_str(), "r");
	if (!p){
		LOGGER.error("Failed to open the popen: "+cmd);
		return string();
	}

	std::string ret;
	while (fgets(buffer, sizeof(buffer), p))
		ret += buffer;
	_pclose(p);
	return ret;

}


class CommandStream {
public:
	queue<string> q;
	string cmd;
	atomic<bool> _end = false,_terminate=false;
	CommandStream() {}
	CommandStream(const string& _cmd):cmd(_cmd) {}
	void start0() {
		FILE* p = 0;
		//p = _popen((cmd + " 2>&1").c_str(), "r");
		cout << "start popen: " << cmd << endl;
		p = _popen(cmd.c_str(), "r");
		if (!p) {
			LOGGER.error("Failed to open the popen: " + cmd);
			_end = true;
			return;
		}
		char buffer[1024] = { '\0' };
		while (fgets(buffer, sizeof(buffer), p)) {
			q.push(buffer);
			if (_terminate)break;
		}
		_pclose(p);
		_end = true;
	}
	void start() {
		thread([&](){
			FILE* p = 0;
			p = _popen((cmd + " 2>&1").c_str(), "r");
			if (!p) {
				LOGGER.error("Failed to open the popen: " + cmd);
				_end = true;
				return;
			}
			char buffer[1024] = { '\0' };
			while (fgets(buffer, sizeof(buffer), p)) {
				//cout << "d\n";
				/*while (q.size()>1) {
					std::this_thread::sleep_for(chrono::milliseconds(1));
				}*/
				q.push(buffer);
				if (_terminate)break;
			}
			_pclose(p);
			_end = true;
			}).detach();
	}
	bool end()const {
		return _end;
	}
	void terminate() {
		if (end())return;
		_terminate = true;
		//while (!end())Sleep(1);
	}
	bool empty()const {
		return q.empty();
	}
	void pop(string& str) {
		if(!empty()){
			str=q.front();
			q.pop();
		}
	}
	void operator>>(string& str) {
		while (!empty()) {
			str+=q.front();
			q.pop();
		}
	}
};

namespace CMakeFiles {
	const string defaultFile = "src/configs/defaultProject.json";
	const Path buildDir = Path(".build");
	const Path runShell = Path(".run.bat");
}

class CMakeProject {
public:
	JsonData data;
	Path file;
	string compiler = "MinGW";
	queue<bool> building, compiling;
	queue<string> out;
	ThreadTimer timer;
	CMakeProject() {
		timer = ThreadTimer(0.0, [&] {
			build();
			compile();
			});
		//timer.start();
	}
	~CMakeProject() {
		timer.stop();
		build(); compile();
	}
	void load(const Path& _file) {
		file = _file;
		data.load(file);
		if (Version2(data["Project"]["Festa version"]) != FESTA_VERSION)
			LOGGER.warning("Different Festa version");
		checkMissingFiles();
	}
	void defaultConfig(const string& name) {
		data.load(CMakeFiles::defaultFile);
		projectName() = name;
		data["Project"]["Festa version"].data = FESTA_VERSION.toStr();
		JsonData& includes = data["Project"]["Include directories"];
		JsonData& sources = sourceFiles();
		includes.insert(FESTA_ROOT.toString(),1);
		includes.insert((FESTA_ROOT+Path("3rd")).toString(), 1);
		includes.insert((FESTA_ROOT + Path("3rd/freetype")).toString(),1);

		sources.insert((FESTA_ROOT+Path("src/Festa.cpp")).toString(),1);
		sources.insert((FESTA_ROOT + Path("src/imgui.cpp")).toString(), 1);
	}
	static void defaultConfig(JsonData& data,const string& name) {
		data.load(CMakeFiles::defaultFile);
		data["Project"]["Project name"].data = name;
		data["Project"]["Festa version"].data = FESTA_VERSION.toStr();
		JsonData& includes = data["Project"]["Include directories"];
		JsonData& sources = data["Project"]["Source files"];
		includes.insert(FESTA_ROOT.toString(), 1);
		includes.insert((FESTA_ROOT + Path("3rd")).toString(), 1);
		includes.insert((FESTA_ROOT + Path("3rd/freetype")).toString(), 1);

		sources.insert((FESTA_ROOT + Path("src/Festa.cpp")).toString(), 1);
		sources.insert((FESTA_ROOT + Path("src/imgui.cpp")).toString(), 1);
	}
	void create(const string& name,const Path& _file) {
		file = _file;
		defaultConfig(name);
		save();
		building.push(true);
		writeCMakeLists();
	}
	void save()const {
		writeString(file, data.serialize());
	}
	Path projectDir()const {
		return file.getDirectory();
	}
	string cmakeFunc(const string& f, const vector<string>& params) {
		string ret = f+"(";
		for (uint i = 0; i < params.size(); i++)ret += params[i] + " ";
		ret.back() = ')';
		return ret+"\n";
	}
	string cmakeFunc(const string& f, queue<string>& params) {
		string ret = f + "(";
		while (params.size()) {
			ret += params.front() + " ";
			params.pop();
		}
		ret.back() = ')';
		return ret + "\n";
	}
	string& projectName() {
		return data["Project"]["Project name"].data;
	}
	JsonData& sourceFiles() {
		return data["Project"]["Source files"];
	}
	string CMakeLists() {
		queue<string> q;
		string ret;
		ret += cmakeFunc("cmake_minimum_required", { "VERSION",data["Project"][KEY_CMAKE_VERSION] });
		ret += cmakeFunc("Project", {projectName()});
		ret += cmakeFunc("set", {"CMAKE_CXX_STANDARD",data["Project"]["stdc++"]});
		JsonData includes = data["Project"]["Include directories"];
		for (auto i : includes) {
			if (i.second.isString() || i.second.to<int>())
				q.push(i.first);
		}
		ret += cmakeFunc("include_directories", q);

		JsonData& sources = sourceFiles();
		q.push("${PROJECT_NAME}");
		for (auto i : sources) {
			if (i.second.isString() || i.second.to<int>())
				q.push(i.first);
		}
		ret += cmakeFunc("add_executable", q);
		
		vector<Path> libs;
		(FESTA_ROOT + Path("libs")).glob(libs,"*.lib");
		q.push("${PROJECT_NAME}");
		for (Path& p : libs)
			if (p.isFile())q.push(p.toString());
		ret += cmakeFunc("target_link_libraries", q);

		return ret;
	}
	void writeCMakeLists() {
		writeString(file.getDirectory()+Path("CMakeLists.txt"), CMakeLists());
	}
	bool buildable(bool warning) {
		if (!data.find("Project")) {
			if(warning)LOGGER.warning("Please create or load a project first");
			return false;
		}
		if (!sourceFiles().size()) {
			if (warning)LOGGER.warning("Please add your source files(.cpp .c)");
			return false;
		}
		return true;
	}
	void checkMissingFiles() {
		JsonData& s = sourceFiles();
		JsonData sources = s;
		for (auto i: s) {
			if (!Path(i.first).exists()) {
				sources.erase(i.first);
				building.push(true);
			}
		}
		s = sources;
	}
	CommandStream* build(bool warning) {
		Path bd = projectDir() + CMakeFiles::buildDir;
		if (!building.size()&&bd.exists())return 0;
		while (building.size())building.pop();
		if (!buildable(warning))return 0;
		cout << "building...\n";
		checkMissingFiles();
		if(!(file.getDirectory() + Path("CMakeLists.txt")).exists()||
			data["Project"]["Generate CMakeLists.txt"].to<int>())
			writeCMakeLists();
		bd.deleteDirectory();
		bd.createDirectory();
		string cmakePath = (FESTA_ROOT + Path("CMake/bin/cmake.exe")).toWindows();
		writeString(bd+ CMakeFiles::runShell,"@echo off\ncd "+bd.toString() + "\n"+
			cmakePath+" -G \""+compiler+" Makefiles\" ..");
		CommandStream* ret=new CommandStream((bd + CMakeFiles::runShell).toWindows());
		ret->start();
		return ret;
	}
	CommandStream* compile(bool warning) {
		if (!compiling.size())return 0;
		while (compiling.size())compiling.pop();
		if(!building.size()&&!(projectDir() + CMakeFiles::buildDir).exists())building.push(true);
		if (building.size()) {
			if(warning)LOGGER.warning("Please build your project first");
			return 0;
		}
		cout << "compiling...\n";
		Path bd = projectDir() + CMakeFiles::buildDir;
		writeString(bd + CMakeFiles::runShell, "@echo off\ncd " + bd.toString() + "\n"+
		"make");//(FESTA_ROOT+Path("mingw64/bin/make.exe")).toWindows()
		cout << "cmd:\n";
		CommandStream* ret = new CommandStream((bd + CMakeFiles::runShell).toWindows());
		ret->start();
		return ret;
	}
	CommandStream* run() {
		CommandStream* ret = new CommandStream((projectDir() + CMakeFiles::buildDir +
			Path(projectName() + ".exe")).toWindows());
		ret->start();
		return ret;
	}

	void build() {
		CommandStream* cmd=build(false);
		if (!cmd)return;
		while (!cmd->end())Sleep(1);
		SafeDelete(cmd);
	}

	void compile() {
		CommandStream* cmd = compile(false);
		if (!cmd)return;
		while (!cmd->end())Sleep(1);
		SafeDelete(cmd);
	}
};


