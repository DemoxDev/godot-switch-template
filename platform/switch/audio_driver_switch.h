#pragma once

#include "core/os/mutex.h"
#include "core/os/thread.h"
#include "core/templates/safe_refcount.h"
#include "servers/audio/audio_server.h"

#include "nx_wrap.h"

/// Stereo s16 output through libnx audout. The mix thread is paced by the hardware: `nx::audio_submit`
/// blocks until a buffer is free, so no sleep-based timing is needed.
class AudioDriverSwitch : public AudioDriver {
	Thread thread;
	Mutex mutex;

	int32_t samples_in[nx::AUDIO_FRAMES_PER_BUFFER * 2];
	int16_t samples_out[nx::AUDIO_FRAMES_PER_BUFFER * 2];

	int mix_rate = 48000;
	SafeFlag active;
	SafeFlag exit_thread;
	bool initialized = false;

	static void thread_func(void *p_udata);

public:
	virtual const char *get_name() const override { return "Switch"; }

	virtual Error init() override;
	virtual void start() override;
	virtual int get_mix_rate() const override { return mix_rate; }
	virtual SpeakerMode get_speaker_mode() const override { return SPEAKER_MODE_STEREO; }

	virtual void lock() override { mutex.lock(); }
	virtual void unlock() override { mutex.unlock(); }
	virtual void finish() override;
};
