#pragma once

#include "../common/common.h"
#include "../utils/gui.h"
#include "CodeAnalysis.h"

namespace Festa {
	class ImGuiTextEditor {
	public:
		typedef std::vector<uint> Palette;
		using Colors=CodeAnalysis::ExpressionType;
		struct EditorParams {
			ImFont* font = 0;
			Palette palette;
			uint indentSize = 4;
			float lineSpacing = 1.0f;
			EditorParams() {
				palette = {
					color(0.1f,0.1f,0.1f),//bg
					color(214,214,214),//line number 0.3f, 0.9f, 0.9f
					color(1.0f, 1.0f, 1.0f),//cursor
					color(0.2f, 0.2f, 0.6f),//selection
					color(1.0f,0.0f,0.0f,0.5f),//error marker

					color(1.0f,1.0f,1.0f),//text
					color(0.1f,0.9f,0.2f),//number
					color(0.7f,0.7f,1.0f),//punctuation
					color(0.9f,1.0f,0.0f),//string

				};
			}
			static uint color(float r, float g, float b,float a=1.0f) {
				return ImGui::ColorConvertFloat4ToU32(ImVec4(r,g,b,a));
			}
			static uint color(int r, int g, int b, int a = 255) {
				return ImGui::ColorConvertFloat4ToU32(ImVec4(
					float(r)/255.0f, float(g)/255.0f, float(b)/255.0f, float(a)/255.0f));
			}
		};
		struct ColorNode {
			uchar color=2;
			uint start = 0;
			ColorNode() {}
			ColorNode(uchar _color, uint _start) :
				color(_color), start(_start) {}
		};
		struct Line {
			std::vector<ColorNode> colors;
			std::string text;
			Line(){
				colors.emplace_back(ColorNode(2,0));
			}
			Line(const std::string& _text) {
				text = _text;
				colors.emplace_back(ColorNode(2,uint(_text.size())));
			}
			uint length()const {
				return uint(text.size());
			}
			void colorize(const std::vector<ColorNode>& nodes) {
				colors = nodes;
			}
			float render(ImDrawList* drawList,Palette& palette,const ImVec2& startPos,uint indent) {
				float offset = 0.0f;
				float space = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ").x;
				for (uint i = 0; i < colors.size(); i++) {
					if (colors[i].color == Colors::BACKGROUND) {
						uint end = i == colors.size() - 1 ? uint(text.size()) : colors[i + 1].start;
						for (uint j = colors[i].start; j < end; j++) {
							if (text[j] == '\t')offset += indent*space;
							else offset += space;
						}
						continue;
					}
					const char* st = text.c_str() + colors[i].start,
						* ed = text.c_str() + (i==colors.size()-1?text.size():colors[i+1].start);
					drawList->AddText(ImVec2(startPos.x+offset, startPos.y), palette[colors[i].color],st,ed);
					offset+= ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, st,ed).x;
				}
				return offset;
			}
			float getOffset(uint begin,uint end) {
				return ImGui::GetFont()->CalcTextSizeA(
					ImGui::GetFontSize(), FLT_MAX, -1.0f, 
					text.c_str()+begin, text.c_str() + end).x;
			}
			float getOffset(uint pos) {
				return getOffset(0, pos);
			}
			uint search(float x,uint indent) {
				if (!text.size())return 0;
				float pos = 0.0f; uint i = 0;
				float space= ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ").x;
				while(i<text.size()){
					float stride;
					uint size = 1;
					if (text[i] == ' ')stride = space;
					else if (text[i] == '\t')stride = space * indent;
					else {
						if (text[i] < 0 && i + 1 < text.size()) size++;
						stride = getOffset(i, i + 1);
					}
					if (pos + stride < x) {
						if (text[i] == ' ' || text[i] == '\t') {
							pos += stride;
							i += size;
							continue;
						}
						string tmp; tmp.push_back(text[i]);
						if (size>1) {
							tmp.push_back(text[i + 1]);
							//cout << "wchar " << u82string(tmp )<< endl;
						}
						tmp.push_back(' ');
						float advance = ImGui::GetFont()->CalcTextSizeA(
							ImGui::GetFontSize(), FLT_MAX, -1.0f, 
							tmp.c_str()).x - space;
						pos += advance;
						i += size;
						continue;
					}
					if (pos + stride - x < x - pos) {
						//cout << "cursor " << i + size << endl;
						return i + size;
					}
					else {
						//cout << "cursor " << i << endl;
						return i;
					}
				}
				return length();
			}
		};
		static EditorParams DefaultParams;
		std::vector<Line> lines;

