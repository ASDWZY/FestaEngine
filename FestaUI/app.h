#pragma once


#include "editor.h"


#define KEY_CLEAR_CONSOLE "Clear Console automatically"

inline ImGuiMode WindowView(const JsonData&config,const string& title) {
	return ImGuiMode(config[title]["Window View"].to<int>() - 1);
}

class Console :public BasePage {
public:
	CommandStream* stream=0;
	std::list<CommandStream*> terminated;
	ImGuiTextEditor editor;
	EditorAssistant assistant;
	Console() {
		editor.readOnly = true;
		editor.params = &Editors::params;
		assistant.init(editor);
	}
	~Console() {
		SafeDelete(stream);
		for (auto it = terminated.begin(); it != terminated.end(); it++) {
			CommandStream* ptr = *it;
			while (!ptr->end())Sleep(1);
			SafeDelete(ptr);
		}
	}
	void render() {
		if (ImGui::Button("Clear"))clear();
		ImGui::SameLine();
		if (ImGui::Button("Terminate") && stream) {
			terminated.push_back(stream);
			stream->terminate();
			stream = 0;
			editor.PushText<EditorAssistant>("\nTerminated\n");
		}
		if (stream) {
			while (!stream->empty()) {
				push(stream->q.front());
				stream->q.pop();
			}
		}
		if (stream && stream->end() && stream->empty()) {
			SafeDelete(stream);
			editor.PushText<EditorAssistant>("\nExited\n");
		}
		for (auto it = terminated.begin(); it != terminated.end(); it++) {
			CommandStream* ptr = *it;
			if (ptr->end()) {
				SafeDelete(ptr);
				terminated.erase(it);
				break;
			}
		}
		editor.Render<EditorAssistant>();
	}
	void clear() {
		editor.Clear();
	}
	void push(const string& text) {
		//cout << text;
		editor.PushText<EditorAssistant>(text);
	}
};

class MainPage {
public:
	struct Tab {
		ImGuiPage page;
		bool focus = false,releasePage=true;
		Path path;
		LeafType type = NOT_LEAF;

		~Tab() {}
		bool render(const string& label) {
			bool x=true;
			if (ImGui::BeginTabItem(string2u8(label).c_str(), &x, focus?ImGuiTabItemFlags_SetSelected:0)) {
				focus = false;
				if (ImGui::IsItemActivated())return x;
				page.render();
				ImGui::EndTabItem();
			}
			focus = false;
			return x;
		}
		void release() {
			page.release(releasePage);
		}
	};
	template<typename Wt>
	struct Page{
		Wt* window=0;
		ImGuiPage page;
		bool releasePage = false;
		~Page() {}
		bool render(bool focus=false) {
			if (!window)return false;
			if (!window->isActivated())
				return true;
			if (focus)ImGui::SetNextWindowFocus();
			window->begin();
			page.render();
			window->end();
			return false;
		}
		void release() {
			SafeDelete(window);
			page.release(releasePage);
		}
	};
	OrderedDict<Tab> tabs;
	vector<Page<DockedWindow>> dockedWindows;
	Page<IMGUIWindow> movableWindow;

