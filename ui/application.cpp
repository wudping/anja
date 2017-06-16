//@	{
//@	"targets":
//@		[{
//@		"name":"application.o","type":"object"
//@		,"dependencies":[{"ref":"../logo.png","rel":"generated"}]
//@		,"cxxoptions":{"cflags_extra":["fno-lto"]}
//@		,"include_targets":["../projectinfo.hpp"]
//@		}]
//@	}

#include "application.hpp"
#include "filenameselect.hpp"
#include "../common/blob.hpp"
#include "../sessiondata/keymap.hpp"
#include "statusicons.hpp"
#include <inttypes.h>
#include <maike/targetinclude.hpp>

using namespace Anja;

ANJA_BLOB(uint8_t,s_logo,MAIKE_TARGET(../logo.png));

static constexpr const char* ANJA_OFFLINE="Click "
	"`Start engine` in the action panel to connect to the default JACK server.";
static constexpr const char* ANJA_ONLINE="Use a JACK patchbay tool such as Catia to route signals to/from Anja.";
static constexpr const char* ANJA_RESTART_NEEDED="The engine needs to be restarted.";

static void title_update(const Session& session,Window& win)
	{
	String title(session.titleGet());
	title.append("—Anja");
	win.title(title.begin());
	}

void Application::save_ask(ConfirmSaveDialogId id)
	{
	const char* title="";
	switch(id)
		{
		case ConfirmSaveDialogId::EXIT:
			title="Anja: Closing Anja";
			break;
		case ConfirmSaveDialogId::SESSION_LOAD:
			title="Anja: Loading another session";
			break;
		case ConfirmSaveDialogId::SESSION_RELOAD:
			title="Anja: Reloading session";
			break;
		case ConfirmSaveDialogId::SESSION_NEW:
			title="Anja: Creating a new session";
			break;
		}
	String msg("Do you want to save changes to ");
	msg.append(m_session.titleGet()).append("?");
	m_confirm.reset(new Dialog<Message,ConfirmSaveDialog>(m_mainwin,title,m_images
		,msg.begin(),Message::Type::WARNING));
	m_confirm->callback(*this,id);
	}

void Application::dismiss(Dialog<Message,ConfirmSaveDialog>& dlg,ConfirmSaveDialogId id)
	{
	m_confirm.reset();
	}

void Application::confirmPositive(Dialog<Message,ConfirmSaveDialog>& dlg,ConfirmSaveDialogId id)
	{
	if(sessionSave())
		{confirmNegative(dlg,id);}
	}

void Application::confirmNegative(Dialog<Message,ConfirmSaveDialog>& dlg,ConfirmSaveDialogId id)
	{
	m_confirm.reset();
	switch(id)
		{
		case ConfirmSaveDialogId::EXIT:
			m_ctx.exit();
			break;
		case ConfirmSaveDialogId::SESSION_LOAD:
			sessionLoad();
			break;
		case ConfirmSaveDialogId::SESSION_RELOAD:
			sessionLoad(m_session.filenameGet().begin());
			break;
		case ConfirmSaveDialogId::SESSION_NEW:
			sessionNew();
			break;
		}
	}

void Application::muted(Engine& engine,int channel) noexcept
	{
	m_ctx.messagePostTry(static_cast<int32_t>(MessageId::CHANNEL_MUTED),channel);
	}

void Application::unmuted(Engine& engine,int channel) noexcept
	{
	m_ctx.messagePostTry(static_cast<int32_t>(MessageId::CHANNEL_UNMUTED),channel);
	}

void Application::recordDone(Engine& engine,int slot) noexcept
	{
	m_ctx.messagePostTry(static_cast<int32_t>(MessageId::RECORD_DONE),slot);
	}

String Application::filename_generate(int slot)
	{
	String ret(m_session.waveformData(slot).keyLabel());
	if(ret.length()!=0)
		{ret.append('-');}
	char buff[64];
	sprintf(buff,"%" PRIx64 "-%x",wallclock(),m_rec_count);
	++m_rec_count;
	ret.append(buff).append(".wav");
	return ret;
	}

