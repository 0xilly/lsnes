#ifndef _opus_hpp_included_
#define _opus_hpp_included_

#include <cstdint>
#include <cstdlib>
#include <stdexcept>

namespace opus
{
struct bad_argument : public std::runtime_error { bad_argument(); };
struct buffer_too_small : public std::runtime_error { buffer_too_small(); };
struct internal_error : public std::runtime_error { internal_error(); };
struct invalid_packet : public std::runtime_error { invalid_packet(); };
struct unimplemented : public std::runtime_error { unimplemented(); };
struct invalid_state : public std::runtime_error { invalid_state(); };

class encoder;
class decoder;

template<typename T> struct generic_eget
{
	typedef T erettype;
	T operator()(encoder& e) const;
};

template<typename T> struct generic_get
{
	typedef T drettype;
	typedef T erettype;
	T operator()(decoder& e) const;
	T operator()(encoder& e) const;
};

template<typename T> struct generic_dget
{
	typedef T drettype;
	T operator()(decoder& e) const;
};

struct samplerate
{
	samplerate(int32_t _fs) { fs = _fs; }
	static samplerate r8k;
	static samplerate r12k;
	static samplerate r16k;
	static samplerate r24k;
	static samplerate r48k;
	operator int32_t() { return fs; }
	typedef samplerate erettype;
	samplerate operator()(encoder& e) const;
private:
	int32_t fs;
};

struct compexity
{
	compexity(int32_t _c) { c = _c; }
	operator int32_t() { return c; }
	typedef generic_eget<compexity> get;
	typedef void erettype;
	void operator()(encoder& e) const;
private:
	int32_t c;
};

struct bitrate
{
	bitrate(int32_t _b) { b = _b; }
	static bitrate _auto;
	static bitrate max;
	operator int32_t() { return b; }
	typedef generic_eget<bitrate> get;
	typedef void erettype;
	void operator()(encoder& e) const;
private:
	int32_t b;
};

struct vbr
{
	vbr(bool _v) { v = _v; }
	static vbr cbr;
	static vbr _vbr;
	operator bool() { return v; }
	typedef generic_eget<vbr> get;
	typedef void erettype;
	void operator()(encoder& e) const;
private:
	bool v;
};

struct vbr_constraint
{
	vbr_constraint(bool _c) { c = _c; }
	static vbr_constraint unconstrained;
	static vbr_constraint constrained;
	operator bool() { return c; }
	typedef generic_eget<vbr_constraint> get;
	typedef void erettype;
	void operator()(encoder& e) const;
private:
	bool c;
};

struct force_channels
{
	force_channels(int32_t _f) { f = _f; }
	static force_channels _auto;
	static force_channels mono;
	static force_channels stereo;
	operator int32_t() { return f; }
	typedef generic_eget<force_channels> get;
	typedef void erettype;
	void operator()(encoder& e) const;
private:
	int32_t f;
};

struct max_bandwidth
{
	max_bandwidth(int32_t _bw) { bw = _bw; }
	static max_bandwidth narrow;
	static max_bandwidth medium;
	static max_bandwidth wide;
	static max_bandwidth superwide;
	static max_bandwidth full;
	operator int32_t() { return bw; }
	typedef generic_eget<max_bandwidth> get;
	typedef void erettype;
	void operator()(encoder& e) const;
private:
	int32_t bw;
};

struct bandwidth
{
	bandwidth(int32_t _bw) { bw = _bw; }
	static bandwidth _auto;
	static bandwidth narrow;
	static bandwidth medium;
	static bandwidth wide;
	static bandwidth superwide;
	static bandwidth full;
	operator int32_t() { return bw; }
	typedef generic_get<bandwidth> get;
	typedef void erettype;
	void operator()(encoder& e) const;
private:
	int32_t bw;
};

struct signal
{
	signal(int32_t _s) { s = _s; }
	static signal _auto;
	static signal music;
	static signal voice;
	operator int32_t() { return s; }
	typedef generic_eget<signal> get;
	typedef void erettype;
	void operator()(encoder& e) const;
private:
	int32_t s;
};

struct application
{
	application(int32_t _app) { app = _app; };
	static application audio;
	static application voice;
	static application lowdelay;
	operator int32_t() { return app; }
	typedef generic_eget<application> get;
	typedef void erettype;
	void operator()(encoder& e) const;
private:
	int32_t app;
};

struct _lookahead
{
	_lookahead() { l = 0; }
	_lookahead(int32_t _l) { l = _l; }
	operator int32_t() { return l; }
	typedef _lookahead erettype;
	_lookahead operator()(encoder& e) const;
private:
	int32_t l;
};
extern _lookahead lookahead;

struct fec
{
	fec(bool _f) { f = _f; }
	static fec disabled;
	static fec enabled;
	operator bool() { return f; }
	typedef generic_eget<fec> get;
	typedef void erettype;
	void operator()(encoder& e) const;
private:
	bool f;
};

struct lossperc
{
	lossperc(int32_t _loss) { loss = _loss; };
	operator int32_t() { return loss; }
	typedef generic_eget<lossperc> get;
	typedef void erettype;
	void operator()(encoder& e) const;
private:
	int32_t loss;
};

struct dtx
{
	dtx(bool _d) { d = _d; }
	static dtx disabled;
	static dtx enabled;
	operator bool() { return d; }
	typedef generic_eget<dtx> get;
	typedef void erettype;
	void operator()(encoder& e) const;
private:
	bool d;
};

struct lsbdepth
{
	lsbdepth(int32_t _depth) { depth = _depth; };
	static lsbdepth d8;
	static lsbdepth d16;
	static lsbdepth d24;
	operator int32_t() { return depth; }
	typedef generic_eget<lsbdepth> get;
	typedef void erettype;
	void operator()(encoder& e) const;
private:
	int32_t depth;
};

struct _pktduration
{
	_pktduration() { d = 0; }
	_pktduration(int32_t _d) { d = _d; }
	operator int32_t() { return d; }
	typedef _pktduration erettype;
	_pktduration operator()(encoder& e) const;
private:
	int32_t d;
};
extern _pktduration pktduration;

struct _reset
{
	typedef void drettype;
	typedef void erettype;
	void operator()(decoder& e) const;
	void operator()(encoder& e) const;
};
extern _reset reset;

struct _finalrange
{
	_finalrange() { f = 0; }
	_finalrange(uint32_t _f) { f = _f; }
	operator uint32_t() { return f; }
	typedef _finalrange drettype;
	typedef _finalrange erettype;
	_finalrange operator()(decoder& e) const;
	_finalrange operator()(encoder& e) const;
private:
	uint32_t f;
};
extern _finalrange finalrange;

struct _pitch
{
	_pitch() { p = 0; }
	_pitch(int32_t _p) { p = _p; }
	operator int32_t() { return p; }
	typedef _pitch drettype;
	typedef _pitch erettype;
	_pitch operator()(encoder& e) const;
	_pitch operator()(decoder& e) const;
private:
	int32_t p;
};
extern _pitch pitch;

struct gain
{
	gain(int32_t _g) { g = _g; };
	operator int32_t() { return g; }
	typedef generic_dget<gain> get;
	typedef void drettype;
	void operator()(decoder& d) const;
private:
	int32_t g;
};

struct set_control_int
{
	set_control_int(int32_t _ctl, int32_t _val) { ctl = _ctl; val = _val; }
	typedef void drettype;
	typedef void erettype;
	void operator()(encoder& e) const;
	void operator()(decoder& e) const;
private:
	int32_t ctl;
	int32_t val;
};

struct get_control_int
{
	get_control_int(int32_t _ctl) { ctl = _ctl; }
	typedef int32_t drettype;
	typedef int32_t erettype;
	int32_t operator()(encoder& e) const;
	int32_t operator()(decoder& e) const;
private:
	int32_t ctl;
};

class encoder
{
public:
	encoder(samplerate rate, bool stereo, application app, char* memory = NULL);
	encoder(void* state, bool _stereo);
	~encoder();
	encoder(const encoder& e);
	encoder(const encoder& e, char* memory);
	static size_t size(bool stereo);
	encoder& operator=(const encoder& e);
	template<typename T> typename T::erettype ctl(const T& c) { return c(*this); }
	size_t encode(const int16_t* in, uint32_t inframes, unsigned char* out, uint32_t maxout);
	size_t encode(const float* in, uint32_t inframes, unsigned char* out, uint32_t maxout);
	bool is_stereo() { return stereo; }
	void* getmem() { return memory; }
private:
	void* memory;
	bool stereo;
	bool user;
};

class decoder
{
public:
	decoder(samplerate rate, bool stereo, char* memory = NULL);
	decoder(void* state, bool _stereo);
	~decoder();
	decoder(const decoder& e);
	decoder(const decoder& e, char* memory);
	static size_t size(bool stereo);
	decoder& operator=(const decoder& e);
	template<typename T> typename T::drettype ctl(const T& c) { return c(*this); }
	size_t decode(const unsigned char* in, uint32_t insize, int16_t* out, uint32_t maxframes,
		bool decode_fec = false);
	size_t decode(const unsigned char* in, uint32_t insize, float* out, uint32_t maxframes,
		bool decode_fec = false);
	uint32_t get_nb_samples(const unsigned char* buf, size_t bufsize);
	bool is_stereo() { return stereo; }
	void* getmem() { return memory; }
private:
	void* memory;
	bool stereo;
	bool user;
};

uint32_t packet_get_nb_frames(const unsigned char* packet, size_t len);
uint32_t packet_get_samples_per_frame(const unsigned char* data, samplerate fs); 
}
#endif