#include "core/dispatch.hpp"
#include "core/globalwrap.hpp"
#include "core/misc.hpp"

#include <sstream>
#include <iomanip>
#include <cmath>

#define START_EH_BLOCK try {
#define END_EH_BLOCK(obj, call) } catch(std::bad_alloc& e) { \
	OOM_panic(); \
	} catch(std::exception& e) { \
		messages << messages << "[dumper " << obj->get_name() << "] Warning: " call ": " << e.what() \
			<< std::endl; \
	} catch(int code) { \
		messages << messages << "[dumper " << obj->get_name() << "] Warning: " call ": Error code #" << code \
			<< std::endl; \
	}

gameinfo_struct::gameinfo_struct() throw(std::bad_alloc)
{
	length = 0;
	rerecords = "0";
}

std::string gameinfo_struct::get_readable_time(unsigned digits) const throw(std::bad_alloc)
{
	double bias = 0.5 * pow(10, -static_cast<int>(digits));
	double len = length + bias;
	std::ostringstream str;
	if(length >= 3600) {
		double hours = floor(len / 3600);
		str << hours << ":";
		len -= hours * 3600;
	}
	double minutes = floor(len / 60);
	len -= minutes * 60;
	double seconds = floor(len);
	len -= seconds;
	str << std::setw(2) << std::setfill('0') << minutes << ":" << seconds;
	if(digits > 0)
		str << ".";
	while(digits > 0) {
		len = 10 * len;
		str << '0' + static_cast<int>(len);
		len -= floor(len);
		digits--;
	}
}

size_t gameinfo_struct::get_author_count() const throw()
{
	return authors.size();
}

std::string gameinfo_struct::get_author_short(size_t idx) const throw(std::bad_alloc)
{
	if(idx >= authors.size())
		return "";
	const std::pair<std::string, std::string>& x = authors[idx];
	if(x.second != "")
		return x.second;
	else
		return x.first;
}

uint64_t gameinfo_struct::get_rerecords() const throw()
{
	uint64_t v = 0;
	uint64_t max = 0xFFFFFFFFFFFFFFFFULL;
	for(size_t i = 0; i < rerecords.length(); i++) {
		if(v < max / 10)
			//No risk of overflow.
			v = v * 10 + static_cast<unsigned>(rerecords[i] - '0');
		else if(v == max / 10) {
			//THis may overflow.
			v = v * 10;
			if(v + static_cast<unsigned>(rerecords[i] - '0') < v)
				return max;
			v = v + static_cast<unsigned>(rerecords[i] - '0');
		} else
			//Definite overflow.
			return max;
	}
	return v;
}

namespace
{
	globalwrap<std::list<information_dispatch*>> dispatch;
	uint32_t srate_n = 32000;
	uint32_t srate_d = 1;
	struct gameinfo_struct sgi;
	information_dispatch* exclusive_key = NULL;
	int32_t vc_xoffset = 0;
	int32_t vc_yoffset = 0;
	uint32_t vc_hscl = 1;
	uint32_t vc_vscl = 1;
	bool recursive = false;
}

information_dispatch::information_dispatch(const std::string& name) throw(std::bad_alloc)
{
	target_name = name;
	dispatch().push_back(this);
	known_if_dumper = false;
	marked_as_dumper = false;
	notified_as_dumper = false;
	grabbing_keys = false;
}

information_dispatch::~information_dispatch() throw()
{
	for(auto i = dispatch().begin(); i != dispatch().end(); ++i) {
		if(*i == this) {
			dispatch().erase(i);
			break;
		}
	}
	if(notified_as_dumper)
		for(auto& i : dispatch()) {
			START_EH_BLOCK
			i->on_destroy_dumper(target_name);
			END_EH_BLOCK(i, "on_destroy_dumper");
		}
}

void information_dispatch::on_close()
{
	//Do nothing.
}

void information_dispatch::do_close() throw()
{
	for(auto& i : dispatch()) {
		START_EH_BLOCK
		i->on_close();
		END_EH_BLOCK(i, "on_close");
	}
}

void information_dispatch::on_click(int32_t x, int32_t y, uint32_t buttonmask)
{
	//Do nothing.
}

void information_dispatch::do_click(int32_t x, int32_t y, uint32_t buttonmask) throw()
{
	x = (x - vc_xoffset) / vc_hscl;
	y = (y - vc_yoffset) / vc_vscl;
	for(auto& i : dispatch()) {
		START_EH_BLOCK
		i->on_click(x, y, buttonmask);
		END_EH_BLOCK(i, "on_click");
	}
}

void information_dispatch::on_sound_unmute(bool unmuted)
{
	//Do nothing.
}

void information_dispatch::do_sound_unmute(bool unmuted) throw()
{
	for(auto& i : dispatch()) {
		START_EH_BLOCK
		i->on_sound_unmute(unmuted);
		END_EH_BLOCK(i, "on_sound_unmute");
	}
}

void information_dispatch::on_sound_change(const std::string& dev)
{
	//Do nothing.
}

