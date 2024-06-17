#pragma once

#include "sources.h"
#include <ft2build.h>
#include <sstream>
#include <unordered_map>
#include FT_FREETYPE_H  


namespace Festa {
	struct FreetypeFont {
		struct Character {
			Image buf;
			int left = 0, top = 0, advance = 0;
			int w()const {
				return buf.width();
			}
			int h()const {
				return buf.height();
			}
			int bottom()const {
				return h() - top;
			}
		};
		struct Box {
			int w = 0, h = 0, top = 0,_advance=0;
			Box() {}
			Box(const Character& ch)
				:w(ch.w()), h(ch.h()), top(ch.top) {
				_advance = ch.advance-w;
			}
			void operator+=(const Box& box) {
				w += _advance + box.w;
				_advance = box._advance;
				h = std::max(h, box.h);
				top = std::max(top, box.top);
			}
			int bottom()const {
				return h - top;
			}
			int advance()const {
				return w + _advance;
			}
		};
		static FT_Library ft;
		FT_Face face;
		std::unordered_map<uint, Character> characters;
		FreetypeFont() {}
		FreetypeFont(const std::string& file, uint height = 48, uint width = 0) {
			init(file, height, width);
		}
		void init(const std::string& file, uint height = 48, uint width = 0);
		void setSize(uint height, uint width = 0) {
			FT_Set_Pixel_Sizes(face, height,width);
			characters.clear();
		}
		void release() {
			FT_Done_Face(face);
		}
		~FreetypeFont() {
			release();
		}
		Character& loadChar(uint ch) {
			if (characters.find(ch) != characters.end())return characters[ch];
			if (FT_Load_Char(face, ch, FT_LOAD_RENDER))LOGGER.error("Failed to load freetype glyph");
			uint h = face->glyph->bitmap.rows,w= face->glyph->bitmap.width;
			Character& c = characters[ch];
			//c.buf = _Image(face->glyph->bitmap.buffer,w,h, CV_8UC1);
			//cv::Mat(h,w,CV_8UC1,face->glyph->bitmap.buffer).copyTo(c.buf.mat);
			c.buf.init(face->glyph->bitmap.buffer,w,h,1);
			c.left = face->glyph->bitmap_left;
			c.top = face->glyph->bitmap_top;
			c.advance = face->glyph->advance.x >> 6;
			return c;
		}
		int putChar(Image* img,Character& c, const vec2& pos,
			const vec3& color = vec3(0.0f)) {
			assert(img->channels() == 3);
			int w = c.buf.width(), h = c.buf.height();
			for (int i = 0; i < w; i++) {
				int u = int(pos.x) + i + c.left;
				if (u < 0 || u >= img->width())break;
				for (int j = 0; j < h; j++) {
					int v = int(pos.y) + j - c.top;
					if (v < 0 || v >= img->height())break;
					float a = float(c.buf.get(i, j, 0)) / 255.0f;
					vec3 bg = vec3(img->get(u, v, 2), img->get(u, v, 1), img->get(u, v, 0));
					vec3 p = (bg + a * (255.0f * color - bg));
					img->get(u, v, 0) = uchar(p.z);
					img->get(u, v, 1) = uchar(p.y);
					img->get(u, v, 2) = uchar(p.x);
				}
			}
			return int(pos.x) + c.advance;
		}
		int putChar(Image* img, uint ch, const vec2& pos, const vec3& color = vec3(0.0f)){
			return putChar(img,loadChar(ch),pos);
		}
		void putText(Image* img, const std::string& text, const vec2& pos, const vec3& color = vec3(0.0f)) {
			int x = int(pos.x);
			std::wstring wstr = string2wstring(text);
			for (uint i = 0; i < wstr.size(); i++)
				x=putChar(img, wstr[i], vec2(x,pos.y), color);
		}
		Box getBox(const std::string& text) {
			Box ret;
			std::wstring wstr = string2wstring(text);
			for (uint i = 0; i < wstr.size(); i++)
				ret+=loadChar(wstr[i]);
			return ret;
		}
		Image* textImage(const std::string& text,const vec3& fg=vec3(0.0f),const vec3& bg=vec3(1.0f)) {
			FreetypeFont::Box box = getBox(text);
			Image* ret = new Image(bg, box.w, box.h);
			putText(ret, text, vec2(0.0f, box.top), fg);
			return ret;
		}
	};
	class FontRendering:public FreetypeFont {
	public:
		struct Character {
			Texture texture;
			uint w = 0, h = 0;
			int left = 0, bottom = 0, advance = 0;
			void init(FreetypeFont& font, uint ch) {
				FreetypeFont::Character& c=font.loadChar(ch);
				w = c.buf.width(), h = c.buf.height();
				texture.generate(w, h, c.buf.data(), GL_RED, GL_RED, GL_UNSIGNED_BYTE);
				Texture::wrapping(GL_CLAMP_TO_EDGE);
				Texture::minFilter(GL_LINEAR);
				Texture::magFilter(GL_LINEAR);
				left = c.left, bottom = h-c.top, advance = c.advance;
			}
			~Character() {
				texture.release();
			}
		};
		//unordered_map<uint, Character> characters;
		vec2 origin;
		int mode = FESTA_ORIGIN;
		
