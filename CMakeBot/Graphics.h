#pragma once
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include "Map.h"
#include "GameState.h"
#include "Point.h"
#include "utility.h"

constexpr char sprite_name[] = "Sprites.png";

class Graphics {
public:
	Graphics(int width, int height);

	~Graphics() {
		window.close();
	}

	inline void set_active(bool state) {
		window.setActive(state);
	}

	void update();

private:
	void draw_map();

	void draw_hex(sf::Vector2f position, sf::Color color);

	void draw_sprite(sf::Vector2f position, sf::Sprite sprite, float scale, sf::Color color = sf::Color::White);

	sf::Vector2f map_to_screen(const Point& coord);

	sf::RenderWindow window;

	int width;
	int height;
	int x_center;
	int y_center;
	const int rad;
	float hex_R;
	float hex_r;

	sf::Sprite tank_sprite[5];
	sf::Sprite repair_sprite;
	sf::Sprite hard_repair_sprite;
	sf::Sprite catapult_sprite;
	sf::Texture sprites;
};