void information_dispatch::do_sound_change(const std::string& dev) throw()
{
	for(auto& i : dispatch()) {
		START_EH_BLOCK
		i->on_sound_change(dev);
		END_EH_BLOCK(i, "on_sound_change");
	}
}

void information_dispatch::on_mode_change(bool readonly)
{
	//Do nothing.
}

void information_dispatch::do_mode_change(bool readonly) throw()
{
	for(auto& i : dispatch()) {
		START_EH_BLOCK
		i->on_mode_change(readonly);
		END_EH_BLOCK(i, "on_mode_change");
	}
}

void information_dispatch::on_autohold_update(unsigned pid, unsigned ctrlnum, bool newstate)
{
	//Do nothing.
}

void information_dispatch::do_autohold_update(unsigned pid, unsigned ctrlnum, bool newstate) throw()
{
	for(auto& i : dispatch()) {
		START_EH_BLOCK
		i->on_autohold_update(pid, ctrlnum, newstate);
		END_EH_BLOCK(i, "on_autohold_update");
	}
}

void information_dispatch::on_autohold_reconfigure()
{
	//Do nothing.
}

void information_dispatch::do_autohold_reconfigure() throw()
{
	for(auto& i : dispatch()) {
		START_EH_BLOCK
		i->on_autohold_reconfigure();
		END_EH_BLOCK(i, "on_autohold_reconfigure");
	}
}

void information_dispatch::on_setting_change(const std::string& setting, const std::string& value)
{
	//Do nothing.
}

void information_dispatch::do_setting_change(const std::string& setting, const std::string& value) throw()
{
	for(auto& i : dispatch()) {
		START_EH_BLOCK
		i->on_setting_change(setting, value);
		END_EH_BLOCK(i, "on_setting_change");
	}
}

void information_dispatch::on_setting_clear(const std::string& setting)
{
	//Do nothing.
}

void information_dispatch::do_setting_clear(const std::string& setting) throw()
{
	for(auto& i : dispatch()) {
		START_EH_BLOCK
		i->on_setting_clear(setting);
		END_EH_BLOCK(i, "on_setting_clear");
	}
}


void information_dispatch::on_raw_frame(const uint32_t* raw, bool hires, bool interlaced, bool overscan,
	unsigned region)
{
	//Do nothing.
}

void information_dispatch::do_raw_frame(const uint32_t* raw, bool hires, bool interlaced, bool overscan,
	unsigned region) throw()
{
	update_dumpers();
	for(auto& i : dispatch()) {
		START_EH_BLOCK
		i->on_raw_frame(raw, hires, interlaced, overscan, region);
		END_EH_BLOCK(i, "on_raw_frame");
	}
}

void information_dispatch::on_frame(struct lcscreen& _frame, uint32_t fps_n, uint32_t fps_d)
{
	//Do nothing.
}

void information_dispatch::do_frame(struct lcscreen& _frame, uint32_t fps_n, uint32_t fps_d) throw()
{
	update_dumpers();
	for(auto& i : dispatch()) {
		START_EH_BLOCK
		i->on_frame(_frame, fps_n, fps_d);
		END_EH_BLOCK(i, "on_frame");
	}
}

void information_dispatch::on_sample(short l, short r)
{
	//Do nothing.
}

void information_dispatch::do_sample(short l, short r) throw()
{
	update_dumpers();
	for(auto& i : dispatch()) {
		START_EH_BLOCK
		i->on_sample(l, r);
		END_EH_BLOCK(i, "on_sample");
	}
}

void information_dispatch::on_dump_end()
{
	//Do nothing.
}

void information_dispatch::do_dump_end() throw()
{
	update_dumpers();
	for(auto& i : dispatch()) {
		START_EH_BLOCK
		i->on_dump_end();
		END_EH_BLOCK(i, "on_dump_end");
	}
}

void information_dispatch::on_sound_rate(uint32_t rate_n, uint32_t rate_d)
{
	if(!known_if_dumper) {
		marked_as_dumper = get_dumper_flag();
		known_if_dumper = true;
	}
	if(marked_as_dumper) {
		messages << "[dumper " << get_name() << "] Warning: Sound sample rate changing not supported!"
			<< std::endl;
	}
}

void information_dispatch::do_sound_rate(uint32_t rate_n, uint32_t rate_d) throw()
{
	update_dumpers();
	uint32_t g = gcd(rate_n, rate_d);
	rate_n /= g;
	rate_d /= g;
	if(rate_n == srate_n && rate_d == srate_d)
		return;
	srate_n = rate_n;
	srate_d = rate_d;
	for(auto& i : dispatch()) {
		START_EH_BLOCK
		i->on_sound_rate(rate_n, rate_d);
		END_EH_BLOCK(i, "on_sound_rate");
	}
}

std::pair<uint32_t, uint32_t> information_dispatch::get_sound_rate() throw()
{
	return std::make_pair(srate_n, srate_d);
}

