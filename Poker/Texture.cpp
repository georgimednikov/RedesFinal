#include <SDL2/SDL_image.h>
#include <iostream>
using namespace std;

class Texture {
private:
	SDL_Texture* texture_;
	SDL_Renderer* renderer_;

public:
	Texture() : texture_(nullptr), renderer_(nullptr) {}
	Texture(SDL_Renderer* renderer, const string& fileName) : texture_(nullptr) { loadFromImg(renderer, fileName); }
	//Texture(SDL_Renderer* renderer, const string& text, const Font* font, const SDL_Color& color) : texture_(nullptr) { loadFromText(renderer, text, font, color); }

	~Texture() { close(); }

	// Carga las texturas a partir de una imagen
	bool loadFromImg(SDL_Renderer* renderer, const string& fileName) {
		SDL_Surface* surface = IMG_Load(fileName.c_str());
		if (surface != nullptr) {
			close(); // destruye la textura actual para sustituirla
			texture_ = SDL_CreateTextureFromSurface(renderer, surface);
			SDL_FreeSurface(surface);
		}
		else {
			throw "No se puede cargar la imagen: " + fileName;
		}
		renderer_ = renderer;
		return texture_ != nullptr;
	}

	//Carga las texturas a partir de un texto con una fuente
	/*bool loadFromText(SDL_Renderer* renderer, const string& text, const Font* font, const SDL_Color& color = { 0, 0, 0, 255 }) {
		SDL_Surface* textSurface = font->renderText(text, color);
		if (textSurface != nullptr) {
			close();
			texture_ = SDL_CreateTextureFromSurface(renderer, textSurface);
			SDL_FreeSurface(textSurface);
		}
		else {
			throw "No se puede cargar el texto: " + text;
		}
		renderer_ = renderer;
		return texture_ != nullptr;
	}*/

	//Renderiza un frame de la textura en el destRect, si no hay frame se renderiza toda la textura
	void render(const SDL_Rect& dest, const SDL_Rect& frame) const {
		if (texture_) {
			SDL_RenderCopy(renderer_, texture_, &frame, &dest);
		}
	}

	// Renderiza un frame de la textura en el destRect con una rotaci�n, si no hay frame se renderiza toda la textura con un �ngulo
	void render(const SDL_Rect& dest, double angle, const SDL_Rect& frame) const {
		if (texture_) {
			SDL_RenderCopyEx(renderer_, texture_, &frame, &dest, angle, nullptr, SDL_FLIP_NONE);
		}
	}

	void close() {
		if (texture_ != nullptr) {
			SDL_DestroyTexture(texture_); // destruye la textura actual
			texture_ = nullptr;
		}
	}
};