		bool readOnly = false;
		bool showLineNumber = false;
		bool showCursor = true;
		
		EditorParams* params=0;

		ImGuiTextEditor() {
			lines.emplace_back("");
			params = &DefaultParams;
		}
		template<typename assistant_t>
		void Render() {

			if (ImGui::Button("Update")&&assistant) {
				((assistant_t*)assistant)->applyChanges();
			}

			if (params->font)ImGui::PushFont(params->font);
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(params->palette[Colors::BACKGROUND]));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
			ImGui::BeginChild("##Text", ImVec2(), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoMove);
			ImVec2 charAdvance = GetCharAdvance();
			ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
			ImVec2 contentSize = ImGui::GetWindowContentRegionMax();
			float lineNumberOff = LineNumberOffset(charAdvance.x);
			float width = lineNumberOff;

			HandleKeyboardInputs<assistant_t>();
			HandleMouseInputs(lineNumberOff);

			vec2 scroll(ImGui::GetScrollX(), ImGui::GetScrollY());

			uint lineIndex = uint(scroll.y / charAdvance.y);
			uint lineMax=std::max(0u, std::min(uint(lines.size() - 1), lineIndex + uint((scroll.y + contentSize.y) / charAdvance.y)));

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			const ivec2 cursorPos = GetCursorPos();

			ivec2 selbegin = _cursorPos, selend = _selPos;
			if (_sel)SwapSelection(selbegin, selend);

			for (; lineIndex <= lineMax;lineIndex++) {

				ImVec2 lineStart = ImVec2(cursorScreenPos.x, cursorScreenPos.y + lineIndex * charAdvance.y);
				float textPos = lineStart.x + lineNumberOff;
				if (showLineNumber) {
					const string num = toString(lineIndex + 1);
					float numberWidth = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, num.c_str(), nullptr, nullptr).x;
					drawList->AddText(ImVec2(lineStart.x + (lineNumberOff - numberWidth)/2.0f, lineStart.y),params->palette[Colors::LINE_NUMBER], num.c_str());
				}

				if (_sel && selbegin.y <= lineIndex && lineIndex <= selend.y) {
					uint l = lineIndex == selbegin.y ? selbegin.x : 0,
						r = lineIndex == selend.y ? selend.x : lines[lineIndex].text.size();
					float off = l < r ? 0.0f : charAdvance.x;
					drawList->AddRectFilled(
						ImVec2(textPos + lines[lineIndex].getOffset(l), 
							lineStart.y),
						ImVec2(textPos + lines[lineIndex].getOffset(r)+off, 
							lineStart.y + charAdvance.y), 
						params->palette[Colors::SELECTION]);
				}
				for (auto& err : errors) {
					if (err.begin.y <= lineIndex && lineIndex <= err.end.y) {
						uint l = lineIndex == err.begin.y ? err.begin.x : 0,
							r = lineIndex == err.end.y ? err.end.x : lines[lineIndex].text.size();
						float off = l < r ? 0.0f : charAdvance.x;
						ImVec2 lt(textPos + lines[lineIndex].getOffset(l),
							lineStart.y), rb(textPos + lines[lineIndex].getOffset(r) + off,
								lineStart.y + charAdvance.y);
						drawList->AddRectFilled(lt,rb,params->palette[Colors::ERRORMARKER]);
						if (ImGui::IsMouseHoveringRect(lt,rb)){
							ImGui::BeginTooltip();
							ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
							ImGui::Text(err.msg.c_str());
							ImGui::PopStyleColor();
							ImGui::EndTooltip();
						}
					}
				}
				
