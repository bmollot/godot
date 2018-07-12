/*************************************************************************/
/*  os_nx.cpp                                                        */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2018 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2018 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/
#include "os_nx.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "core/engine.h"
#include "print_string.h"
#include "servers/physics/physics_server_sw.h"
#include "servers/visual/visual_server_raster.h"
#include "errno.h"

// #include "drivers/unix/memory_pool_static_malloc.h"
#include "core/pool_allocator.h"
#include "core/os/thread_dummy.h"
#include "drivers/dummy/audio_driver_dummy.h"
#include "drivers/unix/thread_posix.h"
#include "drivers/unix/semaphore_posix.h"
#include "drivers/unix/mutex_posix.h"

//#include "core/io/file_access_buffered_fa.h"
#include "drivers/unix/file_access_unix.h"
#include "drivers/unix/dir_access_unix.h"
#include "drivers/unix/ip_unix.h"
#include "drivers/unix/tcp_server_posix.h"
#include "drivers/unix/stream_peer_tcp_posix.h"
#include "drivers/unix/packet_peer_udp_posix.h"
#include "drivers/nx/rasterizer_nx.h"

#include "main/main.h"

#include <SDL2/SDL.h>

namespace NX {
	#include <switch.h>
}

OS_NX::OS_NX()
: video_mode(800, 480, true, false, false)
{
	using namespace NX;
	gfxInitDefault();
	PrintConsole *console = consoleInit(NULL);
	
// 	set_low_processor_usage_mode(true);
	_render_thread_mode=RENDER_THREAD_UNSAFE;

	// AudioDriverManager::add_driver(&audio_driver);
	
	use_vsync = true;
	last_id = 1;
}

OS_NX::~OS_NX()
{
	NX::gfxExit();
}

void OS_NX::run()
{
	using namespace NX;
	if (!main_loop)
		return;
	
	main_loop->init();

	while (appletMainLoop())
	{
		processInput();
		
		if (hidKeysDown(CONTROLLER_P1_AUTO) & KEY_MINUS)
			break;
		
		if (Main::iteration() == true)
			break;
		
		printf("fps:%f\n", Engine::get_singleton()->get_frames_per_second());
	}
	
	main_loop->finish();
}

void OS_NX::initialize_core()
{
	using namespace NX;
	
	FileAccess::make_default<FileAccessUnix>(FileAccess::ACCESS_RESOURCES);
	FileAccess::make_default<FileAccessUnix>(FileAccess::ACCESS_USERDATA);
	FileAccess::make_default<FileAccessUnix>(FileAccess::ACCESS_FILESYSTEM);
	//FileAccessBufferedFA<FileAccessUnix>::make_default();
	DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_RESOURCES);
	DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_USERDATA);
	DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_FILESYSTEM);

#ifndef NO_NETWORK
	TCPServerPosix::make_default();
	StreamPeerTCPPosix::make_default();
	PacketPeerUDPPosix::make_default();
	IP_Unix::make_default();
#endif

#ifdef NO_THREADS
	ThreadDummy::make_default();
	SemaphoreDummy::make_default();
	MutexDummy::make_default();
	RWLockDummy::make_default();
#endif

	ticks_start = svcGetSystemTick();
}

Error OS_NX::initialize(const VideoMode& p_desired, int p_video_driver, int p_audio_driver)
{
	printf("OS_NX::initialize reached\n");
	main_loop = NULL;

	RasterizerNX::make_current();

	visual_server = memnew( VisualServerRaster() );

	if (get_render_thread_mode()!=RENDER_THREAD_UNSAFE) {
		visual_server =memnew(VisualServerWrapMT(visual_server,get_render_thread_mode()==RENDER_SEPARATE_THREAD));
	}

	AudioDriverManager::initialize(p_audio_driver);
	AudioDriverManager::add_driver(&audio_driver);
	
	AudioDriverManager::get_driver(0)->set_singleton();
	if (AudioDriverManager::get_driver(0)->init() != OK)
	{
		ERR_PRINT("Initializing audio failed.");
		return FAILED;
	}
	printf("audio initialized\n");

	audio_server = memnew( AudioServer() );
	audio_server->init();

	printf("audio server initialized\n");

	if (visual_server == NULL)
	{
		ERR_PRINT("Initializing video failed.");
		return FAILED;
	}
	
	visual_server->init();
	printf("visual server initialized\n");
	
	physics_server = memnew( PhysicsServerSW );
	physics_server->init();
	physics_2d_server = Physics2DServerWrapMT::init_server<Physics2DServerSW>();
	physics_2d_server->init();

	printf("physics server initialized\n");

	input = memnew( InputDefault );

	return OK;
}

void OS_NX::delete_main_loop()
{
	if (main_loop)
		memdelete(main_loop);
	main_loop = NULL;
}

void OS_NX::set_main_loop( MainLoop *p_main_loop )
{
	main_loop = p_main_loop;
	input->set_main_loop(p_main_loop);
}

