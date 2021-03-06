//@	 {"targets":[{"name":"waveformdata.o","type":"object"}]}

#include "waveformdata.hpp"
#include "sessionfilerecord.hpp"
#include "optionstring.hpp"
#include "../common/colorstring.hpp"
#include "../common/pathutils.hpp"

#include <cstdlib>
#include <cstring>

using namespace Anja;

WaveformData::WaveformData(const SessionFileRecord& record):m_filename(""),m_description("")
	,m_key_label(""),m_color(0.25f,0.0f,.5f,1.0f),m_stateflags(0)
	{
	auto value=record.propertyGet(String("Description"));
	if(value!=nullptr)
		{description(value->begin());}

	value=record.propertyGet(String("Color"));
	if(value!=nullptr)
		{keyColor(colorFromString(value->begin()));}


	value=record.propertyGet(String("Filename"));
	if(value!=nullptr)
		{m_filename=*value;}

	dirtyClear();
	}

WaveformData& WaveformData::clear()
	{
	m_filename.clear();
	m_description.clear();
	m_key_label.clear();
	m_color=COLORS[ColorID::CYAN_GRAY_DARK];
	m_stateflags=0;
	return *this;
	}

const WaveformData& WaveformData::store(SessionFileRecord& record) const
	{
	record.propertySet(String("Description"),m_description);
	record.propertySet(String("Color"),String(ColorString(m_color).begin()));

//	TODO Save other data not interpreted by Anja
	return *this;
	}

void WaveformData::key_label_update()
	{
	m_key_label.clear();
	auto state=0;
	auto ptr=m_description.begin();
	while(*ptr!='\0' && *ptr!=']')
		{
		switch(*ptr)
			{
			case '[':
				state=1;
				break;
			default:
				switch(state)
					{
					case 0:
						break;
					case 1:
						m_key_label.append(*ptr);
						break;
					}
			}
		++ptr;
		}

	if(m_key_label.length()==0)
		{
		auto ptr=m_description.begin();
		while(*ptr!=' ' && *ptr!='\0')
			{
			m_key_label.append(*ptr);
			++ptr;
			}
		}
	m_stateflags|=DIRTY;
	}

WaveformData& WaveformData::keyColor(const ColorRGBA& color)
	{
	if(std::abs(color.red - m_color.red) > 1e-3f
		|| std::abs(color.green - m_color.green) > 1e-3f
		|| std::abs(color.blue - m_color.blue) > 1e-3f
		|| std::abs(color.alpha - m_color.alpha) > 1e-3f)
		{
		m_color=color;
		m_stateflags|=DIRTY;
		}
	return *this;
	}