				if (ImGui::IsWindowFocused()&&lineIndex==cursorPos.y) {
					if (cursorTimer.interval() > 0.4f) {
						cursorTimer.reset(); showCursor = !showCursor;
					}
					if (showCursor) {
						float st = lines[lineIndex].getOffset(cursorPos.x) + textPos;
						drawList->AddRectFilled(ImVec2(st, lineStart.y), 
							ImVec2(st + 4.0f, lineStart.y + charAdvance.y), 
							params->palette[Colors::CURSOR]);
					}
				}
				float off=lines[lineIndex].render(drawList,params->palette,ImVec2(textPos,lineStart.y),params->indentSize);
				width = max(width, lineNumberOff + off);
			}

			ImGui::Dummy(ImVec2(width+2, lines.size() * charAdvance.y));
			ImGui::EndChild();
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
			if (params->font)ImGui::PopFont();
		}

		void Clear() {
			lines.resize(1);
			lines[0] = Line("");
		}
		template<typename assistant_t>
		ivec2 PushText(const std::string& _text,const ivec2& pos) {
			// use substr(may lead to mem err)
			//std::string text = string2u8(_text);
			std::string text = _text;
			uint y = pos.y;
			const string back=lines[y].text.substr(pos.x,lines[y].text.size()-pos.x);
			lines[y].text.resize(pos.x);
			for (ull i = 0; i < text.size(); i++) {
				if (text[i] == '\n') {
					if (assistant)
						((assistant_t*)assistant)->ColorizeLine(y);
					lines.insert(lines.begin() + y +1, Line());
					y++;
				}
				else{
					lines[y].text.push_back(text[i]);
				}
			}
			uint x = lines[y].text.size();
			lines[y].text += back;
			if (assistant)
				((assistant_t*)assistant)->ColorizeLine(y);
			return ivec2(x,y);
		}
		template<typename assistant_t>
		void PushText(const std::string& _text) {
			// use substr(may lead to mem err)
			std::string text = string2u8(_text);
			for (ull i = 0; i < text.size(); i++) {
				if (text[i] == '\n') {
					if (assistant)
						((assistant_t*)assistant)->ColorizeLine(lines.size() - 1);
					lines.emplace_back(Line());
				}
				else{
					lines.back().text.push_back(text[i]);
				}
			}
			if (assistant)
				((assistant_t*)assistant)->ColorizeLine(lines.size() - 1);
		}
		void GetText(std::string& text)const {
			for (uint i = 0; i < lines.size(); i++) {
				text += u82string(lines[i].text);
				if (i != lines.size() - 1)text.push_back('\n');
			}
		}
		void Save(const Path& file)const {
			std::ofstream f(file.toString().c_str(), std::ios::out);
			try {
				for (uint i = 0; i < lines.size();i++) {
					f << u82string(lines[i].text);
					if (i != lines.size() - 1)f << '\n';
				}
				f.close();
			}
			catch (std::ifstream::failure e) {
				LOGGER.error("Failed to write the file: ");
			}
		}
		ivec2 GetCursorPos() {
			return _cursorPos=NormalizePos(_cursorPos);
		}
		void SetCursorPos(uint line,uint column) {
			_cursorPos = NormalizePos(ivec2(column, line));
		}
		
		template<typename assistant_t>
		void BindAssistant(assistant_t& _assistant) {
			assistant = &_assistant;
		}
		Line& CurrentLine() {
			return lines[GetCursorPos().y];
		}
		static void SwapSelection(ivec2& begin,ivec2& end){
			if (begin.y > end.y ||
				(begin.y == end.y && begin.x > end.x))
				swap(begin, end);
		}
		string GetSelectionText()const {
			if (!_sel)return string();
			ivec2 selbegin = _cursorPos, selend = _selPos;
			SwapSelection(selbegin, selend);
			string ret;
			for (int i = selbegin.y; i <= selend.y; i++) {
				uint l = i == selbegin.y ? selbegin.x : 0,
					r = i == selend.y ? selend.x : lines[i].text.size();
				ret += lines[i].text.substr(l,r-l);
				if (i != selend.y)ret += "\n";
			}
			return ret;
		}
		void SetErrors(const std::list<CodeAnalysis::Message>& _errors) {
			errors = _errors;
		}
		void AddError(const CodeAnalysis::Message& err) {
			errors.emplace_back(err);
		}
		void ClearErrors() {
			errors.clear();
		}
	private:
		std::list<CodeAnalysis::Message> errors;
		bool _sel = false;
		void* assistant = 0;
		ivec2 _cursorPos = ivec2(0),_selPos=ivec2(0);

		Timer cursorTimer;


		ImVec2 GetCharAdvance()const {
			const float fontSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr).x;
			return ImVec2(fontSize, ImGui::GetTextLineHeightWithSpacing() * params->lineSpacing);
		}
		ivec2 NormalizePos(const ivec2& pos)const {
			ivec2 ret;
			ret.y = max(0,min(pos.y,int(lines.size()-1)));
			ret.x = max(0, min(pos.x,int(lines[pos.y].length())));
			return ret;
		} 
		ivec2 ScreenPos2Coord(const ImVec2& pos,float lineNumberOff) {
			ivec2 ret;
			float advance = ImGui::GetTextLineHeightWithSpacing() * params->lineSpacing;
			ImVec2 origin = ImGui::GetCursorScreenPos();
			ret.y=max(0, min(int((pos.y-origin.y-advance*0.5f)/advance+0.5f), int(lines.size()-1)));
			ret.x = lines[ret.y].search(pos.x-origin.x-lineNumberOff,params->indentSize);
			return NormalizePos(ret);
		}
		float LineNumberOffset(float advance)const {
			//return 5.0f * advance;
			return showLineNumber ? 5.0f * advance : 0.0f;
		}
		template<typename assistant_t>
		void HandleKeyboardInputs() {
			if (!ImGui::IsWindowFocused())return;
			ImGuiIO& io = ImGui::GetIO();
			bool shift = io.KeyShift;
			bool ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
			bool alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

			if (ImGui::IsWindowHovered())
				ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
			GetCursorPos();
			ivec2 selbegin = _cursorPos, selend = _selPos;
			if (_sel)SwapSelection(selbegin, selend);
			if (!alt) {
				//shift ctrl
				if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow))) {
					if (_sel) {
						_sel = false;
						_cursorPos = selbegin;
					}
					else {
						if (!_cursorPos.x)_cursorPos.y--, _cursorPos.x = lines[_cursorPos.y].length();
						else _cursorPos.x--;
					}
				}
				else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow))) {
					if (_sel) {
						_sel = false;
						_cursorPos = selend;
					}
					else {
						if (_cursorPos.x == lines[_cursorPos.y].length())_cursorPos.y++, _cursorPos.x = 0;
						else _cursorPos.x++;
					}
				}
				else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow))) {
					_cursorPos.y--;
				}
				else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow))) {
					_cursorPos.y++;
				}
				GetCursorPos();
					
			}
			if (!readOnly&&!ctrl && !shift && !alt) {
				if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
					Backspace<assistant_t>();
				else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
					Enter<assistant_t>();
				else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Tab)))
					EnterCharacter<assistant_t>('\t');
			}
			if (ctrl && !shift && !alt) {
				if (_sel&&ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C)))
					ImGui::SetClipboardText(GetSelectionText().c_str());
				else if (!readOnly && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_V))) {
					if (_sel)ClearSelection<assistant_t>();
					const char* ptr = ImGui::GetClipboardText();
					if(ptr)_cursorPos = PushText<assistant_t>(ptr, _cursorPos);
				}
				else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A)))
					_selPos=ivec2(0,0),_cursorPos = ivec2(lines.back().text.size(),lines.size()-1),_sel=true;
			}
			if (!readOnly && !io.InputQueueCharacters.empty()){
				for (int i = 0; i < io.InputQueueCharacters.Size; i++)
				{
					auto c = io.InputQueueCharacters[i];
					if (c)
						EnterCharacter<assistant_t>(c);
				}
				io.InputQueueCharacters.resize(0);
			}
		}
		void HandleMouseInputs(float lineNumberOff) {
			ImGuiIO& io = ImGui::GetIO();
			bool shift = io.KeyShift;
			bool ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
			bool alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;
			if (shift || alt)return;
			if (ImGui::IsWindowHovered()) {
				bool click = ImGui::IsMouseClicked(0);
				bool doubleClick = ImGui::IsMouseDoubleClicked(0);
				bool t = ImGui::GetTime();
				if (click) {
					_sel = false;
					_cursorPos = ScreenPos2Coord(ImGui::GetMousePos(),lineNumberOff);
				}
				else if (ImGui::IsMouseDragging(0) && ImGui::IsMouseDown(0)){
					io.WantCaptureMouse = true;
					if(!_sel)_selPos = _cursorPos;
					_sel = true;
					_cursorPos= ScreenPos2Coord(ImGui::GetMousePos(), lineNumberOff);
				}
			}
		}
		template<typename assistant_t>
		void ClearSelection() {
			if (!_sel)return;
			ivec2 selbegin = GetCursorPos(), selend = _selPos;
			SwapSelection(selbegin, selend);

			const std::string back = lines[selend.y].text.substr(
				selend.x, lines[selend.y].text.size()-selend.x);
			for (uint i = selbegin.y+1; i <= selend.y; i++) {
				lines.erase(lines.begin() + i); i--; selend.y--;
			}
			lines[selbegin.y].text.resize(selbegin.x);
			lines[selbegin.y].text += back;

			_sel = false;
			_cursorPos = selbegin;
			if (assistant)
				((assistant_t*)assistant)->ColorizeLine(_cursorPos.y);
		}
		void UpdateScroll() {
			ImVec2 charAdvance = GetCharAdvance();
			float scrollX = ImGui::GetScrollX();
			float scrollY = ImGui::GetScrollY();

			auto height = ImGui::GetWindowHeight();
			auto width = ImGui::GetWindowWidth();

			auto top = 1 + (int)ceil(scrollY / charAdvance.y);
			auto bottom = (int)ceil((scrollY + height) / charAdvance.y);

			auto left = (int)ceil(scrollX / charAdvance.x);
			auto right = (int)ceil((scrollX + width) / charAdvance.x);

			const ivec2 pos = GetCursorPos();
			float len = lines[pos.y].getOffset(pos.x);
			float lineNumberOffset = LineNumberOffset(charAdvance.x);

			if (pos.y < top)
				ImGui::SetScrollY(max(0.0f, (pos.y - 1) * charAdvance.y));
			if (pos.y > bottom - 4)
				ImGui::SetScrollY(max(0.0f, (pos.y + 4) * charAdvance.y - height));
			if (len + lineNumberOffset < left + 4)
				ImGui::SetScrollX(max(0.0f, len + lineNumberOffset - 4));
			if (len + lineNumberOffset > right - 4)
				ImGui::SetScrollX(max(0.0f, len + lineNumberOffset + 4 - width));
		}
		template<typename assistant_t>
		void EnterCharacter(uint ch) {
			bool enter = true;
			if (assistant)
				enter=((assistant_t*)assistant)->EnterCharacter(ch);
			if (enter) {
				wstring wstr; wstr.push_back(ch);
				EnterString<assistant_t>(wstring2string(wstr));
			}
			else ((assistant_t*)assistant)->ColorizeLine(GetCursorPos().y);
		}
		template<typename assistant_t>
		void EnterString(const string& str) {
			if (_sel)ClearSelection<assistant_t>();
			const ivec2 cursorPos = GetCursorPos();
			lines[cursorPos.y].text.insert(cursorPos.x, string2u8(str));
			_cursorPos.x += str.size();
			if (assistant)
				((assistant_t*)assistant)->ColorizeLine(cursorPos.y);
		}
		template<typename assistant_t>
		void Backspace() {
			GetCursorPos();
			if (_sel) {
				ClearSelection<assistant_t>(); return;
			}
			bool del=true;
			if (assistant)
				del=((assistant_t*)assistant)->Backspace();
			if (!del) {
				((assistant_t*)assistant)->ColorizeLine(_cursorPos.y);
				return;
			}
			Line& line = lines[_cursorPos.y];
			if (_cursorPos.x) {
				uint size = 1;
				if (_cursorPos.x>1&&line.text[_cursorPos.x - 2] < 0)size++;
				//cout << "delete " << _cursorPos.x << "," << size << endl;
				lines[_cursorPos.y].text.erase(_cursorPos.x-size, size);
				_cursorPos.x--;
			}
			else if(_cursorPos.y){
				_cursorPos.x = lines[_cursorPos.y-1].text.size();
				lines[_cursorPos.y - 1].text += lines[_cursorPos.y].text;
				lines.erase(lines.begin()+_cursorPos.y);
				_cursorPos.y--;
			}
			if (assistant)
				((assistant_t*)assistant)->ColorizeLine(_cursorPos.y);
			
		}
		template<typename assistant_t>
		void Enter() {
			const ivec2 cursor = GetCursorPos();
			Line& line = lines[cursor.y];
			const std::string& back = line.text.substr(cursor.x,
				line.text.size() - cursor.x);
			line.text.resize(cursor.x);
			lines.insert(lines.begin()+cursor.y + 1, Line(back));
			_cursorPos.y++; _cursorPos.x = 0;
			if (assistant) {
				((assistant_t*)assistant)->ColorizeLine(_cursorPos.y-1);
				((assistant_t*)assistant)->ColorizeLine(_cursorPos.y);
				((assistant_t*)assistant)->Enter();
			}
		}
	};

	class EditorAssistant {
	public:
		using Colors = ImGuiTextEditor::Colors;
		ImGuiTextEditor* editor=0;
		EditorAssistant() {}
		EditorAssistant(ImGuiTextEditor& _editor){
			_init(_editor);
		}
		void init(ImGuiTextEditor& _editor) {
			_init(_editor);
		}
		void _init(ImGuiTextEditor& _editor) {
			editor = &_editor;
			editor->BindAssistant(*this);
		}
		virtual void Colorize() {
			if (!editor)return;
			ImGuiTextEditor& e = *editor;

		}
		virtual void ColorizeLine(uint line) {
			if (!editor)return;
			ImGuiTextEditor::Line& l = editor->lines[line];
			l.colors.clear();
			uchar precol = Colors::TYPE_MAX;
			char isStr = 0;
			//cout << "colorize " << l.text << endl;
			for (uint i = 0; i < l.text.size(); i++) {
				uchar col;
				char c = l.text[i];
				bool s = c == '\'' || c == '\"';
				if (s)
					isStr =isStr?0: c;
				if (isStr || s)col = Colors::STRING;
				else if (isSpace(c))col = 0;
				else if (CodeAnalysis::isLetter(c) ||
					(i && CodeAnalysis::isNumber(c) &&
						(CodeAnalysis::isLetter(l.text[i - 1]) ||
							l.text[i - 1] == '\\' ||
							l.text[i - 1] == '/')))
					col = Colors::TEXT;
				else if (CodeAnalysis::isNumber(c)) {
					if (i && CodeAnalysis::isNumber(l.text[i - 1]))
						col = precol;
					else col = Colors::NUMBER;
				}
				else if (c == ' ' || c == '\t')col = Colors::BACKGROUND;
				else col = Colors::PUNCTUATION;
				if (col != precol) {
					l.colors.emplace_back(ImGuiTextEditor::ColorNode(col, i));
					precol = col;
					//cout << "   add " << uint(col) << ", " << i << endl;
				}
			}
		}
		virtual void Enter() {
			if (!editor)return;
			const ivec2 cursor = editor->GetCursorPos();
			uint i = 0; 
			if (cursor.y) {
				ImGuiTextEditor::Line& line = editor->lines[cursor.y-1];
				while (i < line.text.size() && line.text[i] == '\t')i++;
				string indent; indent.resize(i, '\t');
				editor->lines[cursor.y].text = indent + editor->lines[cursor.y].text;
				editor->SetCursorPos(cursor.y,cursor.x+i);
				ColorizeLine(cursor.y);
			}
			
		}
		virtual bool EnterCharacter(uint ch) {
			if (!editor)return true;
			ImGuiTextEditor::Line& line = editor->CurrentLine();
			const ivec2 cursor = editor->GetCursorPos();
			if (ch == uint('(') || ch == uint('[') || ch == uint('{')||
				ch==uint('\'')||ch==uint('\"')) {
				line.text.insert(cursor.x,1,CodeAnalysis::matchToken(char(ch)));
				line.text.insert(cursor.x, 1, char(ch));
				editor->SetCursorPos(cursor.y, cursor.x + 1);
				return false;
			}
			else if (cursor.x<line.text.size()&&
				(ch == uint(')') || ch == uint(']') || ch == uint('}')||
					ch==uint('\'')||ch==uint('\"'))
				&& uint(line.text[cursor.x]) == ch) {
				editor->SetCursorPos(cursor.y,cursor.x+1);
				return false;
			}
			return true;
		}
		virtual bool Backspace() {
			if (!editor)return true;
			const ivec2 cursor = editor->GetCursorPos();
			ImGuiTextEditor::Line& line = editor->CurrentLine();
			if (!cursor.x||cursor.x==line.length())return true;
			char m=CodeAnalysis::matchToken(line.text[cursor.x-1]);
			if(m&&line.text[cursor.x]==m) {
				line.text.erase(cursor.x-1, 2);
				editor->SetCursorPos(cursor.y,cursor.x-1);
				return false;
			}
			return true;
		}
		static bool isSpace(char x) {
			return x == ' ' || x == '\t';
		}
		virtual void applyChanges() {

		}
	};

	class CppAssistant:public EditorAssistant {
	public:
		CodeAnalysis::CppSources* sources = 0;
		Path path;
		CppAssistant() {}
		void init(const Path& _path,ImGuiTextEditor& _editor,CodeAnalysis::CppSources& _sources) {
			path = _path;
			_init(_editor); 
			sources = &_sources;
			sources->addFile(path, "");
		}
		void Enter() {
			if (!editor)return;
			const ivec2 cursor = editor->GetCursorPos();
			uint i = 0;
			if (!cursor.y)return;
			ImGuiTextEditor::Line& line = editor->lines[cursor.y - 1];
			ImGuiTextEditor::Line& current = editor->lines[cursor.y];
			while (i < line.text.size() && line.text[i] == '\t')i++;
			string indent; indent.resize(i+1, '\t');
			if (!line.text.size()|| line.text.back() == ';'||
				line.text.back()==' '|| line.text.back() =='}'||
				line.text.back()=='\t') {
				indent.resize(i);
				current.text = indent + current.text;
				editor->SetCursorPos(cursor.y, cursor.x + i);
			}
			else if ((line.text.back() == '[' || 
				line.text.back() == '(' || 
				line.text.back() == '{')&&current.text.size()&&
				CodeAnalysis::matchToken(line.text.back())==current.text[0]) {
				editor->lines.insert(editor->lines.begin()+cursor.y, ImGuiTextEditor::Line(indent));
				indent.resize(i);
				editor->lines[cursor.y+1].text = indent + editor->lines[cursor.y+1].text;
				editor->SetCursorPos(cursor.y, cursor.x + i + 1);
				ColorizeLine(cursor.y+1);
			}
			else {
				editor->lines[cursor.y].text = indent + editor->lines[cursor.y].text;
				editor->SetCursorPos(cursor.y, cursor.x + i+1);
			}
			ColorizeLine(cursor.y);

		}
		void applyChanges() {
			auto& f=sources->applyChanges(path);
			editor->SetErrors(f.errors);
			cout << "ERRORS: \n";
			for(auto& error:f.errors) {
				cout << error.begin.y+1 << ":" << error.begin.x << " - " <<
					error.end.y+1 << ":" << error.end.x <<"   "<<
					error.msg << endl;
			}
		}
	};




	ImGuiTextEditor::EditorParams ImGuiTextEditor::DefaultParams;
}
