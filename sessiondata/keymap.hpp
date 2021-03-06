//@	{
//@	 "targets":[{"name":"keymap.hpp","type":"include"}]
//@ ,"dependencies_extra":[{"ref":"keymap.o","rel":"implementation"}]
//@	}

#ifndef ANJA_KEYMAP_HPP
#define ANJA_KEYMAP_HPP

#include <cstdint>

namespace Anja
	{
	using namespace Anja;

	uint8_t channelToScancode(int channel);

	static constexpr int scancodeToChannel(uint8_t scancode)
		{
		if(scancode>=59 && scancode<69)
			{return scancode - 59;}
		if(scancode>=87 && scancode<89)
			{return 10 + (scancode - 87);}
		return -1;
		}

	uint8_t slotToScancode(uint8_t slot);

	uint8_t scancodeToSlot(uint8_t slot);

	static constexpr uint8_t slotToMIDI(uint8_t slot)
		{return (slot + 36)&0x7f;}

	static constexpr uint8_t midiToSlot(uint8_t midi)
		{return (midi - 36)&0x7f;}

	inline uint8_t scancodeToMIDI(uint8_t scancode)
		{
		auto slot=scancodeToSlot(scancode);
		if(slot==0xff)
			{return slot;}

		return slotToMIDI(slot);
		}

	namespace Keys
		{
		static constexpr int AUDITION=57;
		static constexpr int FADE_IN=103;
		static constexpr int FADE_OUT=108;
		static constexpr int FADE_IN_FAST=106;
		static constexpr int FADE_OUT_FAST=105;
		static constexpr int KILL_ALL=111;
		static constexpr int KILL_AUDITION=109;
		static constexpr int RECORD_START_L=29;
		static constexpr int RECORD_START_R=97;
		}
	}

#endif
