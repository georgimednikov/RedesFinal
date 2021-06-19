#pragma once
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <string>
using namespace std;

class Font {
private:
	TTF_Font* font_;

public:
	Font() : font_(nullptr) {};
	Font(const string& fileName, int size) { load(fileName, size); };
	virtual ~Font() { close(); };

	void load(const string& fileName, int size) {
		font_ = TTF_OpenFont(fileName.c_str(), size);
		if (font_ == nullptr) {
			std::cout << "Font " << TTF_GetError() << "\n";
		}
	}
	void close() {
		if (font_) {
			TTF_CloseFont(font_);
			font_ = nullptr;
		}
	}
	SDL_Surface* renderText(const string& text, SDL_Color color) const {
		if (font_) {
			return TTF_RenderText_Solid(font_, text.c_str(), color);
		}
		else {
			return nullptr;
		}
	}
};