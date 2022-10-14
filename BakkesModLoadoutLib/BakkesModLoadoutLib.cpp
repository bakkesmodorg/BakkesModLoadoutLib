// BakkesModLoadoutLib.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include <string>
#include "bmloadout.h"
#include <iostream>
#include "helper_classes.h"

#include <fstream>

/*
	Slots supported by BakkesMod
*/
enum EQUIPSLOT {
	SLOT_BODY = 0, //Body won't be applied when loading in BakkesMod, user must have it equipped
	SLOT_SKIN = 1,
	SLOT_WHEELS = 2,
	SLOT_BOOST = 3,
	SLOT_ANTENNA = 4,
	SLOT_HAT = 5,

	SLOT_PAINTFINISH = 7,
	SLOT_PAINTFINISH_SECONDARY = 12,

	SLOT_ENGINE_AUDIO = 13,
	SLOT_SUPERSONIC_TRAIL = 14,
	SLOT_GOALEXPLOSION = 15,
	SLOT_ANTHEM = 18
};

enum ITEMPAINT {
	PAINT_NONE = 0,
	PAINT_CRIMSON = 1,
	PAINT_LIME = 2,
	PAINT_BLACK = 3,
	PAINT_SKYBLUE = 4,
	PAINT_COBALT = 5,
	PAINT_BURNTSIENNA = 6,
	PAINT_FORESTGREEN = 7,
	PAINT_PURPLE = 8,
	PAINT_PINK = 9,
	PAINT_ORANGE = 10,
	PAINT_GREY = 11,
	PAINT_TITANIUMWHITE = 12,
	PAINT_SAFFRON = 13,
	PAINT_GOLD = 14,
	PAINT_ROSEGOLD = 15,
	PAINT_WHITEGOLD = 16,
	PAINT_ONYX = 17,
	PAINT_PLATINUM = 18
};

void print_loadout(BMLoadout loadout)
{
	std::cout << "HEADER " << std::endl << "\tVersion: " << unsigned(loadout.header.version) << std::endl;
	std::cout << "\tSize in bytes: " << loadout.header.code_size << std::endl;
	std::cout << "\tCRC: " << unsigned(loadout.header.crc) << std::endl << std::endl;

	std::cout << "Blue is orange: " << (loadout.body.blue_is_orange ? "true" : "false") << std::endl;

	std::cout << "Blue: " << std::endl;

	for (auto body : loadout.body.blue_loadout)
	{
		std::cout << "\tSlot: " << unsigned(body.first) << ", ID: " << body.second.product_id << ", paint: " << unsigned(body.second.paint_index);
		if (loadout.body.blue_wheel_team_id != 0 && body.second.slot_index == SLOT_WHEELS) {
			std::cout << ", teamID: " << unsigned(loadout.body.blue_wheel_team_id);
		}
		std::cout << std::endl;
	}
	if (loadout.body.blueColor.should_override) {
		std::cout << "Color Primary (" << unsigned(loadout.body.blueColor.primary_colors.r) << ", " << unsigned(loadout.body.blueColor.primary_colors.g) << ", " << unsigned(loadout.body.blueColor.primary_colors.b) << ")";
		std::cout << " Secondary (" << unsigned(loadout.body.blueColor.secondary_colors.r) << ", " << unsigned(loadout.body.blueColor.secondary_colors.g) << ", " << unsigned(loadout.body.blueColor.secondary_colors.b) << ")";
	}

	if (!loadout.body.blue_is_orange)
	{
		std::cout << std::endl << "Orange: " << std::endl;
		for (auto body : loadout.body.orange_loadout)
		{
			std::cout << "\tSlot: " << unsigned(body.first) << ", ID: " << body.second.product_id << ", paint: " << unsigned(body.second.paint_index);
			if (loadout.body.orange_wheel_team_id != 0 && body.second.slot_index == SLOT_WHEELS) {
				std::cout << ", teamID: " << unsigned(loadout.body.orange_wheel_team_id);
			}
			std::cout << std::endl;
		}

		if (loadout.body.orangeColor.should_override) {
			std::cout << "Color Primary (" << unsigned(loadout.body.orangeColor.primary_colors.r) << ", " << unsigned(loadout.body.orangeColor.primary_colors.g) << ", " << unsigned(loadout.body.orangeColor.primary_colors.b) << ")";
			std::cout << " Secondary (" << unsigned(loadout.body.orangeColor.secondary_colors.r) << ", " << unsigned(loadout.body.orangeColor.secondary_colors.g) << ", " << unsigned(loadout.body.orangeColor.secondary_colors.b) << ")";
		}
	}
	std::cout << std::endl << std::endl;
}