	Console console;
	CMakeProject* project=0;
	JsonData* config=0;
	Window* father=0;
	Texture runButton;
	MainPage() {
		
		dockedWindows.resize(5);
	}
	~MainPage(){
		dockedWindows[IMGUI_FULLSCREEN].release();
		closeAllWindows();
	}
	void closeAllTabs() {
		for (auto i : tabs)i.second.release();
		tabs.clear();
	}
	void closeAllWindows() {
		for (uint i = 0; i < 4;i++)dockedWindows[i].release();
		movableWindow.release();
		closeAllTabs();
	}
	void openConsole(CommandStream* stream,const string& cmd) {
		setWindow(*father,"Console",WindowView(*config,"Console"),
			&console,false);
		if (stream) {
			console.stream = stream;
			if ((*config)["Console"][KEY_CLEAR_CONSOLE].to<int>())console.clear();
			console.push(">>>" + cmd+"\n");
		}
	}
	void init(Window& _father,CMakeProject* _project,JsonData& _config){
		project = _project; config = &_config; father = &_father;
		dockedWindows[IMGUI_FULLSCREEN].window = new DockedWindow(father, "##FULLSCREEN",
			IMGUI_FULLSCREEN, -1, false, ImGuiWindowFlags_NoCollapse| ImGuiWindowFlags_NoMove);
		Image img("src/icon/run.jpg");
		alphaImage(img);
		runButton.init(img);
	}
	bool render() {
		bool ret = false;
		for (int i = 0; i < 4; i++) {
			//dockedWindows[i].render();
			if(dockedWindows[i].render())dockedWindows[i].release();
		}
		Page<DockedWindow>& full = dockedWindows[IMGUI_FULLSCREEN];
		if (full.window) {
			full.window->begin();
			float s = ImGui::GetTextLineHeight();
			if (ImGui::ImageButton((void*)runButton.ID(), ImVec2(s, s))) {
				openConsole(project->run(),"Run");
			}
			ret = renderTabs();
			full.window->end();
		}
		//movableWindow.render(true);
		if(movableWindow.render(true))movableWindow.release();
		return ret;
	}
	bool renderTabs() {
		ImGui::BeginTabBar("##TABS");
		for (auto it : tabs) {
			if (!it.second.render(it.first)) {
				it.second.release();
				tabs.erase(it.first);
				ImGui::EndTabBar();
				return true;
			}
		}
		ImGui::EndTabBar();
		return false;
	}
	void setWindow(Window& father,const string& title,ImGuiMode mode,
		ImGuiPage page,bool releasePage) {//page: new
		if (mode == IMGUI_NONE) {
			if(movableWindow.window)movableWindow.release();
			movableWindow.window = new IMGUIWindow(&father, title);
			movableWindow.page = page;
			movableWindow.releasePage = releasePage;
		}
		else if (mode == IMGUI_FULLSCREEN) {
			Tab& tab = tabs.insert(title, Tab());
			tab.page = page;
			tab.focus = true;
			tab.releasePage = releasePage;
		}
		else {
			if (dockedWindows[mode].window)dockedWindows[mode].release();
			dockedWindows[mode].window = new DockedWindow(&father, 
				title, mode, -1, true, 
				ImGuiWindowFlags_NoMove| ImGuiWindowFlags_NoCollapse);
			dockedWindows[mode].page = page;
			dockedWindows[mode].releasePage = releasePage;
		}
	}
	void openFile(const Path& path, LeafType type, const Path& root) {
		const string key = (path - root).toString();
		if (tabs.find(key)) {
			tabs[key].focus = true;
			return;
		}
		Tab& tab = tabs.insert(key, Tab());
		tab.page=initPage(type, path);
		tab.focus = true;
		tab.releasePage = true;
		tab.path = path;
		tab.type = type;
	}
	void renameFile(const Path& oldPath, const Path& newPath, const Path& root) {
		const string oldKey = (oldPath - root).toString();
		Tab& tab = tabs[(newPath - root).toString()];
		tab = tabs[oldKey];
		tabs.erase(oldKey);
	}
	JsonData getFilesJson() {
		JsonData res=JsonData::Dict();
		for (auto i : tabs) {
			if (i.second.type != NOT_LEAF)
				res.insert(i.second.path.toString(), int(i.second.type));
		}
		return res;
	}
private:
	ImGuiPage initPage(LeafType type,const Path& path) {
		switch (type) {
		case LEAF_TEXT:
			return new TextEditor(path);
		case LEAF_SRC:
			return new CppEditor(path);
		case LEAF_PIC:
			return new PictureViewer(path);
		case LEAF_AUDIO:
			return new AudioPlayer(path);
		case LEAF_ASSET:
			return new AssetEditor(path);
		}
	}
};



namespace Files {
	const string configFile = "src/configs/config.json",
		formatFile = "src/configs/configPage.json",
		defaultConfig = "src/configs/default.json";
}

class App {
public:
	typedef std::function<void(App*)> PluginFunc;
	

	struct Plugin {
		void* ptr;
		PluginFunc init,run;
		template<typename T>
		T& plugin() {
			return *((T*)ptr);
		}
	};
	class BasePlugin {
	public:
		string name;

