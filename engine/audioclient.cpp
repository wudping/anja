//@	{"targets":[{"name":"audioclient.o","type":"object","pkgconfig_libs":["jack"]}]}

#include "audioclient.hpp"
#include "../common/error.hpp"
#include <jack/jack.h>

using namespace Anja;

class AudioClient::Impl:private AudioClient
	{
	public:
		Impl(const char* name,void* cb_obj,const Vtable& vt);
		~Impl();

	private:
		void* r_cb_obj;
		Vtable m_vt;
		jack_client_t* m_handle;
	};

AudioClient::AudioClient(const char* name,void* cb_obj,const Vtable& vt)
	{
	m_impl=new Impl(name,cb_obj,vt);
	}

AudioClient::~AudioClient()
	{delete m_impl;}



AudioClient::Impl::Impl(const char* name,void* cb_obj,const Vtable& vt):AudioClient(*this)
	,r_cb_obj(cb_obj),m_vt(vt)
	{
	jack_status_t status;
 	m_handle=jack_client_open(name,JackNoStartServer,&status);
	if(m_handle==NULL)
		{throw Error(name," failed to connect to JACK.");}

	jack_set_process_callback(m_handle,[](uint32_t n_frames,void* obj)
		{
		auto self=reinterpret_cast<Impl*>(obj);
		self->m_vt.process_callback(self->r_cb_obj,*self,n_frames);
		return 0;
		},this);

	PortInfo info;
	int k=0;
	while(vt.port_callback(cb_obj,k,info))
		{
		switch(info.type)
			{
			case PortType::MIDI_IN:
				jack_port_register(m_handle,info.name,JACK_DEFAULT_MIDI_TYPE,JackPortIsInput,0);
				break;
			case PortType::MIDI_OUT:
				jack_port_register(m_handle,info.name,JACK_DEFAULT_MIDI_TYPE,JackPortIsOutput,0);
				break;
			case PortType::WAVE_IN:
				jack_port_register(m_handle,info.name,JACK_DEFAULT_AUDIO_TYPE,JackPortIsInput,0);
				break;
			case PortType::WAVE_OUT:
			jack_port_register(m_handle,info.name,JACK_DEFAULT_AUDIO_TYPE,JackPortIsOutput,0);
				break;
			}
		++k;
		}

	jack_activate(m_handle);
	}

AudioClient::Impl::~Impl()
	{
	jack_deactivate(m_handle);
	jack_client_close(m_handle);
	m_impl=nullptr;
	}
