#include "audio_driver_switch.h"

Error AudioDriverSwitch::init() {
	if (!nx::audio_init()) {
		return ERR_CANT_OPEN;
	}
	initialized = true;
	mix_rate = nx::audio_sample_rate();
	active.clear();
	exit_thread.clear();
	thread.start(AudioDriverSwitch::thread_func, this);
	return OK;
}

void AudioDriverSwitch::thread_func(void *p_udata) {
	AudioDriverSwitch *ad = static_cast<AudioDriverSwitch *>(p_udata);

	while (!ad->exit_thread.is_set()) {
		if (ad->active.is_set()) {
			ad->lock();
			ad->start_counting_ticks();
			ad->audio_server_process(nx::AUDIO_FRAMES_PER_BUFFER, ad->samples_in);
			ad->stop_counting_ticks();
			ad->unlock();

			// The mixer emits 32-bit samples with the signal in the high bits.
			for (uint32_t i = 0; i < nx::AUDIO_FRAMES_PER_BUFFER * 2; i++) {
				ad->samples_out[i] = (int16_t)(ad->samples_in[i] >> 16);
			}
		} else {
			memset(ad->samples_out, 0, sizeof(ad->samples_out));
		}
		nx::audio_submit(ad->samples_out);
	}
}

void AudioDriverSwitch::start() {
	active.set();
}

void AudioDriverSwitch::finish() {
	if (!initialized) {
		return;
	}
	exit_thread.set();
	if (thread.is_started()) {
		thread.wait_to_finish();
	}
	nx::audio_exit();
	initialized = false;
}
