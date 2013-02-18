#ifdef WITH_OPUS_CODEC
#define OPUS_BUILD
#include "opus.hpp"
#include <opus.h>
#include <opus_defines.h>
#include <sstream>
#include <cstring>

//Some of these might not be in stable.
#ifndef OPUS_SET_GAIN_REQUEST
#define OPUS_SET_GAIN_REQUEST 4034
#endif
#ifndef OPUS_GET_GAIN_REQUEST
#define OPUS_GET_GAIN_REQUEST 4045
#endif
#ifndef OPUS_SET_LSB_DEPTH_REQUEST
#define OPUS_SET_LSB_DEPTH_REQUEST 4036
#endif
#ifndef OPUS_GET_LSB_DEPTH_REQUEST
#define OPUS_GET_LSB_DEPTH_REQUEST 4037
#endif
#ifndef OPUS_GET_LAST_PACKET_DURATION_REQUEST
#define OPUS_GET_LAST_PACKET_DURATION_REQUEST 4039
#endif

namespace opus
{
samplerate samplerate::r8k(8000);
samplerate samplerate::r12k(12000);
samplerate samplerate::r16k(16000);
samplerate samplerate::r24k(24000);
samplerate samplerate::r48k(48000);
bitrate bitrate::_auto(OPUS_AUTO);
bitrate bitrate::max(OPUS_BITRATE_MAX);
vbr vbr::cbr(false);
vbr vbr::_vbr(true);
vbr_constraint vbr_constraint::unconstrained(false);
vbr_constraint vbr_constraint::constrained(true);
force_channels force_channels::_auto(OPUS_AUTO);
force_channels force_channels::mono(1);
force_channels force_channels::stereo(2);
max_bandwidth max_bandwidth::narrow(OPUS_BANDWIDTH_NARROWBAND);
max_bandwidth max_bandwidth::medium(OPUS_BANDWIDTH_MEDIUMBAND);
max_bandwidth max_bandwidth::wide(OPUS_BANDWIDTH_WIDEBAND);
max_bandwidth max_bandwidth::superwide(OPUS_BANDWIDTH_SUPERWIDEBAND);
max_bandwidth max_bandwidth::full(OPUS_BANDWIDTH_FULLBAND);
bandwidth bandwidth::_auto(OPUS_AUTO);
bandwidth bandwidth::narrow(OPUS_BANDWIDTH_NARROWBAND);
bandwidth bandwidth::medium(OPUS_BANDWIDTH_MEDIUMBAND);
bandwidth bandwidth::wide(OPUS_BANDWIDTH_WIDEBAND);
bandwidth bandwidth::superwide(OPUS_BANDWIDTH_SUPERWIDEBAND);
bandwidth bandwidth::full(OPUS_BANDWIDTH_FULLBAND);
signal signal::_auto(OPUS_AUTO);
signal signal::music(OPUS_SIGNAL_MUSIC);
signal signal::voice(OPUS_SIGNAL_VOICE);
application application::audio(OPUS_APPLICATION_AUDIO);
application application::voice(OPUS_APPLICATION_VOIP);
application application::lowdelay(OPUS_APPLICATION_RESTRICTED_LOWDELAY);
fec fec::disabled(false);
fec fec::enabled(true);
dtx dtx::disabled(false);
dtx dtx::enabled(true);
lsbdepth lsbdepth::d8(8);
lsbdepth lsbdepth::d16(16);
lsbdepth lsbdepth::d24(24);
_reset reset;
_finalrange finalrange;
_pitch pitch;
_pktduration pktduration;
_lookahead lookahead;

bad_argument::bad_argument()
	: std::runtime_error("Invalid argument") {}

buffer_too_small::buffer_too_small()
	: std::runtime_error("Buffer too small") {}

internal_error::internal_error()
	: std::runtime_error("Internal error") {}

invalid_packet::invalid_packet()
	: std::runtime_error("Invalid packet") {}

unimplemented::unimplemented()
	: std::runtime_error("Not implemented") {}

invalid_state::invalid_state()
	: std::runtime_error("Invalid state") {}

int32_t throwex(int32_t ret)
{
	if(ret >= 0)
		return ret;
	if(ret == OPUS_BAD_ARG)
		throw bad_argument();
	if(ret == OPUS_BUFFER_TOO_SMALL)
		throw buffer_too_small();
	if(ret == OPUS_INTERNAL_ERROR)
		throw internal_error();
	if(ret == OPUS_INVALID_PACKET)
		throw invalid_packet();
	if(ret == OPUS_UNIMPLEMENTED)
		throw unimplemented();
	if(ret == OPUS_INVALID_STATE)
		throw invalid_state();
	if(ret == OPUS_ALLOC_FAIL)
		throw std::bad_alloc();
	std::ostringstream s;
	s << "Unknown error code " << ret << " from libopus.";
	throw std::runtime_error(s.str());
}

template<typename T> struct get_ctlnum { const static int32_t num; };

template<> const int32_t get_ctlnum<compexity>::num = OPUS_GET_COMPLEXITY_REQUEST;
template<> const int32_t get_ctlnum<bitrate>::num = OPUS_GET_BITRATE_REQUEST;
template<> const int32_t get_ctlnum<vbr>::num = OPUS_GET_VBR_REQUEST;
template<> const int32_t get_ctlnum<vbr_constraint>::num = OPUS_GET_VBR_CONSTRAINT_REQUEST;
template<> const int32_t get_ctlnum<force_channels>::num = OPUS_GET_FORCE_CHANNELS_REQUEST;
template<> const int32_t get_ctlnum<max_bandwidth>::num = OPUS_GET_MAX_BANDWIDTH_REQUEST;
template<> const int32_t get_ctlnum<bandwidth>::num = OPUS_GET_BANDWIDTH_REQUEST;
template<> const int32_t get_ctlnum<signal>::num = OPUS_GET_SIGNAL_REQUEST;
template<> const int32_t get_ctlnum<application>::num = OPUS_GET_APPLICATION_REQUEST;
template<> const int32_t get_ctlnum<fec>::num = OPUS_GET_INBAND_FEC_REQUEST;
template<> const int32_t get_ctlnum<lossperc>::num = OPUS_GET_PACKET_LOSS_PERC_REQUEST;
template<> const int32_t get_ctlnum<dtx>::num = OPUS_GET_DTX_REQUEST;
template<> const int32_t get_ctlnum<lsbdepth>::num = OPUS_GET_LSB_DEPTH_REQUEST;
template<> const int32_t get_ctlnum<gain>::num = OPUS_GET_GAIN_REQUEST;

OpusEncoder* E(encoder& e) { return reinterpret_cast<OpusEncoder*>(e.getmem()); }
OpusDecoder* D(decoder& d) { return reinterpret_cast<OpusDecoder*>(d.getmem()); }


template<typename T> T generic_ctl(encoder& e, int32_t ctl)
{
	T val;
	throwex(opus_encoder_ctl(E(e), ctl, &val));
	return val;
}

template<typename T> T generic_ctl(encoder& e, int32_t ctl, T val)
{
	throwex(opus_encoder_ctl(E(e), ctl, val));
}

template<typename T> T generic_ctl(decoder& d, int32_t ctl)
{
	T val;
	throwex(opus_decoder_ctl(D(d), ctl, &val));
	return val;
}

template<typename T> T generic_ctl(decoder& d, int32_t ctl, T val)
{
	throwex(opus_decoder_ctl(D(d), ctl, val));
}

template<typename T> T do_generic_get(encoder& e)
{
	return T(generic_ctl<int32_t>(e, get_ctlnum<T>::num));
}

template<typename T> T do_generic_get(decoder& d)
{
	return T(generic_ctl<int32_t>(d, get_ctlnum<T>::num));
}

template<typename T> T generic_eget<T>::operator()(encoder& e) const
{
	return do_generic_get<T>(e);
}

template<typename T> T generic_dget<T>::operator()(decoder& d) const
{
	return do_generic_get<T>(d);
}

template<typename T> T generic_get<T>::operator()(decoder& d) const
{
	return do_generic_get<T>(d);
}

template<typename T> T generic_get<T>::operator()(encoder& e) const
{
	return do_generic_get<T>(e);
}

samplerate samplerate::operator()(encoder& e) const
{
	return samplerate(generic_ctl<int32_t>(e, OPUS_GET_SAMPLE_RATE_REQUEST));
}

void compexity::operator()(encoder& e) const
{
	generic_ctl(e, OPUS_SET_COMPLEXITY_REQUEST, c);
}

void bitrate::operator()(encoder& e) const
{
	generic_ctl(e, OPUS_SET_BITRATE_REQUEST, b);
}

void vbr::operator()(encoder& e) const
{
	generic_ctl<int32_t>(e, OPUS_SET_VBR_REQUEST, v ? 1 : 0);
}

void vbr_constraint::operator()(encoder& e) const
{
	generic_ctl<int32_t>(e, OPUS_SET_VBR_CONSTRAINT_REQUEST, c ? 1 : 0);
}

void force_channels::operator()(encoder& e) const
{
	generic_ctl(e, OPUS_SET_FORCE_CHANNELS_REQUEST, f);
}

void max_bandwidth::operator()(encoder& e) const
{
	generic_ctl(e, OPUS_SET_MAX_BANDWIDTH_REQUEST, bw);
}

void bandwidth::operator()(encoder& e) const
{
	generic_ctl(e, OPUS_SET_BANDWIDTH_REQUEST, bw);
}

void signal::operator()(encoder& e) const
{
	generic_ctl(e, OPUS_SET_SIGNAL_REQUEST, s);
}

void application::operator()(encoder& e) const
{
	generic_ctl(e, OPUS_SET_APPLICATION_REQUEST, app);
}

_lookahead _lookahead::operator()(encoder& e) const
{
	return _lookahead(generic_ctl<int32_t>(e, OPUS_GET_LOOKAHEAD_REQUEST));
}

void fec::operator()(encoder& e) const
{
	generic_ctl<int32_t>(e, OPUS_SET_INBAND_FEC_REQUEST, f ? 1 : 0);
}

void lossperc::operator()(encoder& e) const
{
	generic_ctl(e, OPUS_SET_PACKET_LOSS_PERC_REQUEST, loss);
}

void dtx::operator()(encoder& e) const
{
	generic_ctl<int32_t>(e, OPUS_SET_DTX_REQUEST, d ? 1 : 0);
}

void lsbdepth::operator()(encoder& e) const
{
	generic_ctl(e, OPUS_SET_LSB_DEPTH_REQUEST, depth);
}

_pktduration _pktduration::operator()(encoder& e) const
{
	return _pktduration(generic_ctl<int32_t>(e, OPUS_GET_LAST_PACKET_DURATION_REQUEST));
}

void _reset::operator()(decoder& d) const
{
	throwex(opus_decoder_ctl(D(d), OPUS_RESET_STATE));
}

void _reset::operator()(encoder& e) const
{
	throwex(opus_encoder_ctl(E(e), OPUS_RESET_STATE));
}

_finalrange _finalrange::operator()(decoder& d) const
{
	return _finalrange(generic_ctl<uint32_t>(d, OPUS_GET_FINAL_RANGE_REQUEST));
}

_finalrange _finalrange::operator()(encoder& e) const
{
	return _finalrange(generic_ctl<uint32_t>(e, OPUS_GET_FINAL_RANGE_REQUEST));
}

_pitch _pitch::operator()(encoder& e) const
{
	return _pitch(generic_ctl<uint32_t>(e, OPUS_GET_PITCH_REQUEST));
}

_pitch _pitch::operator()(decoder& d) const
{
	return _pitch(generic_ctl<uint32_t>(d, OPUS_GET_PITCH_REQUEST));
}

void gain::operator()(decoder& d) const
{
	generic_ctl(d, OPUS_SET_GAIN_REQUEST, g);
}

void set_control_int::operator()(encoder& e) const
{
	generic_ctl(e, ctl, val);
}

void set_control_int::operator()(decoder& d) const
{
	generic_ctl(d, ctl, val);
}

int32_t get_control_int::operator()(encoder& e) const
{
	return generic_ctl<int32_t>(e, ctl);
}

int32_t get_control_int::operator()(decoder& d) const
{
	return generic_ctl<int32_t>(d, ctl);
}

void force_instantiate()
{
	encoder e(samplerate::r48k, true, application::audio);
	decoder d(samplerate::r48k, true);
	compexity::get()(e);
	bitrate::get()(e);
	vbr::get()(e);
	vbr_constraint::get()(e);
	force_channels::get()(e);
	max_bandwidth::get()(e);
	bandwidth::get()(e);
	bandwidth::get()(d);
	signal::get()(e);
	application::get()(e);
	fec::get()(e);
	lossperc::get()(e);
	dtx::get()(e);
	lsbdepth::get()(e);
	gain::get()(d);
}

encoder::~encoder()
{
	if(!user)
		delete[] reinterpret_cast<char*>(memory);
}

encoder& encoder::operator=(const encoder& e)
{
	if(stereo != e.stereo)
		throw std::runtime_error("Channel mismatch in assignment");
	size_t s = size(stereo);
	if(memory != e.memory)
		memcpy(memory, e.memory, s);
	return *this;
}

encoder::encoder(samplerate rate, bool _stereo, application app, char* _memory)
{
	size_t s = size(_stereo);
	memory = _memory ? _memory : new char[s];
	stereo = _stereo;
	user = (_memory != NULL);
	try {
		throwex(opus_encoder_init(E(*this), rate, _stereo ? 2 : 1, app));
	} catch(...) {
		delete[] reinterpret_cast<char*>(memory);
		throw;
	}
}

encoder::encoder(const encoder& e)
{
	size_t s = size(e.stereo);
	memory = new char[s];
	memcpy(memory, e.memory, s);
	stereo = e.stereo;
	user = false;
}

encoder::encoder(const encoder& e, char* _memory)
{
	size_t s = size(e.stereo);
	memory = _memory;
	memcpy(memory, e.memory, s);
	stereo = e.stereo;
	user = true;
}

encoder::encoder(void* state, bool _stereo)
{
	memory = state;
	stereo = _stereo;
	user = true;
}

size_t encoder::size(bool stereo)
{
	return opus_encoder_get_size(stereo ? 2 : 1);
}

size_t encoder::encode(const int16_t* in, uint32_t inframes, unsigned char* out, uint32_t maxout)
{
	return throwex(opus_encode(E(*this), in, inframes, out, maxout));
}

size_t encoder::encode(const float* in, uint32_t inframes, unsigned char* out, uint32_t maxout)
{
	return throwex(opus_encode_float(E(*this), in, inframes, out, maxout));
}

decoder::~decoder()
{
	if(!user)
		delete[] reinterpret_cast<char*>(memory);
}

decoder::decoder(samplerate rate, bool _stereo, char* _memory)
{
	size_t s = size(_stereo);
	memory = _memory ? _memory : new char[s];
	stereo = _stereo;
	user = (_memory != NULL);
	try {
		throwex(opus_decoder_init(D(*this), rate, _stereo ? 2 : 1));
	} catch(...) {
		delete[] reinterpret_cast<char*>(memory);
		throw;
	}
}

decoder& decoder::operator=(const decoder& d)
{
	if(stereo != d.stereo)
		throw std::runtime_error("Channel mismatch in assignment");
	size_t s = size(stereo);
	if(memory != d.memory)
		memcpy(memory, d.memory, s);
	return *this;
}

size_t decoder::size(bool stereo)
{
	return opus_decoder_get_size(stereo ? 2 : 1);
}

decoder::decoder(const decoder& d)
{
	size_t s = size(d.stereo);
	memory = new char[s];
	memcpy(memory, d.memory, s);
	stereo = d.stereo;
	user = false;
}

decoder::decoder(const decoder& d, char* _memory)
{
	size_t s = size(d.stereo);
	memory = _memory;
	memcpy(memory, d.memory, s);
	stereo = d.stereo;
	user = true;
}

decoder::decoder(void* state, bool _stereo)
{
	memory = state;
	stereo = _stereo;
	user = true;
}

size_t decoder::decode(const unsigned char* in, uint32_t insize, int16_t* out, uint32_t maxframes, bool decode_fec)
{
	return throwex(opus_decode(D(*this), in, insize, out, maxframes, decode_fec ? 1 : 0));
}

size_t decoder::decode(const unsigned char* in, uint32_t insize, float* out, uint32_t maxframes, bool decode_fec)
{
	return throwex(opus_decode_float(D(*this), in, insize, out, maxframes, decode_fec ? 1 : 0));
}

uint32_t decoder::get_nb_samples(const unsigned char* buf, size_t bufsize)
{
	return throwex(opus_decoder_get_nb_samples(D(*this), buf, bufsize));
}

uint32_t packet_get_nb_frames(const unsigned char* packet, size_t len)
{
	return throwex(opus_packet_get_nb_frames(packet, len));
}

uint32_t packet_get_samples_per_frame(const unsigned char* data, samplerate fs)
{
	return throwex(opus_packet_get_samples_per_frame(data, fs));
}

}
#endif