void Application::process(UiContext& ctx,MessageId id,MessageParam param)
	{
	switch(id)
		{
		case MessageId::CHANNEL_MUTED:
			assert(param>=0 && param<16);
			m_ch_status_img[param].showPng(m_images,static_cast<size_t>(StatusIcon::STOP)
				,statusIcon(StatusIcon::STOP));
			break;

		case MessageId::CHANNEL_UNMUTED:
			assert(param>=0 && param<16);
			m_ch_status_img[param].showPng(m_images,static_cast<size_t>(StatusIcon::READY)
				,statusIcon(StatusIcon::READY));
			break;

		case MessageId::RECORD_DONE:
			{
			assert(param>=0 && param<128);
			auto scancode=slotToScancode(param);
			if(scancode==0xff)
				{return;}
			m_session.waveformGet(param).sampleRate(m_engine->sampleRate())
				.offsetsReset().flagsSet(Waveform::RECORDED);
			if(m_keystate[scancode])
				{
				auto note=slotToMIDI(param);
				m_engine->messagePost(MIDI::Message
					{
					 MIDI::StatusCodes::NOTE_ON
					,static_cast<int>(m_session.waveformGet(param).channel())
					,note
					,127
					});
				}
			m_session.waveformViewGet(param).filename(filename_generate(param));
			m_session.slotActiveSet(param);
			m_session_editor.sessionUpdated();
			}
			break;
		}
	}

void Application::engine_start()
	{
	m_engine.reset( new Engine(m_session,*this) );
	m_status.message(ANJA_ONLINE).type(Message::Type::READY);
	}

void Application::engine_stop()
	{
	m_engine.reset();
	m_status.message(ANJA_OFFLINE).type(Message::Type::STOP);
	}

Application& Application::sessionNew()
	{
	engine_stop();
	m_session.clear();
	m_session_editor.sessionUpdated();
	title_update(m_session,m_mainwin);
	try
		{engine_start();}
	catch(...)
		{}
	return *this;
	}

bool Application::sessionSave()
	{
	if(m_session.filenameGet().length()!=0)
		{
		m_session.save(m_session.filenameGet().begin());
		return 1;
		}
	return sessionSaveAs();
	}

bool Application::sessionSaveAs()
	{
	auto name=std::string(m_session.filenameGet().begin());
	if(filenameSelect(m_mainwin,m_session.directoryGet().begin()
		,name,Anja::FilenameSelectMode::SAVE))
		{
		m_session.save(name.c_str());
		m_session_editor.sessionUpdated();
		return 1;
		}
	return 0;
	}

Application& Application::sessionLoad()
	{
	auto name=std::string(m_session.filenameGet().begin());
	if(filenameSelect(m_mainwin,m_session.directoryGet().begin()
		,name,Anja::FilenameSelectMode::OPEN,[this](const char* name)
			{return m_session.loadPossible(name);},"Anja session files"))
		{sessionLoad(name.c_str());}
	return *this;
	}



void Application::closing(Window& win,int id)
	{
	if(m_session.dirtyIs())
		{save_ask(ConfirmSaveDialogId::EXIT);}
	else
		{m_ctx.exit();}
	}

UiContext::RunStatus Application::idle(UiContext& ctx)
	{
	return UiContext::RunStatus::WAIT;
	}