std::map<uint8_t, Item> read_items_from_buffer(BitBinaryReader<unsigned char>& reader, const int loadoutVersion)
{
	std::map<uint8_t, Item> items;
	int itemsSize = reader.ReadBits<int>(4); //Read the length of the item array
	std::cout << "itemsSize: " << itemsSize << std::endl;
	if (itemsSize == 0) {
		int itemsSize = reader.ReadBits<int>(4); //Read the length of the item array
		std::cout << "itemsSize: " << itemsSize << std::endl;
	}
	for (int i = 0; i < itemsSize; i++)
	{
		Item option;
		int slotIndex = reader.ReadBits<int>(5); //Read slot of item
		int productId = reader.ReadBits<int>(loadoutVersion >= 4 ? 16 : 13); //Read product ID
		bool isPaintable = reader.ReadBool(); //Read whether item is paintable or not

		if (isPaintable)
		{
			int paintID = reader.ReadBits<int>(6); //Read paint index
			option.paint_index = paintID;
		}
		option.product_id = productId;
		option.slot_index = slotIndex;
		items.insert_or_assign(slotIndex, option); //Add item to loadout at its selected slot
	}
	return items;
}

RGB read_colors_from_buffer(BitBinaryReader<unsigned char>& reader)
{
	RGB col;
	col.r = reader.ReadBits<uint8_t>(8);
	col.g = reader.ReadBits<uint8_t>(8);
	col.b = reader.ReadBits<uint8_t>(8);
	return col;
}

BMLoadout load(std::string loadoutString)
{
	BitBinaryReader<unsigned char> reader(loadoutString);
	BMLoadout loadout;

	/*
	Reads header
		VERSION (6 bits)
		SIZE_IN_BYTES (10 bits)
		CRC (8 BITS)
	*/
	loadout.header.version = reader.ReadBits<uint8_t>(6);
	loadout.header.code_size = reader.ReadBits<uint16_t>(10);
	loadout.header.crc = reader.ReadBits<uint8_t>(8);

	/* Verification (can be skipped if you already know the code is correct) */

	/*
	Calculate whether code_size converted to base64 is actually equal to the given input string
	Mostly done so we don't end up with invalid buffers, but this step is not required.
	*/
	int stringSizeCalc = ((int)ceil((4 * (float)loadout.header.code_size / 3)) + 3) & ~3;
	int stringSize = loadoutString.size();

	if (abs(stringSizeCalc - stringSize) > 6) //Diff may be at most 4 (?) because of base64 padding, but we check > 6 because IDK
	{
		//Given input string is probably invalid, handle
		std::cout << "Invalid input string size!";
		exit(0);
	}

	/*
	Verify CRC, aka check if user didn't mess with the input string to create invalid loadouts
	*/
	if (!reader.VerifyCRC(loadout.header.crc, 3, loadout.header.code_size))
	{
		//User changed characters in input string, items isn't valid! handle here
		std::cout << "Invalid input string! CRC check failed";
		exit(0);
	}

	//At this point we know the input string is probably correct, time to parse the body

	loadout.body.blue_is_orange = reader.ReadBool(); //Read single bit indicating whether blue = orange
	loadout.body.blue_loadout = read_items_from_buffer(reader, loadout.header.version); //Read loadout

	loadout.body.blueColor.should_override = reader.ReadBool(); //Read whether custom colors is on
	if (loadout.body.blueColor.should_override) {
		/* Read rgb for primary colors (0-255)*/
		loadout.body.blueColor.primary_colors = read_colors_from_buffer(reader);

		/* Read rgb for secondary colors (0-255)*/
		loadout.body.blueColor.secondary_colors = read_colors_from_buffer(reader);
	}

	if (loadout.header.version > 2) {
		loadout.body.blue_wheel_team_id = reader.ReadBits<int>(6);
	}

	if (loadout.body.blue_is_orange) //User has same loadout for both teams
	{
		loadout.body.orange_loadout = loadout.body.blue_loadout;
		loadout.body.orange_wheel_team_id = loadout.body.blue_wheel_team_id;
	}
	else {
		loadout.body.orange_loadout = read_items_from_buffer(reader, loadout.header.version);
		loadout.body.orangeColor.should_override = reader.ReadBool(); //Read whether custom colors is on
		if (loadout.body.blueColor.should_override) {
			/* Read rgb for primary colors (0-255)*/
			loadout.body.orangeColor.primary_colors = read_colors_from_buffer(reader);

			/* Read rgb for secondary colors (0-255)*/
			loadout.body.orangeColor.secondary_colors = read_colors_from_buffer(reader);
		}

		if (loadout.header.version > 2) {
			loadout.body.orange_wheel_team_id = reader.ReadBits<int>(6);
		}
	}

	return loadout;
}

