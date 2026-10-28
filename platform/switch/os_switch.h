#pragma once

#include "core/input/input_event.h"
#include "drivers/unix/os_unix.h"

#include "audio_driver_switch.h"
#include "nx_wrap.h"

class OS_Switch : public OS_Unix {
	MainLoop *main_loop = nullptr;

	AudioDriverSwitch driver_switch_audio;

	bool pad_connected = false;
	uint32_t pad_prev_buttons = 0;

	void _process_pad();

protected:
	virtual void initialize() override;
	virtual void finalize() override;

	virtual void initialize_joypads() override;

	virtual void delete_main_loop() override;
	virtual void set_main_loop(MainLoop *p_main_loop) override;

public:
	virtual String get_identifier() const override;
	virtual String get_name() const override;
	virtual String get_distribution_name() const override;
	virtual String get_executable_path() const override;
	virtual String get_version() const override;

	virtual MainLoop *get_main_loop() const override;

	virtual String get_config_path() const override;
	virtual String get_data_path() const override;
	virtual String get_cache_path() const override;

	virtual String get_unique_id() const override;
	virtual String get_processor_name() const override;
	virtual bool _check_internal_feature_support(const String &p_feature) override;

	void run();

	OS_Switch();
};
