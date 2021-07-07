#pragma once
#include <stdint.h>
#include <map>

#define CURRENT_LOADOUT_VERSION 3

struct Header
{
	uint8_t version = 0;
	uint16_t code_size = 0;
	uint8_t crc = 0;
};

struct Item
{
	uint8_t slot_index = 0;
	uint16_t product_id = 0;
	uint8_t paint_index = 0;
};

struct RGB
{
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
};

struct OverrideColor
{
	bool should_override = false;
	RGB primary_colors;
	RGB secondary_colors;
};

struct Body
{
	bool blue_is_orange = true;
	std::map<uint8_t, Item> blue_loadout;
	uint8_t blue_wheel_team_id;
	OverrideColor blueColor;
	std::map<uint8_t, Item> orange_loadout;
	OverrideColor orangeColor;
	uint8_t orange_wheel_team_id;
};

struct BMLoadout
{
	Header header;
	Body body;
};
