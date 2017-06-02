//@	{"targets":[{"name":"engine.o","type":"object"}]}

#include "engine.hpp"
#include "../sessiondata/keymap.hpp"

using namespace Anja;

static String client_name(const String& str)
	{
	auto ret=str;
	ret.append(".anja");
	return std::move(ret);
	}

namespace
	{
	template<Engine::TaskId x>
	struct TaskType
		{static constexpr Engine::TaskId value=x;};
	}

Engine::Engine(const Session& session):r_session(&session)
	,m_running(1)
	,m_ui_events(1024)
	,m_time_init(now_ms())
	,m_voices(64)
	,m_key_to_voice_index(128)
	,m_voices_alloc(m_voices.length())
	,m_client(client_name(session.titleGet()).begin(),*this)
	,m_rec_thread(*this,TaskType<Engine::TaskId::RECORD>{})
	{
	m_voices_alloc.fill();
	std::fill(m_key_to_voice_index.begin(),m_key_to_voice_index.begin()
		,m_voices_alloc.null());
	}

Engine::~Engine()
	{
	m_running=0;
	m_ready.set();
	}

static bool expired(AudioClient::MidiEvent e,double time_factor
	,double now)
	{
	return e.message.valid() && time_factor*e.time_offset<=now;
	}

void Engine::process(MIDI::Message msg) noexcept
	{
	switch(msg.status())
		{
		case MIDI::StatusCodes::NOTE_OFF:
			{
			auto i=m_key_to_voice_index[msg.value1()];
			if(i!=m_voices_alloc.null())
				{
				printf("Note off %u\n",i);
				//TODO: Time offset...
				m_voices[i].stop();

				//This should not be done until the waveform has been played
				m_voices_alloc.idRelease(i);
				}
			}
			break;
		case MIDI::StatusCodes::NOTE_ON:
			{
			auto i=m_voices_alloc.idGet();
			if(i!=m_voices_alloc.null())
				{
				printf("Note on %u\n",i);
				m_key_to_voice_index[msg.value1()]=i;
				//TODO: Time offset...
				m_voices[i]=Voice(r_session->waveformGet(midiToSlot(msg.value1())));
				}
			}
			break;

		default:
			break;
		}
	}

void Engine::process(AudioClient& client,int32_t n_frames) noexcept
	{
	auto time_factor=client.sampleRate()/1000.0; //TODO: Do not hard-code sample rate
	auto now=time_factor*(now_ms() - m_time_init);
	auto midi_in=client.midiIn(0,n_frames);
	auto midi_out=client.midiOut(0,n_frames);
	auto event_current=m_event_last;


	for(int32_t k=0;k<n_frames;++k)
		{
		if(expired(event_current,time_factor,now + k))
			{
			process(event_current.message);
			midi_out.write(event_current.message,k);
			event_current.message.clear();
			}
		while(!m_ui_events.empty())
			{
			event_current=m_ui_events.pop_front();
			if(expired(event_current,time_factor,now + k))
				{
				midi_out.write(event_current.message,k);
				process(event_current.message);
				event_current.message.clear();
				}
			else
				{break;}
			}
		}

	m_event_last=event_current;

	if(client.waveOutCount()==2) //Two outputs => single-channel output (Master + Audition)
		{
		auto master=client.waveOut(0,n_frames);
		auto audition=client.waveOut(1,n_frames);
		memset(audition,0,n_frames*sizeof(*audition));
		std::for_each(m_voices.begin(),m_voices.end(),[audition,n_frames,this](Voice& v)
			{
			if(!v.done())
				{v.generate(audition,n_frames);}
			});
		}


	}

template<>
void Engine::run<Engine::TaskId::RECORD>()
	{
	while(m_running)
		{
		m_ready.wait();
		}
	}

const char* Engine::port(AudioClient::PortType type,int index) const noexcept
	{
	switch(type)
		{
		case AudioClient::PortType::MIDI_IN:
			return index==0?"MIDI in":nullptr;
		case AudioClient::PortType::MIDI_OUT:
			return index==0?"MIDI out":nullptr;
		case AudioClient::PortType::WAVE_IN:
			return index==0?"Wave in":nullptr;
		case AudioClient::PortType::WAVE_OUT:
			if(r_session->flagsGet()&Session::MULTIOUTPUT)
				{
				if(index>=0 && index<16)
					{return r_session->channelLabelGet(index).begin();}
				if(index==16)
					{return "Master out";}
				if(index==17)
					{return "Audition";}
				return nullptr;
				}
			else
				{
				switch(index)
					{
					case 0:
						return "Master out";
					case 1:
						return "Audition";
					default:
						return nullptr;
					}
				}
			break;

		}
	return nullptr;
	}
