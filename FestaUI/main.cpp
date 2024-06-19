
#include "main.h"
//#include "3rd/ImGuiColorTextEdit/TextEditor.h"
#include "assets.h"


void App::FestaConfig::init(App& _app) {
	data = &_app.config;
	copy = _app.config;
	format.load(Files::formatFile);

	comp.addCallback("loadFile", [&](JsonData& data, JsonData& f, const string& label) {
		ImGui::Text((label + ": " + data.data).c_str());
		ImGui::SameLine();
		if (ImGui::Button("load")) {
			Festa::Path path = askOpenFileName(_app.window.hwnd, L"Freetype(*.ttf,*.ttc)\0*.ttf;*.ttc\0All(*.*)\0*.*\0");
			if (path.exists())data.data = path;
		}
		});

	ImGuiIO& io = ImGui::GetIO();

	io.Fonts->Clear();
	io.Fonts->AddFontDefault();

	uiFont=ImGuiFreetypeFont((*data)["UI"]["Font file"],
		(*data)["UI"]["Font size"].to<int>());
	Editors::params.font=ImGuiFreetypeFont((*data)["Editors"]["Font file"],
		(*data)["Editors"]["Font size"].to<int>());
	io.Fonts->Build();

	app = &_app;
	_reinit = true;
	reinit();
}

void App::FestaConfig::reinit() {
	if (!_reinit)return;
	_reinit = false;

	Editors::params.indentSize = (*data)["Editors"]["Code indent size"].to<uint>();

	switch ((*data)["UI"]["Theme"].to<int>()) {
	case 0:
		ImGui::StyleColorsDark();
		break;
	case 1:
		ImGui::StyleColorsClassic();
		break;
	case 2:
		ImGui::StyleColorsLight();
		break;
	}
}

void App::FestaConfig::render() {
	comp.render(copy,format);
	if (ImGui::Button("Reset to Default"))
		copy.load(Files::defaultConfig);
	ImGui::SameLine();
	if (ImGui::Button("OK")) {
		*data = copy;
		_reinit = true;
		save();
	}
}



static void initMenu(App& app) {
	Window& window = app.window;
	MenuBar menubar;
	Menu file, project, win, festa;
	file.add("New Project", [&] {
		app.plugin<NewProject>("New Project").open(app);
		});
	file.add("Open Project", [&] {
		wstring defaultPath = string2wstring(FESTA_PROJECTS_DIR.toWindows());
		Path file = askOpenFileName(app.window.hwnd,
		L"FestaProject(*.fstproj)\0*.fstproj\0All(*.*)\0*.*\0",
		"Open Project", defaultPath.c_str());
		if (file.str().size()) {
			app.project.open(file, app);
		}
		});
	file.add("Open Folder as Project", [&] {
		app.plugin<NewProject>("New Project").open(app);
		});
	file.add("Open File", [&] {
		Path file = askOpenFileName(app.window.hwnd,
		L"All(*.*)\0*.*\0",
		"Open Project");
		if (file.str().size())
			app.openFile(file,getFileType(file));
		});
	project.add("Configurations", [&] {
		if (app.project.empty()) {
			LOGGER.warning("Please create or open a project first");
			return;
		}
		app.plugin<ProjectConfigurations>("projectConfig").open(app);
		});
	project.add("Build", [&] {
		if (app.project.empty()) {
			LOGGER.warning("Please create or open a project first");
			return;
		}
		app.project.cmake.building.push(true);
		app.openConsole(app.project.cmake.build(true),"Build");
		});
	project.add("Compile", [&] {
		if (app.project.empty()) {
			LOGGER.warning("Please create or open a project first");
			return;
		}
		app.project.cmake.compiling.push(true);
		app.openConsole(app.project.cmake.compile(true),"Compile");
		});
	win.add("Open Assets", [&] {
		app.plugin<AssetsTree>("Assets").open(app);
		});
	win.add("Open Console", [&] {
		app.mainPage.openConsole(0,"");
		});
	win.add("Close All Tabs", [&] {
		app.mainPage.closeAllTabs();
		});
	win.add("Close All SubWindows", [&] {
		app.mainPage.closeAllWindows();
		});
	festa.add("Settings", [&] {
		app.openConfigPage();
		});
	menubar.addMenu("File", file);
	menubar.addMenu("Project", project);
	menubar.addMenu("Window", win);
	menubar.addMenu("Festa", festa);
	menubar.bind(app.window.hwnd);
}

using namespace CodeAnalysis;

void test() {
	string code =
		"#include <iost\\\nream >\n";
	std::list<Message> errors;
	CppAnalysis x(errors);
	CppFileSource src;
	x.parse(code, src);
	cout << "ERROR:\n";
	for (Message& error : errors) {
		cout << error.begin.y << ":" << error.begin.x << " - " <<
			error.end.y << ":" << error.end.x <<
			error.msg << endl;
	}
	exit(0);
}
int main() {
	//test();
	
	App app;
	NewProject newProject; newProject.reg(app);
	ProjectConfigurations pc; pc.reg(app);
	AssetsTree assetsTree; assetsTree.reg(app);
	initMenu(app);
	LOGGER.debug("Run.");
	app.run();
	LOGGER.debug("Exit");
	return 0;
}