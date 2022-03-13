#include "Graphics.h"

Graphics::Graphics(int width, int height) : rad(Map::get().rad),
	width(width), height(height)
{
	sprites.loadFromFile(sprite_name);
	sprites.setSmooth(true);
	for (int i = 0; i < 4; i++) {
		tank_sprite[i].setTexture(sprites);
		tank_sprite[i].setTextureRect(sf::IntRect(32 * i, 0, 32, 32));
	}
	tank_sprite[4].setTexture(sprites);
	tank_sprite[4].setTextureRect(sf::IntRect(0, 32, 32, 32));
	repair_sprite.setTexture(sprites);
	repair_sprite.setTextureRect(sf::IntRect(32, 32, 32, 32));
	hard_repair_sprite.setTexture(sprites);
	hard_repair_sprite.setTextureRect(sf::IntRect(64, 32, 32, 32));
	catapult_sprite.setTexture(sprites);
	catapult_sprite.setTextureRect(sf::IntRect(96, 32, 32, 32));


	hex_R = float(std::min(width, height)) / (4 * rad);
	hex_r = hex_R / 2 * sqrt(3);

	x_center = width / 2 - hex_R;
	y_center = height / 2 - hex_R;
}

void Graphics::update()
{
	set_active(true);
	window.create(sf::VideoMode(width, height), "Bot");

	float factor = hex_R / 16.f * 0.5f;
	float shift = (hex_R - 16.f) / factor;

	while (window.isOpen())
	{
		window.clear(sf::Color::Black);

		draw_map();
		for (const auto& base : Map::get().base) {
			draw_hex(map_to_screen(base), sf::Color::Green);
		}
		for (const auto& obstacle : Map::get().obstacle) {
			draw_hex(map_to_screen(obstacle), sf::Color(255, 255, 255, 100));
		}
		for (const auto& repair : Map::get().light_repair) {
			draw_sprite(map_to_screen(repair) + sf::Vector2f(shift, shift), repair_sprite, factor);
		}
		for (const auto& repair : Map::get().hard_repair) {
			draw_sprite(map_to_screen(repair) + sf::Vector2f(shift, shift), hard_repair_sprite, factor);
		}
		for (const auto& catapult : Map::get().catapult) {
			draw_sprite(map_to_screen(catapult) + sf::Vector2f(shift, shift), catapult_sprite, factor);
		}
		for (const auto& [player, tanks] : GameState::get().vehicles) {
			sf::Color color = GameState::get().current_player_idx == player ? sf::Color(0, 128, 20, 255) : sf::Color::Red;
			for (int i = 0; i < tanks.size() && tanks[i]; i++) {
				draw_sprite(map_to_screen(tanks[i]->position) + sf::Vector2f(shift, shift), tank_sprite[i], factor, color);
			}
		}
		sf::Event event;
		window.display();
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}
	}
}

void Graphics::draw_map()
{
	for (int x = -rad; x < rad + 1; x++) {
		for (int y = -rad; y < rad + 1; y++) {
			int z = -x - y;
			if (distance({ x, y, z }, { 0, 0, 0 }) > rad) continue;
			draw_hex({ x_center + hex_r * (2 * x + y), y_center + y * hex_R * 1.5f }, sf::Color::Black);
		}
	}
}

void Graphics::draw_hex(sf::Vector2f position, sf::Color color)
{
	sf::CircleShape hex(hex_R, 6);
	hex.setPosition(position);
	hex.setFillColor(color);
	hex.setOutlineThickness(1.f);
	window.draw(hex);
}

void Graphics::draw_sprite(sf::Vector2f position, sf::Sprite sprite, float scale, sf::Color color)
{
	sprite.setColor(color);
	sprite.setPosition(position);
	sprite.setScale({ scale, scale });
	window.draw(sprite);
}

sf::Vector2f Graphics::map_to_screen(const Point& coord)
{
	return { x_center + hex_r * (2 * coord.x + coord.y), y_center + coord.y * hex_R * 1.5f };
}
