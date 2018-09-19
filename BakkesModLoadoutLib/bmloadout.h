#pragma once
#include <stdint.h>
#include <map>

#define CURRENT_LOADOUT_VERSION 1

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

struct Body
{
	bool blue_is_orange = true;
	std::map<uint8_t, Item> blue_loadout;
	std::map<uint8_t, Item> orange_loadout;
};

struct BMLoadout
{
	Header header;
	Body body;
};
