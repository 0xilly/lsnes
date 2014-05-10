#ifndef _inthread__hpp__included__
#define _inthread__hpp__included__

#include <list>
#include <cstdint>
#include <string>

class voice_commentary
{
public:
	enum external_stream_format
	{
		EXTFMT_SOX,
		EXTFMT_OGGOPUS
	};

	struct playback_stream_info
	{
		uint64_t id;
		uint64_t base;
		uint64_t length;
	};

	voice_commentary();
	~voice_commentary();
	void init();
	void kill();
	void frame_number(uint64_t newframe, double rate);
	bool collection_loaded();
	std::list<playback_stream_info> get_stream_info();
	void play_stream(uint64_t id);
	void export_stream(uint64_t id, const std::string& filename, external_stream_format fmt);
	uint64_t import_stream(uint64_t ts, const std::string& filename, external_stream_format fmt);
	void delete_stream(uint64_t id);
	void export_superstream(const std::string& filename);
	void load_collection(const std::string& filename);
	void unload_collection();
	void alter_timebase(uint64_t id, uint64_t ts);
	uint64_t parse_timebase(const std::string& n);
	double ts_seconds(uint64_t ts);
	float get_gain(uint64_t id);
	void set_gain(uint64_t id, float gain);
	void set_active_flag(bool flag);
private:
	void* internal;
};

#endif