void Application::keyDown(Anja::Window& win,int scancode,Anja::keymask_t keymask,int id)
	{
	if(m_engine && !m_keystate[scancode])
		{
		auto note=scancodeToMIDI(scancode);
		if(note!=0xff)
			{
			auto slot=scancodeToSlot(scancode);
			if(keymask&KEYMASK_KEY_CTRL)
				{m_engine->recordStop().recordStart(note);}
			else
				{
				m_engine->messagePost(MIDI::Message
					{
					 MIDI::StatusCodes::NOTE_ON
					,static_cast<int>(m_session.waveformGet(slot).channel())
					,note
					,127
					});
				}
			}
		else
			{
			auto ch=scancodeToChannel(scancode);
			if(ch>=0 && ch<16)
				{
				if(m_keystate[Keys::FADE_IN])
					{m_engine->fadeIn(ch,m_session.channel(ch).fadeTime());}
				else
				if(m_keystate[Keys::FADE_OUT])
					{m_engine->fadeOut(ch,m_session.channel(ch).fadeTime());}
				else
				if(m_keystate[Keys::FADE_IN_FAST])
					{m_engine->fadeIn(ch,1e-3f);}
				else
				if(m_keystate[Keys::FADE_OUT_FAST])
					{m_engine->fadeOut(ch,1e-3f);}
				}
			else
				{
				switch(scancode)
					{
					case Keys::AUDITION:
						{
						auto slot_current=m_session.slotActiveGet();
						assert(slot_current>=0 && slot_current<128);
						note=slotToMIDI(slot_current);
						m_engine->messagePost(MIDI::Message{MIDI::StatusCodes::NOTE_ON,0,note|0x80,127});
						}
						break;

					case Keys::KILL_ALL:
						for(size_t k=0;k<ChannelMixer::length();++k)
							{m_engine->messagePost(MIDI::Message{MIDI::ControlCodes::SOUND_OFF,static_cast<int>(k),0});}
						break;
					}
				}
			}
		}
	m_keystate[scancode]=1;
	}

void Application::keyUp(Anja::Window& win,int scancode,Anja::keymask_t keymask,int id)
	{
	if(m_engine)
		{
		auto note=scancodeToMIDI(scancode);
		if(note!=0xff)
			{
			auto slot=scancodeToSlot(scancode);
			if(keymask&KEYMASK_KEY_CTRL || scancode==Keys::RECORD_START_L
				|| scancode==Keys::RECORD_START_R)
				{m_engine->recordStop();}
			else
				{
				m_engine->messagePost(MIDI::Message
					{
					 MIDI::StatusCodes::NOTE_OFF
					,static_cast<int>(m_session.waveformGet(slot).channel())
					,note
					,127
					});
				}
			}
		else
			{
			switch(scancode)
				{
				case Keys::AUDITION:
					{
					auto slot_current=m_session.slotActiveGet();
					assert(slot_current>=0 && slot_current<128);
					note=slotToMIDI(slot_current);
					m_engine->messagePost(MIDI::Message{MIDI::StatusCodes::NOTE_OFF,0,note,127});
					}
					break;

				case Keys::RECORD_START_L:
				case Keys::RECORD_START_R:
					m_engine->recordStop();
					break;
				}

			}
		}
	m_keystate[scancode]=0;
	}


void Application::clicked(ButtonList& buttons,int id,Button& btn)
	{
	try
		{
		switch(btn.id())
			{
			case 0:
				if(m_session.dirtyIs())
					{save_ask(ConfirmSaveDialogId::SESSION_NEW);}
				else
					{sessionNew();}
				break;
			case 1:
				if(m_session.dirtyIs())
					{save_ask(ConfirmSaveDialogId::SESSION_LOAD);}
					{sessionLoad();}
				break;
			case 2:
				if(m_session.dirtyIs())
					{save_ask(ConfirmSaveDialogId::SESSION_RELOAD);}
				else
					{sessionLoad(m_session.filenameGet().begin());}
				break;
			case 3:
				sessionSave();
				break;
			case 4:
				sessionSaveAs();
				break;
			case 5:
				engine_start();
				break;
			case 6:
				engine_stop();
				break;
			case 7:
				m_fullscreen=!m_fullscreen;
				m_mainwin.fullscreen(m_fullscreen);
				btn.label(m_fullscreen?"Windowed":"Fullscreen");
				break;
			case 8:
				m_ctx.dark(!m_ctx.dark());
				btn.label(m_ctx.dark()?"Light UI":"Dark UI");
				break;

			case 9:
				if(m_session.dirtyIs())
					{save_ask(ConfirmSaveDialogId::EXIT);}
				else
					{m_ctx.exit();}
				break;

			case 10:
				m_about.reset(new Dialog<AboutBox,AboutDialog>(m_mainwin,"About Anja",ProjectInfo{}));
				m_about->widget().logo(m_images,StatusIconEnd,{s_logo_begin,s_logo_end},192);
				m_about->callback(*this,0);
				break;
			}
		}
	catch(const Error& e)
		{
		m_error.reset(new Dialog<Message,DialogOk>(m_mainwin,"Anja error",m_images
			,e.message(),Message::Type::ERROR));
		m_error->callback(*this,0);
		}
	btn.state(0);
	}