		BasePlugin() {}
		virtual void init(App& app) {

		}
		virtual void run(App& app) {

		}
		void reg(App& app) {
			app.registerPlugin(name, this,
				[this](App* app) {
					init(*app);
				},
				[this](App* app) {
					run(*app);
				}
			);
		}
	};
	class FestaConfig :public BasePage {
	public:
		JsonData copy, format,*data=0;
		JsonComponent comp;
		ImFont* uiFont = 0;
		bool _reinit = false;
		App* app=0;
		FestaConfig() {}
		void init(App& _app);
		void reinit();
		void save() {
			writeString(Files::configFile, data->serialize());
		}
		void render();

	};
	class Project {
	public:
		CMakeProject cmake;
		Project() {

		}
		void open(const Path& file, App& app) {
			app.loadProject(file,true);
		}
		void create(const string& name, const Path& dir, App& app) {
			dir.createDirectory();
			Path file = dir + Path(".project.fstproj");
			cmake.create(name, file);
			app.loadProject(file,false);
		}
		bool empty()const {
			return cmake.file.empty();
		}
		void build() {
			cmake.save();
			cmake.build(true);
		}
		void reset(App& app) {
			cmake.defaultConfig(cmake.projectName());
		}
		JsonData& data() {
			return cmake.data;
		}
	};
	//unordered_map<string, Plugin>plugins;
	OrderedDict<Plugin> plugins;
	Window window;
	FestaConfig configPage;
	Project project;
	MainPage mainPage;
	const string appFile = "src/configs/app.json";
	JsonData data,config;
	App() {
		window.init(1200, 900, "FestaEngine");
		window.maximize();
		window.initImgui();
		Image icon("src/icon/festa.jpg");
		alphaImage(icon);
		window.setIcon(icon);
	}
	void saveData() {
		project.data()["files"] = mainPage.getFilesJson();
		project.cmake.save();
		writeString(appFile, data.serialize());
	}
	void loadProject(const Path& file,bool loadFile) {
		mainPage.closeAllWindows();
		data["projects"].insert(file.toString(), "");
		data["project"] = file.toString();

		if(loadFile)project.cmake.load(file);
		for (auto i : project.data()["files"]) {
			if (!Path(i.first).exists())continue;
			mainPage.openFile(i.first, LeafType(int(i.second.to<int>())),projectDir());
		}

		saveData();
		for (auto t : plugins) {
			Plugin& plugin = t.second;
			//cout << "init plugin " << t.first << endl;
			plugin.init(this);
			//cout << "init done\n";
		}
	}
	void render() {
		ImGuiIO& io = ImGui::GetIO();
		configPage.reinit();
		if (configPage.uiFont)ImGui::PushFont(io.Fonts->Fonts[1]);
		for (auto t : plugins) {
			Plugin& plugin = t.second;
			//cout << "run plugin " << t.first << endl;
	 		plugin.run(this);
     		//cout << "done plugin " << t.first << endl;
		}
		if (mainPage.render())
			saveData();
		if (configPage.uiFont)ImGui::PopFont();
	}
	void run() {
		config.load(Files::configFile);
		configPage.init(*this);

		data.load(appFile);
		LOGGER.debug("init mainPage");
		mainPage.init(window, &project.cmake,config);
		Path projectPath = Path(data["project"].data);
		if (projectPath.exists()) {
			LOGGER.debug("load project: "+projectPath.toString());
			loadProject(projectPath,true);
		}
		LOGGER.debug("mainloop");
		while (window.update()==1)render();
	}


	Path projectDir()const {
		return project.cmake.projectDir();
	}
	void openConsole(CommandStream* stream, const string& cmd) {
		mainPage.openConsole(stream, cmd);
	}
	void openFile(const Path& path, LeafType type) {
		mainPage.openFile(path, type, projectDir());
		saveData();
	}
	void addSourceFile(const Path& path) {
		sourceFiles().insert(path.toString(), "");
		project.cmake.save();
		project.cmake.building.push(true);
	}
	void removeSourceFile(const Path& path) {
		sourceFiles().erase(path.toString());
		project.cmake.save();
		project.cmake.building.push(true);
	}
	bool isSourceFile(const Path& path) {
		return sourceFiles().find(path.toString());
	}
	JsonData& sourceFiles() {
		return project.cmake.data["Project"]["Source files"];
	}
	void renameFile(const Path& oldPath, const Path& newPath) {
		JsonData& files = project.data()["files"];
		const string key = oldPath.toString();
		if (files.find(key)) {
			mainPage.renameFile(oldPath, newPath, projectDir());
			files.insert(newPath.toString(), "");
			files.erase(key);
		}
		if (isSourceFile(key)) {
			removeSourceFile(key);
			addSourceFile(newPath.toString());
		}
	}
	bool update() {
		return window.update() == 1;
	}

	void openConfigPage() {
		setWindow("Configurations",
			WindowView(config, "Configurations"),
			&configPage, false);
	}
	void registerPlugin(const string& name, void* data, PluginFunc&& initf, PluginFunc&& runf) {
		plugins[name] = { data,initf,runf };
	}
	template<typename T>
	T& plugin(const string& name) {
		return plugins[name].plugin<T>();
	}
	void setWindow(const string& title, ImGuiMode mode, ImGuiPage page,bool releasePage) {
		mainPage.setWindow(window, title, mode, page,releasePage);
	}
};

typedef App::BasePlugin Plugin;
