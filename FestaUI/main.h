#pragma once

#include "app.h"




class ImGuiTreeNode {
public:
	string label;
	bool isLeaf = false;
	bool open = false;
	bool focused = false;
	ImGuiTreeNode() {}
	ImGuiTreeNode(const string& _label) {

	}
	void init(const string& _label) {
		label = string2u8(_label);
	}
	virtual bool renderMenu() {
		return false;
	}
	virtual bool renderComponents() {
		return ImGui::Selectable(label.c_str(), &focused, ImGuiSelectableFlags_SpanAllColumns);
	}
	virtual void renderChildren() {

	}
	bool render() {
		if (open) {
			open = false;
			if(!isLeaf)ImGui::SetNextItemOpen(true);
		}
		bool ret = true;
		if (!isLeaf) {
			ret = ImGui::TreeNode(("##" + label).c_str());
			ImGui::SameLine();
		}
		bool tmp = false;
		tmp |= renderComponents();
		bool menu = renderMenu();
		if (!menu && (ImGui::IsMouseDown(0) || ImGui::IsMouseDown(1)))focused = tmp;
		if (menu)focused = true;
		if (ImGui::IsItemHovered()&&ImGui::IsMouseDoubleClicked(0))open = true;
		if (ret&&!isLeaf) {
			renderChildren();
			ImGui::TreePop();
		}
		return ret;
	}
};

class NewProjectPage :public BasePage {
public:
	string pname, directory = FESTA_PROJECTS_DIR;
	ImGuiInputBox box1, box2;
	App* _app = 0;
	bool _render = false;
	NewProjectPage() {}
	void init(App& app) {
		_render = true;
		pname.clear();
		directory = FESTA_PROJECTS_DIR;
		_app = &app;
		box1.init("Project Name:");
		box2.init("Project Root:");
		box2.str() = directory;
	}
	void render() {
		box1.render(); box2.render();
		pname = box1.str(), directory = box2.str();
		ImGui::SameLine();
		if (ImGui::Button("...")) {
			directory = askOpenFileName(_app->window.hwnd, L"");
		}
		string errorlog = Errorlog();
		if (pname.empty());
		if (errorlog.size()) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
			ImGui::Text(errorlog.c_str());
			ImGui::PopStyleColor(1);
		}
		else {
			Path dir = Path(directory) + pname;
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
			ImGui::Text((string("Your project directory: ") + dir.str()).c_str());
			ImGui::PopStyleColor(1);
			if (ImGui::Button("OK")) {
				_app->project.create(pname, dir, *_app);
				_render = false;
			}
		}
	}
	static bool isNum(char s) {
		return '0' <= s && s <= '9';
	}
	static bool isLetter(char s) {
		return 'A' <= s && s <= 'z' || s == '_';
	}
	string Errorlog() {
		if (pname.empty())return "Empty project name";
		bool unlawful = false;
		if (!isLetter(pname[0])) unlawful = true;
		else {
			for (uint i = 1; i < pname.size(); i++)
				if (!isLetter(pname[i]) && !isNum(pname[i])) {
					unlawful = true; break;
				}
		}
		if (unlawful)return "Unlawful project name";
		else if (!Path(directory).exists())return "The direcctory does not exist";
		else if ((Path(directory) + pname).exists())return "The project already exists";
		else return "";
	}
};

class NewProject :public Plugin {
public:
	NewProjectPage page;
	NewProject() {
		name = "New Project";
	}
	void open(App& app) {
		page.init(app);
		app.setWindow("New Project",IMGUI_NONE,&page,false);
	}
};



class PathTreeNode :public ImGuiTreeNode {
public:
	class SubTreeNode :public ImGuiTreeNode {
	public:
		JsonData* data = 0;
		PathTreeNode* father = 0;
		SubTreeNode() {}
		SubTreeNode(const string& _label, JsonData& _data,PathTreeNode* _father) {
			label = _label;
			data = &_data;
			father = _father;
			isLeaf = true;
		}
		bool renderComponents() {
			bool ret=false;
			if (data->isString()|| data->to<int>()) {
				ImGui::Bullet();
				ret |= ImGui::IsItemHovered();
				ImGui::SameLine();
				ret|=ImGui::Selectable(label.c_str(), &focused, ImGuiSelectableFlags_SpanAllColumns);
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
				ImGui::Bullet();
				ret |= ImGui::IsItemHovered();
				ImGui::SameLine();
				ret |= ImGui::Selectable(label.c_str(), &focused, ImGuiSelectableFlags_SpanAllColumns);
				ImGui::PopStyleColor(1);
			}
			return ret;
		}
		bool renderMenu() {
			if (ImGui::BeginPopupContextItem()) {
				if (data->isString()) {
					if (ImGui::MenuItem("Delete")) {
						father->data->erase(label);
						ImGui::EndMenu();
						return true;
					}
				}
				else if (data->to<int>()) {
					if (ImGui::MenuItem("Disable")) {
						data->to<int>() = 0;
						ImGui::EndMenu();
						return true;
					}
				}
				else if (ImGui::MenuItem("Enable")) {
					data->to<int>() = 1;
					ImGui::EndMenu();
					return true;
				}
				ImGui::EndPopup();
				return true;
			}
			return false;
		}
	};