		FontRendering() { window = 0; origin = vec2(0.0f); }
		FontRendering(const Window* _window, const std::string& file, uint height = 48, uint width = 0)
		{
			_init(_window,file, height, width);
		}
		void setMode(int _mode) {
			mode = _mode;
		}
		void _init(const Window* _window, const std::string& file, uint height = 48, uint width = 0){
			window = _window;
			origin = vec2(0.0f);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			init(file, height, width);
		}
		void setOrigin(const vec2& pos) {
			origin = pos;
		}
		void render(const std::string& str, const vec3& color = vec3(0.0f), const vec2& scale = vec2(1.0f)) {
			std::wstring wstr = string2wstring(str);
			for (uint i = 0; i < wstr.size(); i++)
				render(wstr[i], color, scale);
		}
		void render(const std::string& str, const vec2& pos, const vec3& color = vec3(0.0f), const vec2& scale = vec2(1.0f)) {
			origin = pos;
			render(str, color, scale);
		}
		void render(const Camera& camera,
			const std::string& str, const vec3& pos, const vec3& color = vec3(0.0f), const vec2& scale = vec2(1.0f)) {
			//origin = camera.world2screen(pos,window->viewport);
			vec3 ndc = camera.world2ndc(pos);
			//origin = window->center2lt(ndc);
			origin = window->viewport.ndc2screen(ndc);
			render(str, color, scale);
		}
		
		~FontRendering() {
			
		}
	private:
		const Window* window;
		void render(uint ch, const vec3& color = vec3(0.0f), const vec2& scale = vec2(1.0f)) {
			//if (!characters[ch].texture)characters[ch].init(*this, ch);
			//const Character& c = characters[ch];
			Character c; c.init(*this, ch);
			const vec2 size(float(c.w), float(c.h)),
				vsize=window->viewport.size();
			vec2 position;
			switch (mode) {
			/*case FESTA_LEFT_BOTTOM:
				position = origin +
					scale * vec2(float(c.left) + size.x / 2.0f,-size.y / 2.0f) -
					vec2(float(window->viewport.x), float(window->viewport.y));
				break;
			case FESTA_LEFT_TOP:
				position = origin +
					scale * vec2(float(c.left) + size.x / 2.0f, -float(c.h-c.bottom) + size.y / 2.0f) -
					vec2(float(window->viewport.x), float(window->viewport.y));
				break;*/
			case FESTA_ORIGIN:
				position = origin - window->viewport.pos() +
					scale * vec2(float(c.left) + size.x / 2.0f, float(c.bottom) - size.y / 2.0f);
				break;
			}
			
			position = vec2(position.x-vsize.x / 2.0f,vsize.y/2.0f-position.y)/(vsize/2.0f);
			mat4 trans =  translate4(vec3(position, 0.0f))*
				scale4(vec3(size * scale / vsize, 0.0f));
			drawTexture(c.texture, trans, mat4(0.0f, 0.0f, 0.0f, 1.0f,
				0.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 0.0f,
				color.x, color.y, color.z, 0.0f));
			origin.x += c.advance * scale.x;
		}
		void render(uint ch, const vec2& pos, const vec3& color = vec3(0.0f), const vec2& scale = vec2(1.0f)) {
			origin = pos;
			render(ch, color, scale);
		}	
	};


}