void write_loadout(BitBinaryWriter<unsigned char>& writer, std::map<uint8_t, Item> loadout)
{
	//Save current position so we can write the length here later
	const int amountStorePos = writer.current_bit;
	//Reserve 4 bits to write size later
	writer.WriteBits(0, 4);

	//Counter that keeps track of size
	int loadoutSize = 0;
	for (auto opt : loadout)
	{
		//In bakkesmod, when unequipping the productID gets set to 0 but doesn't
		//get removed, so we do this check here.
		if (opt.second.product_id <= 0)
			continue;
		loadoutSize++;
		writer.WriteBits(opt.first, 5); //Slot index, 5 bits so we get slot upto 31
		writer.WriteBits(opt.second.product_id, 16); //Item id, 13 bits so upto 8191 should be enough (Note: it wasnt, now 16 bits (since RL2.21, loadout v4))
		writer.WriteBool(opt.second.paint_index > 0); //Bool indicating whether item is paintable or not
		if (opt.second.paint_index > 0) //If paintable
		{
			writer.WriteBits(opt.second.paint_index, 6); //6 bits, allow upto 63 paints
		}
	}

	//Save current position of writer
	const int amountStorePos2 = writer.current_bit;
	writer.current_bit = amountStorePos;
	//Write the size of the loadout to the spot we allocated earlier
	writer.WriteBits(loadoutSize, 4); //Gives us a max of 15 customizable slots per team
	writer.current_bit = amountStorePos2; //Set back reader to original position
}

void write_color(BitBinaryWriter<unsigned char>& writer, RGB color)
{
	writer.WriteBits(color.r, 8);
	writer.WriteBits(color.g, 8);
	writer.WriteBits(color.b, 8);
}


std::string save(BMLoadout loadout)
{
	//Allocate buffer thats big enough
	BitBinaryWriter<unsigned char> writer(10000);
	writer.WriteBits(CURRENT_LOADOUT_VERSION, 6); //Write current version

	/*
	We write 18 empty bits here, because we determine size and CRC after writing the whole loadout
	but we still need to allocate this space in advance
	*/
	writer.WriteBits(0, 18);

	writer.WriteBool(loadout.body.blue_is_orange); //Write blue == orange?
	write_loadout(writer, loadout.body.blue_loadout);

	writer.WriteBool(loadout.body.blueColor.should_override); //Write override blue car colors or not

	if (loadout.body.blueColor.should_override)
	{
		write_color(writer, loadout.body.blueColor.primary_colors); // write primary colors RGB (R = 0-255, G = 0-255, B = 0-255)
		write_color(writer, loadout.body.blueColor.secondary_colors); //write secondary
	}

	writer.WriteBits(loadout.body.blue_wheel_team_id, 6);

	if (!loadout.body.blue_is_orange)
	{
		write_loadout(writer, loadout.body.orange_loadout);
		writer.WriteBool(loadout.body.orangeColor.should_override);//Write override orange car colors or not
		if (loadout.body.orangeColor.should_override)
		{
			write_color(writer, loadout.body.orangeColor.primary_colors); //write primary
			write_color(writer, loadout.body.orangeColor.secondary_colors); //write secondary
		}
	}

	writer.WriteBits(loadout.body.orange_wheel_team_id, 6);

	const int currentBit = writer.current_bit; //Save current location of writer

	int sizeInBytes = currentBit / 8 + (currentBit % 8 == 0 ? 0 : 1); //Calculate how many bytes are used
	writer.current_bit = 6; //Set writer to header (bit 6)
	writer.WriteBits(sizeInBytes, 10); //Write size
	writer.WriteBits(writer.CalculateCRC(3, sizeInBytes), 8); //Write calculated CRC
	writer.current_bit = currentBit; //Set writer back to original position
	return writer.ToHex();
}