	bool dir = false;
	std::list<SubTreeNode> ch;
	JsonData* data = 0;
	PathTreeNode() {}
	void init(JsonData& _data,const string& _label, bool _dir) {
		ch.clear();
		label = string2u8(_label); dir = _dir;
		data = &_data;
		open = true;
		for (auto i : _data) {
			ch.emplace_back(SubTreeNode(i.first,i.second,this));
		}
	}
	bool renderMenu() {
		if (ImGui::BeginPopupContextItem()) {
			if (ImGui::MenuItem("Add")) {
				Path path=askOpenFileName(0, L"All(*.*)\0*.*\0", "Open");
				if ((dir && path.isDirectory()) || (!dir && path.isFile())) {
					const string p = path.toString();
					data->insert(p, "");
					ch.emplace_back(SubTreeNode(p,(*data)[p],this));
				}
				ImGui::EndMenu();
				return true;
			}
			ImGui::EndPopup();
			return true;
		}
		return false;
	}
	void renderChildren() {
		for (auto it = ch.begin(); it != ch.end();it++) {
			it->render();
			if (!data->find(it->label)) {
				ch.erase(it);
				break;
			}
		}
		ImGui::Text("");
	}
};

class ProjectPage :public BasePage {
public:
	const string formatFile = "src/configs/projectPage.json";
	JsonData format, copy;
	App* _app = 0;
	JsonComponent comp;


	PathTreeNode sourceFiles, includeDirs,libFiles;

	ProjectPage() {
		format.load(formatFile);
	}
	void init(App& app) {
		app.project.cmake.checkMissingFiles();
		copy = app.project.cmake.data;
		_app = &app;

		initPaths();
	}
	void initPaths() {
		sourceFiles.init(copy["Project"]["Source files"], "Source files", false);
		includeDirs.init(copy["Project"]["Include directories"], "Include directories", true);
		libFiles.init(copy["Project"]["Library files"], "Library files", false);
	}
	void render() {
		JsonData& data = _app->project.cmake.data;
		JsonData& project = copy["Project"];

		bool gen = project["Generate CMakeLists.txt"].to<int>();
		comp.render(copy, format);
		auto genx = project["Generate CMakeLists.txt"].to<int>();
		if (genx != gen) {
			_app->project.cmake.writeCMakeLists();
		}
		sourceFiles.render();
		includeDirs.render();
		libFiles.render();
		//pathList(project,"Source files",sourceFiles);
		//pathList(project, "Include directories",includeDirs);
		if (ImGui::Button("Reset to Default")) {
			CMakeProject::defaultConfig(copy, _app->project.cmake.projectName());
			initPaths();
		}
		ImGui::SameLine();
		if (ImGui::Button("OK")) {
			data = copy;
			_app->project.cmake.save();
			_app->project.cmake.building.push(true);
			_app->loadProject(_app->project.cmake.file,false);
		}
	}
	void pathList(JsonData& root,const string& key, PathTreeNode& node) {
		JsonData& paths = root[key];
		JsonData tmp = paths;
		if (node.render()) {
			for (auto i : paths) {
				if (i.second.isString()) {
					BulletText(i.first.c_str());
					//ImGui::SameLine();
					//if (ImGui::Button("Delete"))tmp.erase(i.first);
				}
				else if (i.second.to<int>()) {
					BulletText(i.first.c_str());
					//ImGui::SameLine();
					//if (ImGui::Button("Disable"))i.second.to<int>() = 0;
				}
				else {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f,0.5f,0.5f, 1.0f));
					BulletText(i.first.c_str());
					ImGui::PopStyleColor(1);
					//ImGui::SameLine();
					//if (ImGui::Button("Enable"))i.second.to<int>() = 1;
				}
				pathMenu(tmp,i.first,i.second);
			}
			ImGui::Text("");
			ImGui::TreePop();
		}
		if (paths.size() > tmp.size())paths = tmp;
	}
	void BulletText(const string& text) {
		ImGui::Bullet();
		ImGui::SameLine();
		ImGui::Selectable(text.c_str());
	}
	bool pathMenu(JsonData& root,const string& key,JsonData& data) {
		if (ImGui::BeginPopupContextItem()) {
			if (data.isString()&&ImGui::MenuItem("Delete")) {
				root.erase(key);
				ImGui::EndMenu();
				return true;
			}
			else if (data.to<int>() && ImGui::MenuItem("Disable")) {
				data.to<int>() = 0;
				ImGui::EndMenu();
				return true;
			}
			else if (ImGui::MenuItem("Enable")) {
				data.to<int>() = 1;
				ImGui::EndMenu();
				return true;
			}
			ImGui::EndPopup();
			return true;
		}
		return false;
		
	}
};

class ProjectConfigurations :public Plugin {
public:
	ProjectPage page;
	ProjectConfigurations() {
		name = "projectConfig";
	}
	void open(App& app) {
		page.init(app);
		app.setWindow("Project Configurations", WindowView(app.config, "Project Configurations"), &page, false);
	}
	void init(App& app) {
		
	}
	void run(App& app) {
		
	}
};