void information_dispatch::on_gameinfo(const struct gameinfo_struct& gi)
{
	//Do nothing.
}

void information_dispatch::do_gameinfo(const struct gameinfo_struct& gi) throw()
{
	update_dumpers();
	try {
		sgi = gi;
	} catch(...) {
		OOM_panic();
	}
	for(auto& i : dispatch()) {
		START_EH_BLOCK
		i->on_gameinfo(sgi);
		END_EH_BLOCK(i, "on_gameinfo");
	}
}

const struct gameinfo_struct& information_dispatch::get_gameinfo() throw()
{
	return sgi;
}

bool information_dispatch::get_dumper_flag() throw()
{
	return false;
}

void information_dispatch::on_new_dumper(const std::string& dumper)
{
	//Do nothing.
}

void information_dispatch::on_destroy_dumper(const std::string& dumper)
{
	//Do nothing.
}

unsigned information_dispatch::get_dumper_count() throw()
{
	update_dumpers(true);
	unsigned count = 0;
	for(auto& i : dispatch())
		if(i->marked_as_dumper)
			count++;
	if(!recursive) {
		recursive = true;
		update_dumpers();
		recursive = false;
	}
	return count;
}

std::set<std::string> information_dispatch::get_dumpers() throw(std::bad_alloc)
{
	update_dumpers();
	std::set<std::string> r;
	try {
		for(auto& i : dispatch())
			if(i->notified_as_dumper)
				r.insert(i->get_name());
	} catch(...) {
		OOM_panic();
	}
	return r;
}

void information_dispatch::on_key_event(const modifier_set& modifiers, keygroup& keygroup, unsigned subkey,
	bool polarity, const std::string& name)
{
	//Do nothing.
}

void information_dispatch::do_key_event(const modifier_set& modifiers, keygroup& keygroup, unsigned subkey,
	bool polarity, const std::string& name) throw()
{
	if(exclusive_key) {
		START_EH_BLOCK
		exclusive_key->on_key_event(modifiers, keygroup, subkey, polarity, name);
		END_EH_BLOCK(exclusive_key, "on_key_event");
		return;
	}
	for(auto& i : dispatch()) {
		START_EH_BLOCK
		i->on_key_event(modifiers, keygroup, subkey, polarity, name);
		END_EH_BLOCK(i, "on_key_event");
	}
}

void information_dispatch::grab_keys() throw()
{
	if(grabbing_keys)
		return;
	exclusive_key = this;
	grabbing_keys = true;
}

void information_dispatch::ungrab_keys() throw()
{
	if(!grabbing_keys)
		return;
	exclusive_key = NULL;
	grabbing_keys = false;
	for(auto& i : dispatch())
		if(i->grabbing_keys) {
			exclusive_key = i;
			break;
		}
}

const std::string& information_dispatch::get_name() throw()
{
	return target_name;
}

void information_dispatch::update_dumpers(bool nocalls) throw()
{
	for(auto& i : dispatch()) {
		if(!i->known_if_dumper) {
			i->marked_as_dumper = i->get_dumper_flag();
			i->known_if_dumper = true;
		}
		if(i->marked_as_dumper && !i->notified_as_dumper && !nocalls) {
			for(auto& j : dispatch()) {
				START_EH_BLOCK
				j->on_new_dumper(i->target_name);
				END_EH_BLOCK(j, "on_new_dumper");
			}
			i->notified_as_dumper = true;
		}
	}
}

void information_dispatch::do_click_compensation(uint32_t xoffset, uint32_t yoffset, uint32_t hscl, uint32_t vscl)
{
	vc_xoffset = xoffset;
	vc_yoffset = yoffset;
	vc_hscl = hscl;
	vc_vscl = vscl;
}

void information_dispatch::on_screen_resize(screen& scr, uint32_t w, uint32_t h)
{
	//Do nothing.
}

void information_dispatch::do_screen_resize(screen& scr, uint32_t w, uint32_t h) throw()
{
	for(auto& i : dispatch()) {
		START_EH_BLOCK
		i->on_screen_resize(scr, w, h);
		END_EH_BLOCK(i, "on_screen_resize");
	}
}

void information_dispatch::on_render_update_start()
{
	//Do nothing.
}

void information_dispatch::do_render_update_start() throw()
{
	for(auto& i : dispatch()) {
		START_EH_BLOCK
		i->on_render_update_start();
		END_EH_BLOCK(i, "on_render_update_start");
	}
}

void information_dispatch::on_render_update_end()
{
	//Do nothing.
}

void information_dispatch::do_render_update_end() throw()
{
	for(auto& i : dispatch()) {
		START_EH_BLOCK
		i->on_render_update_end();
		END_EH_BLOCK(i, "on_render_update_end");
	}
}

void information_dispatch::on_status_update()
{
	//Do nothing.
}

void information_dispatch::do_status_update() throw()
{
	for(auto& i : dispatch()) {
		START_EH_BLOCK
		i->on_status_update();
		END_EH_BLOCK(i, "on_status_update");
	}
}