void OS_NX::finalize()
{
	if(main_loop)
		memdelete(main_loop);
	main_loop=NULL;

	memdelete(input);

	audio_server->finish();
	memdelete(audio_server);

	visual_server->finish();
	memdelete(visual_server);
	memdelete(rasterizer);

	physics_server->finish();
	memdelete(physics_server);

	physics_2d_server->finish();
	memdelete(physics_2d_server);
}

void OS_NX::finalize_core()
{
}

// FIXME: Actually do something here
bool OS_NX::_check_internal_feature_support(const String &p_feature) {
	return false;
}

void OS_NX::vprint(const char* p_format, va_list p_list,bool p_stder)
{
	if (p_stder) {
		vfprintf(stderr,p_format,p_list);
		fflush(stderr);
	} else {
		vprintf(p_format,p_list);
		fflush(stdout);
	}
}

void OS_NX::alert(const String& p_alert,const String& p_title)
{
	fprintf(stderr,"ERROR: %s\n",p_alert.utf8().get_data());
}

Error OS_NX::set_cwd(const String& p_cwd)
{
	printf("set cwd: %s", p_cwd.utf8().get_data());
	if (chdir(p_cwd.utf8().get_data())!=0)
		return ERR_CANT_OPEN;

	return OK;
}

OS::Date OS_NX::get_date(bool utc) const
{

	time_t t=time(NULL);
	struct tm *lt;
	if (utc)
		lt=gmtime(&t);
	else
		lt=localtime(&t);
	Date ret;
	ret.year=1900+lt->tm_year;
	// Index starting at 1 to match OS_Unix::get_date
	//   and Windows SYSTEMTIME and tm_mon follows the typical structure 
	//   of 0-11, noted here: http://www.cplusplus.com/reference/ctime/tm/
	ret.month=(Month)(lt->tm_mon + 1);
	ret.day=lt->tm_mday;
	ret.weekday=(Weekday)lt->tm_wday;
	ret.dst=lt->tm_isdst;
	
	return ret;
}

OS::Time OS_NX::get_time(bool utc) const
{
	time_t t=time(NULL);
	struct tm *lt;
	if (utc)
		lt=gmtime(&t);
	else
		lt=localtime(&t);
	Time ret;
	ret.hour=lt->tm_hour;
	ret.min=lt->tm_min;
	ret.sec=lt->tm_sec;
	get_time_zone_info();
	return ret;
}

OS::TimeZoneInfo OS_NX::get_time_zone_info() const
{
	time_t t = time(NULL);
	struct tm *lt = localtime(&t);
	char name[16];
	strftime(name, 16, "%Z", lt);
	name[15] = 0;
	TimeZoneInfo ret;
	ret.name = name;

	char bias_buf[16];
	strftime(bias_buf, 16, "%z", lt);
	int bias;
	bias_buf[15] = 0;
	sscanf(bias_buf, "%d", &bias);

	// convert from ISO 8601 (1 minute=1, 1 hour=100) to minutes
	int hour = (int)bias / 100;
	int minutes = bias % 100;
	if (bias < 0)
		ret.bias = hour * 60 - minutes;
	else
		ret.bias = hour * 60 + minutes;

	return ret;
}

void OS_NX::delay_usec(uint32_t p_usec) const
{
 	printf("delay_usec: %lu\n", p_usec);
	NX::svcSleepThread(1000ULL * p_usec);
}

#define TICKS_PER_SEC 268123480ULL
#define TICKS_PER_USEC 268

uint64_t OS_NX::get_ticks_usec() const
{
	return (NX::svcGetSystemTick() - ticks_start) / TICKS_PER_USEC;
}

uint64_t OS_NX::get_unix_time() const
{
	return time(NULL);
}

uint64_t OS_NX::get_system_time_secs() const
{
	struct timeval tv_now;
	gettimeofday(&tv_now, NULL);
	//localtime(&tv_now.tv_usec);
	//localtime((const long *)&tv_now.tv_usec);
	return uint64_t(tv_now.tv_sec);
}

void OS_NX::swap_buffers()
{
	using namespace NX;
	gfxFlushBuffers();
	gfxSwapBuffers();
	if (use_vsync)
		gfxWaitForVsync();
}

// FIXME: Is this right?
int OS_NX::get_processor_count() const
{
	return 1;
}

static NX::u32 buttons[16] = {
	NX::KEY_B,
	NX::KEY_A,
	NX::KEY_Y,
	NX::KEY_X,
	NX::KEY_L,
	NX::KEY_R,
	NX::KEY_ZL,    // L2
	NX::KEY_ZR,    // R2
	NX::KEY_LSTICK,		 // L3
	NX::KEY_RSTICK,		 // R3
	NX::KEY_MINUS, // Select
	NX::KEY_PLUS,  // Start
	NX::KEY_DUP,
	NX::KEY_DDOWN,
	NX::KEY_DLEFT,
	NX::KEY_DRIGHT,
};

void OS_NX::processInput()
{
	using namespace NX;
	hidScanInput();
	u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
	u32 kUp = hidKeysUp(CONTROLLER_P1_AUTO);
	last_id++;
	
	for (int i = 0; i < 16; ++i)
	{
		if (buttons[i] & kDown)
			input->joy_button(last_id, i, true);
		else if (buttons[i] & kUp)
			input->joy_button(last_id, i, false);
	}
}