int main()
{
	{
		//Loadout v3 test
		std::string myCode = "AwtFClyAjKh6BghADgD5Y/0HXv7//z8KXIBBOGUZoQEVyEVmWSfhtgEAACg=";

		//Loadout loading test
		BMLoadout loadout = load(myCode);
		print_loadout(loadout);
	}

	{
		//Loadout v4 test
		std::string myCode = "hAMwB1wABMItCGEWAAA=";
		BMLoadout loadout = load(myCode);
		print_loadout(loadout);
	}
	/*
	Create new loadout, leave header empty as the save() function takes care of it
	*/
	BMLoadout customLoadout;
	customLoadout.body.blue_is_orange = false; //Two seperate loadouts

	/*
	Customize blue car
	*/
	customLoadout.body.blue_loadout[SLOT_BODY] = { SLOT_BODY, 23, PAINT_TITANIUMWHITE }; //Equip TW octane body
	customLoadout.body.blue_loadout[SLOT_WHEELS] = { SLOT_WHEELS, 3925, PAINT_NONE }; //Equip Bionics esports wheel
	customLoadout.body.blue_wheel_team_id = 7; // G2 esports
	customLoadout.body.blue_loadout[SLOT_ANTENNA] = { SLOT_ANTENNA, 7, PAINT_NONE }; //Equip normal heart antenna
	customLoadout.body.blue_loadout[SLOT_BOOST] = { SLOT_BOOST, 32, PAINT_NONE }; //Equip normal alphaboost
	customLoadout.body.blue_loadout[SLOT_ANTHEM] = { SLOT_ANTHEM, 5695, PAINT_NONE }; //Equip Brutal Darkness anthem

	customLoadout.body.blueColor.should_override = true;
	customLoadout.body.blueColor.primary_colors = { 255, 192, 203 }; //pink primary
	customLoadout.body.blueColor.secondary_colors = { 255, 255, 255 }; //white secondary 

	/*
	Customize orange car
	*/
	customLoadout.body.orange_loadout[SLOT_BODY] = { SLOT_BODY, 23, PAINT_CRIMSON }; //Equip crimson octane body
	customLoadout.body.orange_loadout[SLOT_SKIN] = { SLOT_SKIN, 3239, PAINT_TITANIUMWHITE }; //Equip TW mainframe
	customLoadout.body.orange_loadout[SLOT_WHEELS] = { SLOT_WHEELS, 4122, PAINT_NONE }; //Equip Apex esports wheel
	customLoadout.body.orange_wheel_team_id = 20; // Spacestation gaming
	customLoadout.body.orange_loadout[SLOT_HAT] = { SLOT_HAT, 2960, PAINT_NONE }; //Equip birthday hat
	customLoadout.body.orange_loadout[SLOT_ANTHEM] = { SLOT_ANTHEM, 5721, PAINT_NONE }; //Equip Flip Reset anthem

	customLoadout.body.orangeColor.should_override = true;
	customLoadout.body.orangeColor.primary_colors = { 147, 112, 219 }; //purple primary
	customLoadout.body.orangeColor.secondary_colors = { 0,0,0 }; //black secondary 

	std::string loadoutString = save(customLoadout);
	std::cout << "Saved loadout to string " << loadoutString << std::endl;

	//Test by reloading our currently saved string and printing it
	BMLoadout reloaded = load(loadoutString);
	print_loadout(reloaded);

	return 0;
}