void Application::confirmPositive(Dialog<Message,DialogOk>& dlg,int id)
	{m_error.reset();}

void Application::confirmPositive(Dialog<AboutBox,AboutDialog>& dlg,int id)
	{m_about.reset();}

void Application::user1(Dialog<AboutBox,AboutDialog>& dlg,int id)
	{dlg.widget().legalBrief();}

void Application::user2(Dialog<AboutBox,AboutDialog>& dlg,int id)
	{dlg.widget().techstring();}

void Application::titleChanged(SessionPropertiesEditor& editor,int id)
	{
	title_update(m_session,m_mainwin);
	}

void Application::descriptionChanged(SessionPropertiesEditor& editor,int id)
	{}

void Application::optionChanged(SessionPropertiesEditor& editor,int id,int option)
	{
	if(id==0 && (1<<option)==Session::MULTIOUTPUT)
		{m_status.message(ANJA_RESTART_NEEDED).type(Message::Type::WAIT);}
	}


Application& Application::sessionLoad(const char* filename)
	{
	engine_stop();
	try
		{
		m_session.load(filename);
		m_session_editor.sessionUpdated();
		title_update(m_session,m_mainwin);
		}
	catch(...)
		{
		try
			{engine_start();}
		catch(...)
			{}
		throw;
		}
	try
		{engine_start();}
	catch(...)
		{}
	return *this;
	}

void Application::nameChanged(ChannelStrip& strip,int id)
	{
	if(m_engine && m_session.flagsGet()&Session::MULTIOUTPUT)
		{m_engine->waveOutName(id,strip.name().begin());}
	}

void Application::gainChanged(ChannelStrip& strip,int id)
	{
	if(m_engine)
		{m_engine->channelGain(id,strip.gain());}
	}

Application::Application():
	m_mainwin("New session--Anja")
		,m_cols(m_mainwin,false)
			,m_session_control(m_cols,true)
			,m_cols_sep(m_cols.insertMode({2,0}),true)
			,m_rows(m_cols.insertMode({0,Anja::Box::EXPAND|Anja::Box::FILL}),true)
				,m_status(m_rows,m_images,ANJA_OFFLINE,Message::Type::STOP,0)
				,m_sep_a(m_rows.insertMode({2,0}),false)
				,m_ch_status(m_rows.insertMode({0,0}),false)
					,m_ch_status_left(m_ch_status.insertMode({0,Anja::Box::EXPAND|Anja::Box::FILL}))
					,m_ch_status_img(m_ch_status.insertMode({0,0}),false)
					,m_ch_status_right(m_ch_status.insertMode({0,Anja::Box::EXPAND|Anja::Box::FILL}))
				,m_sep_b(m_rows.insertMode({2,0}),false)
				,m_session_editor(m_rows.insertMode({2,Anja::Box::EXPAND|Anja::Box::FILL}),m_images,m_session)
	,m_fullscreen(0)
	,m_rec_count(0)
	{
	m_session_control.append("New session","Load session","Reload session","Save session"
		,"Save session as","","Start engine","Stop engine","","Fullscreen","Dark UI",""
		,"Exit","About Anja");
	m_session_control.callback(*this,0);
	m_mainwin.callback(*this,0);
	m_session_editor.callback(*this,0);
	title_update(m_session,m_mainwin);
	m_ch_status_img.append<ChannelMixer::length()>();
	std::for_each(m_ch_status_img.begin(),m_ch_status_img.end(),[this](ImageView& v)
		{
		v.minHeight(20)
			.showPng(m_images,static_cast<size_t>(StatusIcon::READY),statusIcon(StatusIcon::READY));
		});
	m_mainwin.show();
	try
		{engine_start();}
	catch(...)
		{}
	}