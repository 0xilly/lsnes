#include "core/movie.hpp"
#include "core/moviedata.hpp"
#include "core/dispatch.hpp"
#include "core/emucore.hpp"
#include "core/window.hpp"

#include "core/mainloop.hpp"
#include "platform/wxwidgets/platform.hpp"
#include "platform/wxwidgets/scrollbar.hpp"
#include "platform/wxwidgets/textrender.hpp"
#include "library/minmax.hpp"
#include "library/string.hpp"
#include "library/utf8.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <wx/wx.h>
#include <wx/event.h>
#include <wx/control.h>
#include <wx/combobox.h>
#include <wx/clipbrd.h>

enum
{
	wxID_TOGGLE = wxID_HIGHEST + 1,
	wxID_CHANGE,
	wxID_SWEEP,
	wxID_APPEND_FRAME,
	wxID_CHANGE_LINECOUNT,
	wxID_INSERT_AFTER,
	wxID_DELETE_FRAME,
	wxID_DELETE_SUBFRAME,
	wxID_POSITION_LOCK,
	wxID_RUN_TO_FRAME,
	wxID_APPEND_FRAMES,
	wxID_TRUNCATE,
	wxID_SCROLL_FRAME,
	wxID_SCROLL_CURRENT_FRAME,
	wxID_COPY_FRAMES,
	wxID_CUT_FRAMES,
	wxID_PASTE_FRAMES,
	wxID_PASTE_APPEND,
	wxID_INSERT_CONTROLLER_AFTER,
	wxID_DELETE_CONTROLLER_SUBFRAMES,
};

void update_movie_state();

namespace
{
	unsigned lines_to_display = 28;
	uint64_t divs[] = {1000000, 100000, 10000, 1000, 100, 10, 1};
	uint64_t divsl[] = {1000000, 100000, 10000, 1000, 100, 10, 0};
	const unsigned divcnt = sizeof(divs)/sizeof(divs[0]);
}

struct control_info
{
	unsigned position_left;
	unsigned reserved;	//Must be at least 6 for axes.
	unsigned index;		//Index in poll vector.
	int type;		//-2 => Port, -1 => Fixed, 0 => Button, 1 => axis.
	char32_t ch;
	std::u32string title;
	unsigned port;
	unsigned controller;
	static control_info portinfo(unsigned& p, unsigned port, unsigned controller);
	static control_info fixedinfo(unsigned& p, const std::u32string& str);
	static control_info buttoninfo(unsigned& p, char32_t character, const std::u32string& title, unsigned idx,
		unsigned port, unsigned controller);
	static control_info axisinfo(unsigned& p, const std::u32string& title, unsigned idx,
		unsigned port, unsigned controller);
};

control_info control_info::portinfo(unsigned& p, unsigned port, unsigned controller)
{
	control_info i;
	i.position_left = p;
	i.reserved = (stringfmt() << port << "-" << controller).str32().length();
	p += i.reserved;
	i.index = 0;
	i.type = -2;
	i.ch = 0;
	i.title = U"";
	i.port = port;
	i.controller = controller;
	return i;
}

control_info control_info::fixedinfo(unsigned& p, const std::u32string& str)
{
	control_info i;
	i.position_left = p;
	i.reserved = str.length();
	p += i.reserved;
	i.index = 0;
	i.type = -1;
	i.ch = 0;
	i.title = str;
	i.port = 0;
	i.controller = 0;
	return i;
}

control_info control_info::buttoninfo(unsigned& p, char32_t character, const std::u32string& title, unsigned idx,
	unsigned port, unsigned controller)
{
	control_info i;
	i.position_left = p;
	i.reserved = 1;
	p += i.reserved;
	i.index = idx;
	i.type = 0;
	i.ch = character;
	i.title = title;
	i.port = port;
	i.controller = controller;
	return i;
}

control_info control_info::axisinfo(unsigned& p, const std::u32string& title, unsigned idx,
	unsigned port, unsigned controller)
{
	control_info i;
	i.position_left = p;
	i.reserved = title.length();
	if(i.reserved < 6)
		i.reserved = 6;
	p += i.reserved;
	i.index = idx;
	i.type = 1;
	i.ch = 0;
	i.title = title;
	i.port = port;
	i.controller = controller;
	return i;
}

class frame_controls
{
public:
	frame_controls();
	void set_types(controller_frame& f);
	short read_index(controller_frame& f, unsigned idx);
	void write_index(controller_frame& f, unsigned idx, short value);
	uint32_t read_pollcount(pollcounter_vector& v, unsigned idx);
	const std::list<control_info>& get_controlinfo() { return controlinfo; }
	std::u32string line1() { return _line1; }
	std::u32string line2() { return _line2; }
	size_t width() { return _width; }
private:
	size_t _width;
	std::u32string _line1;
	std::u32string _line2;
	void format_lines();
	void add_port(unsigned& c, unsigned pid, porttype_info& p);
	std::list<control_info> controlinfo;
};


frame_controls::frame_controls()
{
	_width = 0;
}

void frame_controls::set_types(controller_frame& f)
{
	unsigned nextp = 0;
	unsigned nextc = 0;
	controlinfo.clear();
	controlinfo.push_back(control_info::portinfo(nextp, 0, 0));
	controlinfo.push_back(control_info::buttoninfo(nextc, U'F', U"Framesync", 0, 0, 0));
	controlinfo.push_back(control_info::buttoninfo(nextc, U'R', U"Reset", 1, 0, 0));
	nextc++;
	controlinfo.push_back(control_info::axisinfo(nextc, U" rhigh", 2, 0, 0));
	nextc++;
	controlinfo.push_back(control_info::axisinfo(nextc, U"  rlow", 3, 0, 0));
	if(nextp > nextc)
		nextc = nextp;
	nextp = nextc;
	add_port(nextp, 1, f.get_port_type(0));
	add_port(nextp, 2, f.get_port_type(1));
	format_lines();
}

void frame_controls::add_port(unsigned& c, unsigned pid, porttype_info& p)
{
	unsigned i = 0;
	unsigned ccount = MAX_CONTROLLERS_PER_PORT * MAX_CONTROLS_PER_CONTROLLER;
	auto limits = get_core_logical_controller_limits();
	while(p.is_present(i)) {
		controlinfo.push_back(control_info::fixedinfo(c, U"\u2502"));
		unsigned nextp = c;
		controlinfo.push_back(control_info::portinfo(nextp, pid, i + 1));
		unsigned b = 0;
		if(p.is_analog(i)) {
			controlinfo.push_back(control_info::axisinfo(c, U" xaxis", 4 + ccount * pid + i *
				MAX_CONTROLS_PER_CONTROLLER - ccount, pid, i));
			c++;
			controlinfo.push_back(control_info::axisinfo(c, U" yaxis", 5 + ccount * pid + i *
				MAX_CONTROLS_PER_CONTROLLER - ccount, pid, i));
			if(p.button_symbols[0])
				c++;
			b = 2;
		}
		for(unsigned j = 0; p.button_symbols[j]; j++, b++) {
			unsigned lbid;
			for(lbid = 0; lbid < limits.second; lbid++)
				if(p.button_id(i, lbid) == b)
					break;
			if(lbid == limits.second)
				lbid = 0;
			std::u32string name = to_u32string(get_logical_button_name(lbid));
			controlinfo.push_back(control_info::buttoninfo(c, p.button_symbols[j], name, 4 + b + ccount *
				pid + i * MAX_CONTROLS_PER_CONTROLLER - ccount, pid, i));
		}
		if(nextp > c)
			c = nextp;
		i++;
	}
}

short frame_controls::read_index(controller_frame& f, unsigned idx)
{
	if(idx == 0)
		return f.sync() ? 1 : 0;
	if(idx == 1)
		return f.reset() ? 1 : 0;
	if(idx == 2)
		return f.delay().first;
	if(idx == 3)
		return f.delay().second;
	return f.axis2(idx - 4);
}

void frame_controls::write_index(controller_frame& f, unsigned idx, short value)
{
	if(idx == 0)
		return f.sync(value);
	if(idx == 1)
		return f.reset(value);
	if(idx == 2)
		return f.delay(std::make_pair(value, f.delay().second));
	if(idx == 3)
		return f.delay(std::make_pair(f.delay().first, value));
	return f.axis2(idx - 4, value);
}

uint32_t frame_controls::read_pollcount(pollcounter_vector& v, unsigned idx)
{
	if(idx == 0)
		return max(v.max_polls(), (uint32_t)1);
	if(idx < 4)
		return v.get_system() ? 1 : 0;
	return v.get_polls(idx - 4);
}

void frame_controls::format_lines()
{
	_width = 0;
	for(auto i : controlinfo) {
		if(i.position_left + i.reserved > _width)
			_width = i.position_left + i.reserved;
	}
	std::u32string cp1;
	std::u32string cp2;
	uint32_t off = divcnt + 1;
	cp1.resize(_width + divcnt + 1);
	cp2.resize(_width + divcnt + 1);
	for(unsigned i = 0; i < cp1.size(); i++)
		cp1[i] = cp2[i] = 32;
	cp1[divcnt] = 0x2502;
	cp2[divcnt] = 0x2502;
	//Line1
	//For every port-controller, find the least coordinate.
	for(auto i : controlinfo) {
		if(i.type == -1) {
			auto _title = i.title;
			std::copy(_title.begin(), _title.end(), &cp1[i.position_left + off]);
		} else if(i.type == -2) {
			auto _title = (stringfmt() << i.port << "-" << i.controller).str32();
			std::copy(_title.begin(), _title.end(), &cp1[i.position_left + off]);
		}
	}
	//Line2
	for(auto i : controlinfo) {
		auto _title = i.title;
		if(i.type == -1 || i.type == 1)
			std::copy(_title.begin(), _title.end(), &cp2[i.position_left + off]);
		if(i.type == 0)
			cp2[i.position_left + off] = i.ch;
	}
	_line1 = cp1;
	_line2 = cp2;
}

namespace
{
	//TODO: Use real clipboard.
	std::string clipboard;

	void copy_to_clipboard(const std::string& text)
	{
		clipboard = text;
	}

	bool clipboard_has_text()
	{
		return (clipboard.length() > 0);
	}

	void clear_clipboard()
	{
		clipboard = "";
	}

	std::string copy_from_clipboard()
	{
		return clipboard;
	}

	std::string encode_line(controller_frame& f)
	{
		char buffer[512];
		f.serialize(buffer);
		return buffer;
	}

	std::string encode_line(frame_controls& info, controller_frame& f, unsigned port, unsigned controller)
	{
		std::ostringstream x;
		bool last_axis = false;
		bool first = true;
		for(auto i : info.get_controlinfo()) {
			if(i.port != port)
				continue;
			if(i.controller != controller)
				continue;
			switch(i.type) {
			case 0:		//Button.
				if(last_axis)
					x << " ";
				if(info.read_index(f, i.index)) {
					char32_t tmp1[2];
					tmp1[0] = i.ch;
					tmp1[1] = 0;
					x << to_u8string(std::u32string(tmp1));
				} else
					x << "-";
				last_axis = false;
				first = false;
				break;
			case 1:		//Axis.
				if(!first)
					x << " ";
				x << info.read_index(f, i.index);
				first = false;
				last_axis = true;
				break;
			}
		}
		return x.str();
	}

	short read_short(const std::u32string& s, size_t& r)
	{
		unsigned short _res = 0;
		bool negative = false;
		if(r < s.length() && s[r] == '-') {
			negative = true;
			r++;
		}
		while(r < s.length() && s[r] >= 48 && s[r] <= 57) {
			_res = _res * 10 + (s[r] - 48);
			r++;
		}
		return negative ? -_res : _res;
	}

	void decode_line(frame_controls& info, controller_frame& f, std::string line, unsigned port,
		unsigned controller)
	{
		std::u32string _line = to_u32string(line);
		bool last_axis = false;
		bool first = true;
		short y;
		char32_t y2;
		size_t ridx = 0;
		for(auto i : info.get_controlinfo()) {
			if(i.port != port)
				continue;
			if(i.controller != controller)
				continue;
			switch(i.type) {
			case 0:		//Button.
				if(last_axis) {
					ridx++;
					while(ridx < _line.length() && (_line[ridx] == 9 || _line[ridx] == 10 ||
						_line[ridx] == 13 || _line[ridx] == 32))
						ridx++;
				}
				y2 = (ridx < _line.length()) ? _line[ridx++] : 0;
				if(y2 == U'-' || y2 == 0)
					info.write_index(f, i.index, 0);
				else
					info.write_index(f, i.index, 1);
				last_axis = false;
				first = false;
				break;
			case 1:		//Axis.
				if(!first)
					ridx++;
				while(ridx < _line.length() && (_line[ridx] == 9 || _line[ridx] == 10 ||
					_line[ridx] == 13 || _line[ridx] == 32))
					ridx++;
				y = read_short(_line, ridx);
				info.write_index(f, i.index, y);
				first = false;
				last_axis = true;
				break;
			}
		}
	}

	std::string encode_lines(controller_frame_vector& fv, uint64_t start, uint64_t end)
	{
		std::ostringstream x;
		x << "lsnes-moviedata-whole" << std::endl;
		for(uint64_t i = start; i < end; i++) {
			controller_frame tmp = fv[i];
			x << encode_line(tmp) << std::endl;
		}
		return x.str();
	}

	std::string encode_lines(frame_controls& info, controller_frame_vector& fv, uint64_t start, uint64_t end,
		unsigned port, unsigned controller)
	{
		std::ostringstream x;
		x << "lsnes-moviedata-controller" << std::endl;
		for(uint64_t i = start; i < end; i++) {
			controller_frame tmp = fv[i];
			x << encode_line(info, tmp, port, controller) << std::endl;
		}
		return x.str();
	}

	int clipboard_get_data_type()
	{
		if(!clipboard_has_text())
			return -1;
		std::string y = copy_from_clipboard();
		std::istringstream x(y);
		std::string hdr;
		std::getline(x, hdr);
		if(hdr == "lsnes-moviedata-whole")
			return 1;
		if(hdr == "lsnes-moviedata-controller")
			return 0;
		return -1;
	}

	std::set<unsigned> controller_index_set(frame_controls& info, unsigned port, unsigned controller)
	{
		std::set<unsigned> r;
		for(auto i : info.get_controlinfo()) {
			if(i.port == port && i.controller == controller && (i.type == 0 || i.type == 1))
				r.insert(i.index);
		}
		return r;
	}

	void move_index_set(frame_controls& info, controller_frame_vector& fv, uint64_t src, uint64_t dst,
		uint64_t len, const std::set<unsigned>& indices)
	{
		if(src == dst)
			return;
		if(src > dst) {
			//Copy forwards.
			uint64_t shift = src - dst;
			for(uint64_t i = dst; i < dst + len; i++) {
				controller_frame _src = fv[i + shift];
				controller_frame _dst = fv[i];
				for(auto j : indices)
					info.write_index(_dst, j, info.read_index(_src, j));
			}
		} else {
			//Copy backwards.
			uint64_t shift = dst - src;
			for(uint64_t i = src + len - 1; i >= src && i < src + len; i--) {
				controller_frame _src = fv[i];
				controller_frame _dst = fv[i + shift];
				for(auto j : indices)
					info.write_index(_dst, j, info.read_index(_src, j));
			}
		}
	}

	void zero_index_set(frame_controls& info, controller_frame_vector& fv, uint64_t dst, uint64_t len,
		const std::set<unsigned>& indices)
	{
		for(uint64_t i = dst; i < dst + len; i++) {
			controller_frame _dst = fv[i];
			for(auto j : indices)
				info.write_index(_dst, j, 0);
		}
	}
}

class wxeditor_movie : public wxDialog
{
public:
	wxeditor_movie(wxWindow* parent);
	~wxeditor_movie() throw();
	bool ShouldPreventAppExit() const;
	void on_close(wxCommandEvent& e);
	void on_wclose(wxCloseEvent& e);
	void on_focus_wrong(wxFocusEvent& e);
	void on_keyboard_down(wxKeyEvent& e);
	void on_keyboard_up(wxKeyEvent& e);
	scroll_bar* get_scroll();
	void update();
private:
	struct _moviepanel : public wxPanel, public information_dispatch
	{
		_moviepanel(wxeditor_movie* v);
		~_moviepanel() throw();
		void signal_repaint();
		void on_paint(wxPaintEvent& e);
		void on_erase(wxEraseEvent& e);
		void on_mouse(wxMouseEvent& e);
		void on_popup_menu(wxCommandEvent& e);
		uint64_t moviepos;
	private:
		int get_lines();
		void render(text_framebuffer& fb, unsigned long long pos);
		void on_mouse0(unsigned x, unsigned y, bool polarity);
		void on_mouse1(unsigned x, unsigned y, bool polarity);
		void on_mouse2(unsigned x, unsigned y, bool polarity);
		void do_toggle_buttons(unsigned idx, uint64_t row1, uint64_t row2);
		void do_alter_axis(unsigned idx, uint64_t row1, uint64_t row2);
		void do_sweep_axis(unsigned idx, uint64_t row1, uint64_t row2);
		void do_append_frames(uint64_t count);
		void do_append_frames();
		void do_insert_frame_after(uint64_t row);
		void do_delete_frame(uint64_t row1, uint64_t row2, bool wholeframe);
		void do_truncate(uint64_t row);
		void do_set_stop_at_frame();
		void do_scroll_to_frame();
		void do_scroll_to_current_frame();
		void do_copy(uint64_t row1, uint64_t row2, unsigned port, unsigned controller);
		void do_copy(uint64_t row1, uint64_t row2);
		void do_cut(uint64_t row1, uint64_t row2, unsigned port, unsigned controller);
		void do_cut(uint64_t row1, uint64_t row2);
		void do_paste(uint64_t row, unsigned port, unsigned controller, bool append);
		void do_paste(uint64_t row, bool append);
		void do_insert_controller(uint64_t row, unsigned port, unsigned controller);
		void do_delete_controller(uint64_t row1, uint64_t row2, unsigned port, unsigned controller);
		uint64_t first_editable(unsigned index);
		uint64_t first_nextframe();
		int width(controller_frame& f);
		std::u32string render_line1(controller_frame& f);
		std::u32string render_line2(controller_frame& f);
		void render_linen(text_framebuffer& fb, controller_frame& f, uint64_t sfn, int y);
		unsigned long long spos;
		void* prev_obj;
		uint64_t prev_seqno;
		void update_cache();
		std::map<uint64_t, uint64_t> subframe_to_frame;
		uint64_t max_subframe;
		frame_controls fcontrols;
		wxeditor_movie* m;
		bool requested;
		text_framebuffer fb;
		uint64_t movielines;
		unsigned new_width;
		unsigned new_height;
		std::vector<uint8_t> pixels;
		unsigned press_x;
		uint64_t press_line;
		uint64_t rpress_line;
		unsigned press_index;
		bool pressed;
		bool recursing;
		uint64_t linecount;
		uint64_t cached_cffs;
		bool position_locked;
		wxMenu* current_popup;
	};
	_moviepanel* moviepanel;
	wxButton* closebutton;
	scroll_bar* moviescroll;
	bool closing;
};

namespace
{
	wxeditor_movie* movieeditor_open;

	//Find the first real editable subframe.
	//Call only in emulator thread.
	uint64_t real_first_editable(frame_controls& fc, unsigned idx)
	{
		uint64_t cffs = movb.get_movie().get_current_frame_first_subframe();
		controller_frame_vector& fv = movb.get_movie().get_frame_vector();
		pollcounter_vector& pv = movb.get_movie().get_pollcounters();
		uint64_t vsize = fv.size();
		uint32_t pc = fc.read_pollcount(pv, idx);
		for(uint32_t i = 1; i < pc; i++)
			if(cffs + i >= vsize || fv[cffs + i].sync())
				return cffs + i;
		return cffs + pc;
	}

	uint64_t real_first_editable(frame_controls& fc, std::set<unsigned> idx)
	{
		uint64_t m = 0;
		for(auto i : idx)
			m = max(m, real_first_editable(fc, i));
		return m;
	}

	//Find the first real editable whole frame.
	//Call only in emulator thread.
	uint64_t real_first_nextframe(frame_controls& fc)
	{
		uint64_t base = real_first_editable(fc, 0);
		controller_frame_vector& fv = movb.get_movie().get_frame_vector();
		uint64_t vsize = fv.size();
		for(uint32_t i = 0;; i++)
			if(base + i >= vsize || fv[base + i].sync())
				return base + i;
	}

	//Adjust movie length by specified number of frames.
	//Call only in emulator thread.
	void movie_framecount_change(int64_t adjust, bool known = true);
	void movie_framecount_change(int64_t adjust, bool known)
	{
		if(known)
			movb.get_movie().adjust_frame_count(adjust);
		else
			movb.get_movie().recount_frames();
		update_movie_state();
		graphics_plugin::notify_status();
	}
}

wxeditor_movie::_moviepanel::~_moviepanel() throw() {}
wxeditor_movie::~wxeditor_movie() throw() {}

wxeditor_movie::_moviepanel::_moviepanel(wxeditor_movie* v)
	: wxPanel(v, wxID_ANY, wxDefaultPosition, wxSize(100, 100), wxWANTS_CHARS),
	information_dispatch("movieeditor-listener")
{
	m = v;
	Connect(wxEVT_PAINT, wxPaintEventHandler(_moviepanel::on_paint), NULL, this);
	Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(_moviepanel::on_erase), NULL, this);
	new_width = 0;
	new_height = 0;
	moviepos = 0;
	spos = 0;
	prev_obj = NULL;
	prev_seqno = 0;
	max_subframe = 0;
	recursing = false;
	position_locked = true;
	current_popup = NULL;

	Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(_moviepanel::on_mouse), NULL, this);
	Connect(wxEVT_LEFT_UP, wxMouseEventHandler(_moviepanel::on_mouse), NULL, this);
	Connect(wxEVT_MIDDLE_DOWN, wxMouseEventHandler(_moviepanel::on_mouse), NULL, this);
	Connect(wxEVT_MIDDLE_UP, wxMouseEventHandler(_moviepanel::on_mouse), NULL, this);
	Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(_moviepanel::on_mouse), NULL, this);
	Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(_moviepanel::on_mouse), NULL, this);
	Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(_moviepanel::on_mouse), NULL, this);

	signal_repaint();
	requested = false;
}

void wxeditor_movie::_moviepanel::update_cache()
{
	movie& m = movb.get_movie();
	controller_frame_vector& fv = m.get_frame_vector();
	if(&m == prev_obj && prev_seqno == m.get_seqno()) {
		//Just process new subframes if any.
		for(uint64_t i = max_subframe; i < fv.size(); i++) {
			uint64_t prev = (i > 0) ? subframe_to_frame[i - 1] : 0;
			controller_frame f = fv[i];
			if(f.sync())
				subframe_to_frame[i] = prev + 1;
			else
				subframe_to_frame[i] = prev;
		}
		max_subframe = fv.size();
		return;
	}
	//Reprocess all subframes.
	for(uint64_t i = 0; i < fv.size(); i++) {
		uint64_t prev = (i > 0) ? subframe_to_frame[i - 1] : 0;
		controller_frame f = fv[i];
		if(f.sync())
			subframe_to_frame[i] = prev + 1;
		else
			subframe_to_frame[i] = prev;
	}
	max_subframe = fv.size();
	controller_frame model = fv.blank_frame(false);
	fcontrols.set_types(model);
	prev_obj = &m;
	prev_seqno = m.get_seqno();
}

int wxeditor_movie::_moviepanel::width(controller_frame& f)
{
	update_cache();
	return divcnt + 1 + fcontrols.width();
}

std::u32string wxeditor_movie::_moviepanel::render_line1(controller_frame& f)
{
	update_cache();
	return fcontrols.line1();
}

std::u32string wxeditor_movie::_moviepanel::render_line2(controller_frame& f)
{
	update_cache();
	return fcontrols.line2();
}

void wxeditor_movie::_moviepanel::render_linen(text_framebuffer& fb, controller_frame& f, uint64_t sfn, int y)
{
	update_cache();
	size_t fbstride = fb.get_stride();
	text_framebuffer::element* _fb = fb.get_buffer();
	text_framebuffer::element e;
	e.bg = 0xFFFFFF;
	e.fg = 0x000000;
	for(unsigned i = 0; i < divcnt; i++) {
		uint64_t fn = subframe_to_frame[sfn];
		e.ch = (fn >= divsl[i]) ? (((fn / divs[i]) % 10) + 48) : 32;
		_fb[y * fbstride + i] = e;
	}
	e.ch = 0x2502;
	_fb[y * fbstride + divcnt] = e;
	const std::list<control_info>& ctrlinfo = fcontrols.get_controlinfo();
	uint64_t curframe = movb.get_movie().get_current_frame();
	pollcounter_vector& pv = movb.get_movie().get_pollcounters();
	uint64_t cffs = movb.get_movie().get_current_frame_first_subframe();
	cached_cffs = cffs;
	int past = -1;
	if(!movb.get_movie().readonly_mode())
		past = 1;
	else if(subframe_to_frame[sfn] < curframe)
		past = 1;
	else if(subframe_to_frame[sfn] > curframe)
		past = 0;
	bool now = (subframe_to_frame[sfn] == curframe);
	unsigned xcord = 32768;
	if(pressed)
		xcord = press_x;

	for(auto i : ctrlinfo) {
		int rpast = past;
		unsigned off = divcnt + 1;
		bool cselected = (xcord >= i.position_left + off && xcord < i.position_left + i.reserved + off);
		if(rpast == -1) {
			unsigned polls = fcontrols.read_pollcount(pv, i.index);
			rpast = ((cffs + polls) > sfn) ? 1 : 0;
		}
		uint32_t bgc = 0xC0C0C0;
		if(rpast)
			bgc |= 0x0000FF;
		if(now)
			bgc |= 0xFF0000;
		if(cselected)
			bgc |= 0x00FF00;
		if(bgc == 0xC0C0C0)
			bgc = 0xFFFFFF;
		if(i.type == -1) {
			//Separator.
			fb.write(i.title, 0, divcnt + 1 + i.position_left, y, 0x000000, 0xFFFFFF);
		} else if(i.type == 0) {
			//Button.
			char32_t c[2];
			bool v = (fcontrols.read_index(f, i.index) != 0);
			c[0] = i.ch;
			c[1] = 0;
			fb.write(c, 0, divcnt + 1 + i.position_left, y, v ? 0x000000 : 0xC8C8C8, bgc);
		} else if(i.type == 1) {
			//Axis.
			char c[7];
			sprintf(c, "%6d", fcontrols.read_index(f, i.index));
			fb.write(c, 0, divcnt + 1 + i.position_left, y, 0x000000, bgc);
		}
	}
}

void wxeditor_movie::_moviepanel::render(text_framebuffer& fb, unsigned long long pos)
{
	spos = pos;
	controller_frame_vector& fv = movb.get_movie().get_frame_vector();
	controller_frame cf = fv.blank_frame(false);
	int _width = width(cf);
	fb.set_size(_width, lines_to_display + 3);
	size_t fbstride = fb.get_stride();
	auto fbsize = fb.get_characters();
	text_framebuffer::element* _fb = fb.get_buffer();
	fb.write((stringfmt() << "Current frame: " << movb.get_movie().get_current_frame() << " of "
		<< movb.get_movie().get_frame_count()).str(), _width, 0, 0,
		 0x000000, 0xFFFFFF);
	fb.write(render_line1(cf), _width, 0, 1, 0x000000, 0xFFFFFF);
	fb.write(render_line2(cf), _width, 0, 2, 0x000000, 0xFFFFFF);
	unsigned long long lines = fv.size();
	unsigned long long i;
	unsigned j;
	for(i = pos, j = 3; i < pos + lines_to_display; i++, j++) {
		text_framebuffer::element e;
		if(i >= lines) {
			//Out of range.
			e.bg = 0xFFFFFF;
			e.fg = 0x000000;
			e.ch = 32;
			for(unsigned k = 0; k < fbsize.first; k++)
				_fb[j * fbstride + k] = e;
		} else {
			controller_frame frame = fv[i];
			render_linen(fb, frame, i, j);
		}
	}
}

void wxeditor_movie::_moviepanel::do_toggle_buttons(unsigned idx, uint64_t row1, uint64_t row2)
{
	frame_controls* _fcontrols = &fcontrols;
	uint64_t _press_line = row1;
	uint64_t line = row2;
	if(_press_line > line)
		std::swap(_press_line, line);
	recursing = true;
	runemufn([idx, _press_line, line, _fcontrols]() {
		int64_t adjust = 0;
		if(!movb.get_movie().readonly_mode())
			return;
		uint64_t fedit = real_first_editable(*_fcontrols, idx);
		controller_frame_vector& fv = movb.get_movie().get_frame_vector();
		for(uint64_t i = _press_line; i <= line; i++) {
			if(i < fedit || i >= fv.size())
				continue;
			controller_frame cf = fv[i];
			bool v = _fcontrols->read_index(cf, idx);
			_fcontrols->write_index(cf, idx, !v);
			adjust += (v ? -1 : 1);
		}
		if(idx == 0)
			movie_framecount_change(adjust);
	});
	recursing = false;
	if(idx == 0)
		max_subframe = _press_line;	//Reparse.
	signal_repaint();
}

void wxeditor_movie::_moviepanel::do_alter_axis(unsigned idx, uint64_t row1, uint64_t row2)
{
	frame_controls* _fcontrols = &fcontrols;
	uint64_t line = row1;
	uint64_t line2 = row2;
	short value;
	bool valid = true;
	runemufn([idx, line, &value, _fcontrols, &valid]() {
		if(!movb.get_movie().readonly_mode()) {
			valid = false;
			return;
		}
		uint64_t fedit = real_first_editable(*_fcontrols, idx);
		controller_frame_vector& fv = movb.get_movie().get_frame_vector();
		if(line < fedit || line >= fv.size()) {
			valid = false;
			return;
		}
		controller_frame cf = fv[line];
		value = _fcontrols->read_index(cf, idx);
	});
	if(!valid)
		return;
	try {
		std::string text = pick_text(m, "Set value", "Enter new value:", (stringfmt() << value).str());
		value = parse_value<short>(text);
	} catch(canceled_exception& e) {
		return;
	} catch(std::exception& e) {
		wxMessageBox(wxT("Invalid value"), _T("Error"), wxICON_EXCLAMATION | wxOK, m);
		return;
	}
	if(line > line2)
		std::swap(line, line2);
	runemufn([idx, line, line2, value, _fcontrols]() {
		uint64_t fedit = real_first_editable(*_fcontrols, idx);
		controller_frame_vector& fv = movb.get_movie().get_frame_vector();
		for(uint64_t i = line; i <= line2; i++) {
			if(i < fedit || i >= fv.size())
				continue;
			controller_frame cf = fv[i];
			_fcontrols->write_index(cf, idx, value);
		}
	});
	signal_repaint();
}

void wxeditor_movie::_moviepanel::do_sweep_axis(unsigned idx, uint64_t row1, uint64_t row2)
{
	frame_controls* _fcontrols = &fcontrols;
	uint64_t line = row1;
	uint64_t line2 = row2;
	short value;
	short value2;
	bool valid = true;
	if(line > line2)
		std::swap(line, line2);
	runemufn([idx, line, line2, &value, &value2, _fcontrols, &valid]() {
		if(!movb.get_movie().readonly_mode()) {
			valid = false;
			return;
		}
		uint64_t fedit = real_first_editable(*_fcontrols, idx);
		controller_frame_vector& fv = movb.get_movie().get_frame_vector();
		if(line2 < fedit || line2 >= fv.size()) {
			valid = false;
			return;
		}
		controller_frame cf = fv[line];
		value = _fcontrols->read_index(cf, idx);
		controller_frame cf2 = fv[line2];
		value2 = _fcontrols->read_index(cf2, idx);
	});
	if(!valid)
		return;
	runemufn([idx, line, line2, value, value2, _fcontrols]() {
		uint64_t fedit = real_first_editable(*_fcontrols, idx);
		controller_frame_vector& fv = movb.get_movie().get_frame_vector();
		for(uint64_t i = line + 1; i <= line2 - 1; i++) {
			if(i < fedit || i >= fv.size())
				continue;
			controller_frame cf = fv[i];
			auto tmp2 = static_cast<int64_t>(i - line) * (value2 - value) /
				static_cast<int64_t>(line2 - line);
			short tmp = value + tmp2;
			_fcontrols->write_index(cf, idx, tmp);
		}
	});
	signal_repaint();
}

void wxeditor_movie::_moviepanel::do_append_frames(uint64_t count)
{
	recursing = true;
	uint64_t _count = count;
	runemufn([_count]() {
		if(!movb.get_movie().readonly_mode())
			return;
		controller_frame_vector& fv = movb.get_movie().get_frame_vector();
		for(uint64_t i = 0; i < _count; i++)
			fv.append(fv.blank_frame(true));
		movie_framecount_change(_count);
	});
	recursing = false;
	signal_repaint();
}

void wxeditor_movie::_moviepanel::do_append_frames()
{
	uint64_t value;
	try {
		std::string text = pick_text(m, "Append frames", "Enter number of frames to append:", "");
		value = parse_value<uint64_t>(text);
	} catch(canceled_exception& e) {
		return;
	} catch(std::exception& e) {
		wxMessageBox(wxT("Invalid value"), _T("Error"), wxICON_EXCLAMATION | wxOK, m);
		return;
	}
	do_append_frames(value);
	signal_repaint();
}

void wxeditor_movie::_moviepanel::do_insert_frame_after(uint64_t row)
{
	recursing = true;
	frame_controls* _fcontrols = &fcontrols;
	uint64_t _row = row;
	runemufn([_row, _fcontrols]() {
		if(!movb.get_movie().readonly_mode())
			return;
		controller_frame_vector& fv = movb.get_movie().get_frame_vector();
		uint64_t fedit = real_first_editable(*_fcontrols, 0);
		//Find the start of the next frame.
		uint64_t nframe = _row + 1;
		uint64_t vsize = fv.size();
		while(nframe < vsize && !fv[nframe].sync())
			nframe++;
		if(nframe < fedit)
			return;
		fv.append(fv.blank_frame(true));
		if(nframe < vsize) {
			//Okay, gotta copy all data after this point. nframe has to be at least 1.
			for(uint64_t i = vsize - 1; i >= nframe; i--)
				fv[i + 1] = fv[i];
			fv[nframe] = fv.blank_frame(true);
		}
		movie_framecount_change(1);
	});
	max_subframe = row;
	recursing = false;
	signal_repaint();
}

void wxeditor_movie::_moviepanel::do_delete_frame(uint64_t row1, uint64_t row2, bool wholeframe)
{
	recursing = true;
	uint64_t _row1 = row1;
	uint64_t _row2 = row2;
	bool _wholeframe = wholeframe;
	frame_controls* _fcontrols = &fcontrols;
	if(_row1 > _row2) std::swap(_row1, _row2);
	runemufn([_row1, _row2, _wholeframe, _fcontrols]() {
		controller_frame_vector& fv = movb.get_movie().get_frame_vector();
		uint64_t vsize = fv.size();
		if(_row1 >= vsize)
			return;		//Nothing to do.
		uint64_t row2 = min(_row2, vsize - 1);
		uint64_t row1 = min(_row1, vsize - 1);
		row1 = max(row1, real_first_editable(*_fcontrols, 0));
		if(_wholeframe) {
			if(_row2 < real_first_nextframe(*_fcontrols))
				return;		//Nothing to do.
			//Scan backwards for the first subframe of this frame and forwards for the last.
			uint64_t fsf = row1;
			uint64_t lsf = row2;
			if(fv[_row2].sync())
				lsf++;		//Bump by one so it finds the end.
			while(fsf < vsize && !fv[fsf].sync())
				fsf--;
			while(lsf < vsize && !fv[lsf].sync())
				lsf++;
			fsf = max(fsf, real_first_editable(*_fcontrols, 0));
			uint64_t tonuke = lsf - fsf;
			int64_t frames_tonuke = 0;
			//Count frames nuked.
			for(uint64_t i = fsf; i < lsf; i++)
				if(fv[i].sync())
					frames_tonuke++;
			//Nuke from fsf to lsf.
			for(uint64_t i = fsf; i < vsize - tonuke; i++)
				fv[i] = fv[i + tonuke];
			fv.resize(vsize - tonuke);
			movie_framecount_change(-frames_tonuke);
		} else {
			if(row2 < real_first_editable(*_fcontrols, 0))
				return;		//Nothing to do.
			//The sync flag needs to be inherited if:
			//1) Some deleted subframe has sync flag AND
			//2) The subframe immediately after deleted region doesn't.
			bool inherit_sync = false;
			for(uint64_t i = row1; i <= row2; i++)
				inherit_sync = inherit_sync || fv[i].sync();
			inherit_sync = inherit_sync && (row2 + 1 < vsize && !fv[_row2 + 1].sync());
			int64_t frames_tonuke = 0;
			//Count frames nuked.
			for(uint64_t i = row1; i <= row2; i++)
				if(fv[i].sync())
					frames_tonuke++;
			//If sync is inherited, one less frame is nuked.
			if(inherit_sync) frames_tonuke--;
			//Nuke the subframes.
			uint64_t tonuke = row2 - row1 + 1;
			for(uint64_t i = row1; i < vsize - tonuke; i++)
				fv[i] = fv[i + tonuke];
			fv.resize(vsize - tonuke);
			//Next subframe inherits the sync flag.
			if(inherit_sync)
				fv[row1].sync(true);
			movie_framecount_change(-frames_tonuke);
		}
	});
	max_subframe = _row1;
	recursing = false;
	signal_repaint();
}

void wxeditor_movie::_moviepanel::do_truncate(uint64_t row)
{
	recursing = true;
	uint64_t _row = row;
	frame_controls* _fcontrols = &fcontrols;
	runemufn([_row, _fcontrols]() {
		controller_frame_vector& fv = movb.get_movie().get_frame_vector();
		uint64_t vsize = fv.size();
		if(_row >= vsize)
			return;
		if(_row < real_first_editable(*_fcontrols, 0))
			return;
		int64_t delete_count = 0;
		for(uint64_t i = _row; i < vsize; i++)
			if(fv[i].sync())
				delete_count--;
		fv.resize(_row);
		movie_framecount_change(delete_count);
	});
	max_subframe = row;
	recursing = false;
	signal_repaint();
}

void wxeditor_movie::_moviepanel::do_set_stop_at_frame()
{
	uint64_t curframe;
	uint64_t frame;
	runemufn([&curframe]() {
		curframe = movb.get_movie().get_current_frame();
	});
	try {
		std::string text = pick_text(m, "Frame", (stringfmt() << "Enter frame to stop at (currently at "
			<< curframe << "):").str(), "");
		frame = parse_value<uint64_t>(text);
	} catch(canceled_exception& e) {
		return;
	} catch(std::exception& e) {
		wxMessageBox(wxT("Invalid value"), _T("Error"), wxICON_EXCLAMATION | wxOK, m);
		return;
	}
	if(frame < curframe) {
		wxMessageBox(wxT("The movie is already past that point"), _T("Error"), wxICON_EXCLAMATION | wxOK, m);
		return;
	}
	runemufn([frame]() {
		set_stop_at_frame(frame);
	});
}

void wxeditor_movie::_moviepanel::on_mouse0(unsigned x, unsigned y, bool polarity)
{
	if(y < 3)
		return;
	if(polarity) {
		press_x = x;
		press_line = spos + y - 3;
	}
	pressed = polarity;
	if(polarity)
		return;
	uint64_t line = spos + y - 3;
	if(press_x < divcnt && x < divcnt) {
		//Press on frame count.
		uint64_t row1 = press_line;
		uint64_t row2 = line;
		if(row1 > row2)
			std::swap(row1, row2);
		do_append_frames(row2 - row1 + 1);
	}
	for(auto i : fcontrols.get_controlinfo()) {
		unsigned off = divcnt + 1;
		unsigned idx = i.index;
		if((press_x >= i.position_left + off && press_x < i.position_left + i.reserved + off) &&
			(x >= i.position_left + off && x < i.position_left + i.reserved + off)) {
			if(i.type == 0)
				do_toggle_buttons(idx, press_line, line);
			else if(i.type == 1)
				do_alter_axis(idx, press_line, line);
		}
	}
}

void wxeditor_movie::_moviepanel::do_scroll_to_frame()
{
	uint64_t frame;
	try {
		std::string text = pick_text(m, "Frame", (stringfmt() << "Enter frame to scroll to:").str(), "");
		frame = parse_value<uint64_t>(text);
	} catch(canceled_exception& e) {
		return;
	} catch(std::exception& e) {
		wxMessageBox(wxT("Invalid value"), _T("Error"), wxICON_EXCLAMATION | wxOK, m);
		return;
	}
	uint64_t wouldbe;
	uint64_t low = 0;
	uint64_t high = max_subframe;
	while(low < high) {
		wouldbe = (low + high) / 2;
		if(subframe_to_frame[wouldbe] < frame)
			low = wouldbe;
		else if(subframe_to_frame[wouldbe] > frame)
			high = wouldbe;
		else
			break;
	}
	while(wouldbe > 1 && subframe_to_frame[wouldbe - 1] == frame)
		wouldbe--;
	moviepos = wouldbe;
	signal_repaint();
}

void wxeditor_movie::_moviepanel::do_scroll_to_current_frame()
{
	moviepos = cached_cffs;
	signal_repaint();
}

void wxeditor_movie::_moviepanel::on_popup_menu(wxCommandEvent& e)
{
	wxMenuItem* tmpitem;
	int id = e.GetId();

	unsigned port = 0;
	unsigned controller = 0;
	for(auto i : fcontrols.get_controlinfo())
		if(i.index == press_index) {
		port = i.port;
		controller = i.controller;
	}

	switch(id) {
	case wxID_TOGGLE:
		do_toggle_buttons(press_index, rpress_line, press_line);
		return;
	case wxID_CHANGE:
		do_alter_axis(press_index, rpress_line, press_line);
		return;
	case wxID_SWEEP:
		do_sweep_axis(press_index, rpress_line, press_line);
		return;
	case wxID_APPEND_FRAME:
		do_append_frames(1);
		return;
	case wxID_APPEND_FRAMES:
		do_append_frames();
		return;
	case wxID_INSERT_AFTER:
		do_insert_frame_after(press_line);
		return;
	case wxID_DELETE_FRAME:
		do_delete_frame(press_line, rpress_line, true);
		return;
	case wxID_DELETE_SUBFRAME:
		do_delete_frame(press_line, rpress_line, false);
		return;
	case wxID_TRUNCATE:
		do_truncate(press_line);
		return;
	case wxID_RUN_TO_FRAME:
		do_set_stop_at_frame();
		return;
	case wxID_SCROLL_FRAME:
		do_scroll_to_frame();
		return;
	case wxID_SCROLL_CURRENT_FRAME:
		do_scroll_to_current_frame();
		return;
	case wxID_POSITION_LOCK:
		if(!current_popup)
			return;
		tmpitem = current_popup->FindItem(wxID_POSITION_LOCK);
		position_locked = tmpitem->IsChecked();
		return;
	case wxID_CHANGE_LINECOUNT:
		try {
			std::string text = pick_text(m, "Set number of lines", "Set number of lines visible:",
				(stringfmt() << lines_to_display).str());
			unsigned tmp = parse_value<unsigned>(text);
			if(tmp < 1 || tmp > 255)
				throw std::runtime_error("Value out of range");
			lines_to_display = tmp;
			m->get_scroll()->set_page_size(lines_to_display);
		} catch(canceled_exception& e) {
			return;
		} catch(std::exception& e) {
			wxMessageBox(wxT("Invalid value"), _T("Error"), wxICON_EXCLAMATION | wxOK, m);
			return;
		}
		signal_repaint();
		return;
	case wxID_COPY_FRAMES:
		if(press_index == std::numeric_limits<unsigned>::max())
			do_copy(rpress_line, press_line);
		else
			do_copy(rpress_line, press_line, port, controller);
		return;
	case wxID_CUT_FRAMES:
		if(press_index == std::numeric_limits<unsigned>::max())
			do_cut(rpress_line, press_line);
		else
			do_cut(rpress_line, press_line, port, controller);
		return;
	case wxID_PASTE_FRAMES:
		if(press_index == std::numeric_limits<unsigned>::max() || clipboard_get_data_type() == 1)
			do_paste(press_line, false);
		else
			do_paste(press_line, port, controller, false);
		return;
	case wxID_PASTE_APPEND:
		if(press_index == std::numeric_limits<unsigned>::max() || clipboard_get_data_type() == 1)
			do_paste(press_line, true);
		else
			do_paste(press_line, port, controller, true);
		return;
	case wxID_INSERT_CONTROLLER_AFTER:
		if(press_index == std::numeric_limits<unsigned>::max())
			;
		else
			do_insert_controller(press_line, port, controller);
		return;
	case wxID_DELETE_CONTROLLER_SUBFRAMES:
		if(press_index == std::numeric_limits<unsigned>::max())
			;
		else
			do_delete_controller(press_line, rpress_line, port, controller);
		return;
	};
}

uint64_t wxeditor_movie::_moviepanel::first_editable(unsigned index)
{
	uint64_t cffs = cached_cffs;
	if(!subframe_to_frame.count(cffs))
		return cffs;
	uint64_t f = subframe_to_frame[cffs];
	pollcounter_vector& pv = movb.get_movie().get_pollcounters();
	uint32_t pc = fcontrols.read_pollcount(pv, index);
	for(uint32_t i = 1; i < pc; i++)
		if(!subframe_to_frame.count(cffs + i) || subframe_to_frame[cffs + i] > f)
				return cffs + i;
	return cffs + pc;
}

uint64_t wxeditor_movie::_moviepanel::first_nextframe()
{
	uint64_t base = first_editable(0);
	if(!subframe_to_frame.count(cached_cffs))
		return cached_cffs;
	uint64_t f = subframe_to_frame[cached_cffs];
	for(uint32_t i = 0;; i++)
		if(!subframe_to_frame.count(base + i) || subframe_to_frame[base + i] > f)
			return base + i;
}

void wxeditor_movie::_moviepanel::on_mouse1(unsigned x, unsigned y, bool polarity) {}
void wxeditor_movie::_moviepanel::on_mouse2(unsigned x, unsigned y, bool polarity)
{
	if(polarity) {
		//Pressing mouse, just record line it was pressed on.
		rpress_line = spos + y - 3;
		return;
	}
	//Releasing mouse, open popup menu.
	unsigned off = divcnt + 1;
	press_x = x;
	if(y < 3)
		return;
	press_line = spos + y - 3;
	wxMenu menu;
	current_popup = &menu;
	menu.Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(wxeditor_movie::_moviepanel::on_popup_menu),
		NULL, this);

	//Find what controller is the click on.
	bool clicked_button = false;
	control_info clicked;
	std::string controller_name;
	if(press_x < off) {
		clicked_button = false;
		press_index = std::numeric_limits<unsigned>::max();
	} else {
		for(auto i : fcontrols.get_controlinfo())
			if(press_x >= i.position_left + off && press_x < i.position_left + i.reserved + off) {
				if(i.type == 0 || i.type == 1) {
					clicked_button = true;
					clicked = i;
					controller_name = (stringfmt() << "controller " << i.port << "-"
						<< (i.controller + 1)).str();
					press_index = i.index;
				}
			}
	}

	//Find first editable frame, controllerframe and buttonframe.
	bool not_editable = !movb.get_movie().readonly_mode();
	uint64_t eframe_low = first_editable(0);
	uint64_t ebutton_low = clicked_button ? first_editable(clicked.index) : std::numeric_limits<uint64_t>::max();
	uint64_t econtroller_low = ebutton_low;
	for(auto i : fcontrols.get_controlinfo())
		if(i.port == clicked.port && i.controller == clicked.controller && (i.type == 0 || i.type == 1))
			econtroller_low = max(econtroller_low, first_editable(i.index));

	bool click_zero = (clicked_button && !clicked.port && !clicked.controller);
	bool enable_append_frame = !not_editable;
	bool enable_toggle_button = false;
	bool enable_change_axis = false;
	bool enable_sweep_axis = false;
	bool enable_insert_frame = false;
	bool enable_insert_controller = false;
	bool enable_delete_frame = false;
	bool enable_delete_subframe = false;
	bool enable_delete_controller_subframe = false;
	bool enable_truncate_movie = false;
	bool enable_cut_frame = false;
	bool enable_copy_frame = false;
	bool enable_paste_frame = false;
	bool enable_paste_append = false;
	std::string copy_title;
	std::string paste_title;

	//Toggle button is enabled if clicked on button and either end is in valid range.
	enable_toggle_button = (!not_editable && clicked_button && clicked.type == 0 && ((press_line >= ebutton_low &&
		press_line < linecount) || (rpress_line >= ebutton_low && rpress_line < linecount)));
	//Change axis is enabled in similar conditions, except if type is axis.
	enable_change_axis = (!not_editable && clicked_button && clicked.type == 1 && ((press_line >= ebutton_low &&
		press_line < linecount) || (rpress_line >= ebutton_low && rpress_line < linecount)));
	//Sweep axis is enabled if change axis is enabled and lines don't match.
	enable_sweep_axis = (enable_change_axis && press_line != rpress_line);
	//Insert frame is enabled if this frame is completely editable and press and release lines match.
	enable_insert_frame = (!not_editable && press_line + 1 >= eframe_low && press_line < linecount &&
		press_line == rpress_line);
	//Insert controller frame is enabled if controller is completely editable and lines match.
	enable_insert_controller = (!not_editable && clicked_button && press_line >= econtroller_low &&
		press_line < linecount && press_line == rpress_line);
	enable_insert_controller = enable_insert_controller && (clicked.port || clicked.controller);
	//Delete frame is enabled if range is completely editable (relative to next-frame).
	enable_delete_frame = (!not_editable && press_line >= first_nextframe() && press_line < linecount &&
		rpress_line >= first_nextframe() && rpress_line < linecount);
	//Delete subframe is enabled if range is completely editable.
	enable_delete_subframe = (!not_editable && press_line >= eframe_low && press_line < linecount &&
		rpress_line >= eframe_low && rpress_line < linecount);
	//Delete controller subframe is enabled if range is completely controller-editable.
	enable_delete_controller_subframe = (!not_editable && clicked_button && press_line >= econtroller_low &&
		press_line < linecount && rpress_line >= econtroller_low && rpress_line < linecount);
	enable_delete_controller_subframe = enable_delete_controller_subframe && (clicked.port || clicked.controller);
	//Truncate movie is enabled if lines match and is completely editable.
	enable_truncate_movie = (!not_editable && press_line == rpress_line && press_line >= eframe_low &&
		press_line < linecount);
	//Cut frames is enabled if range is editable (possibly controller-editable).
	if(clicked_button)
		enable_cut_frame = (!not_editable && press_line >= econtroller_low && press_line < linecount
			&& rpress_line >= econtroller_low && rpress_line < linecount && !click_zero);
	else
		enable_cut_frame = (!not_editable && press_line >= eframe_low & press_line < linecount
			&& rpress_line >= eframe_low && rpress_line < linecount);
	if(clicked_button && clipboard_get_data_type() == 0) {
		enable_paste_append = (!not_editable && linecount >= eframe_low);
		enable_paste_frame = (!not_editable && press_line >= econtroller_low && press_line < linecount
			&& rpress_line >= econtroller_low && rpress_line < linecount && !click_zero);
	} else if(clipboard_get_data_type() == 1) {
		enable_paste_append = (!not_editable && linecount >= econtroller_low);
		enable_paste_frame = (!not_editable && press_line >= eframe_low & press_line < linecount
			&& rpress_line >= eframe_low && rpress_line < linecount);
	}
	//Copy frames is enabled if range exists.
	enable_copy_frame = (press_line < linecount && rpress_line < linecount);
	copy_title = (clicked_button ? controller_name : "frames");
	paste_title = ((clipboard_get_data_type() == 0) ? copy_title : "frames");

	if(clipboard_get_data_type() == 0 && click_zero) enable_paste_append = enable_paste_frame = false;

	if(enable_toggle_button)
		menu.Append(wxID_TOGGLE, towxstring(U"Toggle " + clicked.title));
	if(enable_change_axis)
		menu.Append(wxID_CHANGE, towxstring(U"Change " + clicked.title));
	if(enable_sweep_axis)
		menu.Append(wxID_SWEEP, towxstring(U"Sweep " + clicked.title));
	if(enable_toggle_button || enable_change_axis || enable_sweep_axis)
		menu.AppendSeparator();
	menu.Append(wxID_INSERT_AFTER, wxT("Insert frame after"))->Enable(enable_insert_frame);
	menu.Append(wxID_INSERT_CONTROLLER_AFTER, wxT("Insert controller frame"))
		->Enable(enable_insert_controller);
	menu.Append(wxID_APPEND_FRAME, wxT("Append frame"))->Enable(enable_append_frame);
	menu.Append(wxID_APPEND_FRAMES, wxT("Append frames..."))->Enable(enable_append_frame);
	menu.AppendSeparator();
	menu.Append(wxID_DELETE_FRAME, wxT("Delete frame(s)"))->Enable(enable_delete_frame);
	menu.Append(wxID_DELETE_SUBFRAME, wxT("Delete subframe(s)"))->Enable(enable_delete_subframe);
	menu.Append(wxID_DELETE_CONTROLLER_SUBFRAMES, wxT("Delete controller subframes(s)"))
		->Enable(enable_delete_controller_subframe);
	menu.AppendSeparator();
	menu.Append(wxID_TRUNCATE, wxT("Truncate movie"))->Enable(enable_truncate_movie);
	menu.AppendSeparator();
	menu.Append(wxID_CUT_FRAMES, towxstring("Cut " + copy_title))->Enable(enable_cut_frame);
	menu.Append(wxID_COPY_FRAMES, towxstring("Copy " + copy_title))->Enable(enable_copy_frame);
	menu.Append(wxID_PASTE_FRAMES, towxstring("Paste " + paste_title))->Enable(enable_paste_frame);
	menu.Append(wxID_PASTE_APPEND, towxstring("Paste append " + paste_title))->Enable(enable_paste_append);
	menu.AppendSeparator();
	menu.Append(wxID_SCROLL_FRAME, wxT("Scroll to frame..."));
	menu.Append(wxID_SCROLL_CURRENT_FRAME, wxT("Scroll to current frame"));
	menu.Append(wxID_RUN_TO_FRAME, wxT("Run to frame..."));
	menu.Append(wxID_CHANGE_LINECOUNT, wxT("Change number of lines visible"));
	menu.AppendCheckItem(wxID_POSITION_LOCK, wxT("Lock scroll to playback"))->Check(position_locked);
	menu.Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(wxeditor_movie::_moviepanel::on_popup_menu),
		NULL, this);
	PopupMenu(&menu);
}

int wxeditor_movie::_moviepanel::get_lines()
{
	controller_frame_vector& fv = movb.get_movie().get_frame_vector();
	return fv.size();
}

void wxeditor_movie::_moviepanel::signal_repaint()
{
	if(requested || recursing)
		return;
	auto s = m->get_scroll();
	requested = true;
	uint32_t width, height;
	uint64_t lines;
	wxeditor_movie* m2 = m;
	uint64_t old_cached_cffs = cached_cffs;
	uint32_t prev_width, prev_height;
	bool done_again = false;
do_again:
	runemufn([&lines, &width, &height, m2, this]() {
		lines = this->get_lines();
		if(lines < lines_to_display)
			this->moviepos = 0;
		else if(this->moviepos > lines - lines_to_display)
			this->moviepos = lines - lines_to_display;
		this->render(fb, moviepos);
		auto x = fb.get_characters();
		width = x.first;
		height = x.second;
	});
	if(old_cached_cffs != cached_cffs && position_locked && !done_again) {
		moviepos = cached_cffs;
		done_again = true;
		goto do_again;
	}
	prev_width = new_width;
	prev_height = new_height;
	new_width = width;
	new_height = height;
	movielines = lines;
	if(s) {
		s->set_range(lines);
		s->set_position(moviepos);
	}
	auto size = fb.get_pixels();
	pixels.resize(size.first * size.second * 3);
	fb.render((char*)&pixels[0]);
	if(prev_width != new_width || prev_height != new_height) {
		auto cell = fb.get_cell();
		SetMinSize(wxSize(new_width * cell.first, (lines_to_display + 3) * cell.second));
		if(new_width > 0 && s)
			m->Fit();
	}
	linecount = lines;
	Refresh();
}

void wxeditor_movie::_moviepanel::on_mouse(wxMouseEvent& e)
{
	auto cell = fb.get_cell();
	if(e.LeftDown() && !e.ControlDown())
		on_mouse0(e.GetX() / cell.first, e.GetY() / cell.second, true);
	if(e.LeftUp() && !e.ControlDown())
		on_mouse0(e.GetX() / cell.first, e.GetY() / cell.second, false);
	if(e.MiddleDown())
		on_mouse1(e.GetX() / cell.first, e.GetY() / cell.second, true);
	if(e.MiddleUp())
		on_mouse1(e.GetX() / cell.first, e.GetY() / cell.second, false);
	if(e.RightDown() || (e.LeftDown() && e.ControlDown()))
		on_mouse2(e.GetX() / cell.first, e.GetY() / cell.second, true);
	if(e.RightUp() || (e.LeftUp() && e.ControlDown()))
		on_mouse2(e.GetX() / cell.first, e.GetY() / cell.second, false);
	auto s = m->get_scroll();
	unsigned speed = 1;
	if(e.ShiftDown())
		speed = 10;
	if(e.ShiftDown() && e.ControlDown())
		speed = 50;
	s->apply_wheel(e.GetWheelRotation(), e.GetWheelDelta(), speed);
}

void wxeditor_movie::_moviepanel::on_erase(wxEraseEvent& e)
{
	//Blank.
}

void wxeditor_movie::_moviepanel::on_paint(wxPaintEvent& e)
{
	auto size = fb.get_pixels();
	if(!size.first || !size.second) {
		wxPaintDC dc(this);
		dc.Clear();
		requested = false;
		return;
	}
	wxPaintDC dc(this);
	wxBitmap bmp(wxImage(size.first, size.second, &pixels[0], true));
	dc.DrawBitmap(bmp, 0, 0, false);
	requested = false;
}

void wxeditor_movie::_moviepanel::do_copy(uint64_t row1, uint64_t row2, unsigned port, unsigned controller)
{
	frame_controls* _fcontrols = &fcontrols;
	uint64_t line = row1;
	uint64_t line2 = row2;
	if(line2 < line)
		std::swap(line, line2);
	std::string copied;
	runemufn([port, controller, line, line2, _fcontrols, &copied]() {
		controller_frame_vector& fv = movb.get_movie().get_frame_vector();
		uint64_t vsize = fv.size();
		if(!vsize)
			return;
		uint64_t _line = min(line, vsize - 1);
		uint64_t _line2 = min(line2, vsize - 1);
		copied = encode_lines(*_fcontrols, fv, _line, _line2 + 1, port, controller);
	});
	copy_to_clipboard(copied);
}

void wxeditor_movie::_moviepanel::do_copy(uint64_t row1, uint64_t row2)
{
	uint64_t line = row1;
	uint64_t line2 = row2;
	if(line2 < line)
		std::swap(line, line2);
	std::string copied;
	runemufn([line, line2, &copied]() {
		controller_frame_vector& fv = movb.get_movie().get_frame_vector();
		uint64_t vsize = fv.size();
		if(!vsize)
			return;
		uint64_t _line = min(line, vsize - 1);
		uint64_t _line2 = min(line2, vsize - 1);
		copied = encode_lines(fv, _line, _line2 + 1);
	});
	copy_to_clipboard(copied);
}

void wxeditor_movie::_moviepanel::do_cut(uint64_t row1, uint64_t row2, unsigned port, unsigned controller)
{
	do_copy(row1, row2, port, controller);
	do_delete_controller(row1, row2, port, controller);
}

void wxeditor_movie::_moviepanel::do_cut(uint64_t row1, uint64_t row2)
{
	do_copy(row1, row2);
	do_delete_frame(row1, row2, false);
}

void wxeditor_movie::_moviepanel::do_paste(uint64_t row, bool append)
{
	frame_controls* _fcontrols = &fcontrols;
	recursing = true;
	uint64_t _gapstart = row;
	std::string cliptext = copy_from_clipboard();
	runemufn([_fcontrols, &cliptext, _gapstart, append]() {
		//Insert enough lines for the pasted content.
		uint64_t gapstart = _gapstart;
		if(!movb.get_movie().readonly_mode())
			return;
		uint64_t gaplen = 0;
		int64_t newframes = 0;
		{
			std::istringstream y(cliptext);
			std::string z;
			if(!std::getline(y, z))
				return;
			istrip_CR(z);
			if(z != "lsnes-moviedata-whole")
				return;
			while(std::getline(y, z))
				gaplen++;
		}
		controller_frame_vector& fv = movb.get_movie().get_frame_vector();
		uint64_t vsize = fv.size();
		if(gapstart < real_first_editable(*_fcontrols, 0))
			return;
		if(gapstart > vsize)
			return;
		if(append) gapstart = vsize;
		for(uint64_t i = 0; i < gaplen; i++)
			fv.append(fv.blank_frame(false));
		for(uint64_t i = vsize - 1; i >= gapstart && i <= vsize; i--)
			fv[i + gaplen] = fv[i];
		//Write the pasted frames.
		{
			std::istringstream y(cliptext);
			std::string z;
			std::getline(y, z);
			uint64_t idx = gapstart;
			while(std::getline(y, z)) {
				fv[idx++].deserialize(z.c_str());
				if(fv[idx - 1].sync())
					newframes++;
			}
		}
		movie_framecount_change(newframes);
	});
	recursing = false;
	signal_repaint();
}

void wxeditor_movie::_moviepanel::do_paste(uint64_t row, unsigned port, unsigned controller, bool append)
{
	if(!port && !controller)
		return;
	frame_controls* _fcontrols = &fcontrols;
	auto iset = controller_index_set(fcontrols, port, controller);
	recursing = true;
	uint64_t _gapstart = row;
	std::string cliptext = copy_from_clipboard();
	runemufn([_fcontrols, iset, &cliptext, _gapstart, port, controller, append]() {
		//Insert enough lines for the pasted content.
		//TODO: Check that this won't alter the past.
		uint64_t gapstart = _gapstart;
		if(!movb.get_movie().readonly_mode())
			return;
		uint64_t gaplen = 0;
		int64_t newframes = 0;
		{
			std::istringstream y(cliptext);
			std::string z;
			if(!std::getline(y, z))
				return;
			istrip_CR(z);
			if(z != "lsnes-moviedata-controller")
				return;
			while(std::getline(y, z)) {
				gaplen++;
				newframes++;
			}
		}
		controller_frame_vector& fv = movb.get_movie().get_frame_vector();
		uint64_t vsize = fv.size();
		if(gapstart < real_first_editable(*_fcontrols, iset))
			return;
		if(gapstart > vsize)
			return;
		if(append) gapstart = vsize;
		for(uint64_t i = 0; i < gaplen; i++)
			fv.append(fv.blank_frame(true));
		move_index_set(*_fcontrols, fv, gapstart, gapstart + gaplen, vsize - gapstart, iset);
		//Write the pasted frames.
		{
			std::istringstream y(cliptext);
			std::string z;
			std::getline(y, z);
			uint64_t idx = gapstart;
			while(std::getline(y, z)) {
				controller_frame f = fv[idx++];
				decode_line(*_fcontrols, f, z, port, controller);
			}
		}
		movie_framecount_change(newframes);
	});
	recursing = false;
	signal_repaint();
}

void wxeditor_movie::_moviepanel::do_insert_controller(uint64_t row, unsigned port, unsigned controller)
{
	if(!port && !controller)
		return;
	frame_controls* _fcontrols = &fcontrols;
	auto iset = controller_index_set(fcontrols, port, controller);
	recursing = true;
	uint64_t gapstart = row;
	runemufn([_fcontrols, iset, gapstart, port, controller]() {
		//Insert enough lines for the pasted content.
		//TODO: Check that this won't alter the past.
		if(!movb.get_movie().readonly_mode())
			return;
		controller_frame_vector& fv = movb.get_movie().get_frame_vector();
		uint64_t vsize = fv.size();
		if(gapstart < real_first_editable(*_fcontrols, iset))
			return;
		if(gapstart > vsize)
			return;
		fv.append(fv.blank_frame(true));
		move_index_set(*_fcontrols, fv, gapstart, gapstart + 1, vsize - gapstart, iset);
		zero_index_set(*_fcontrols, fv, gapstart, 1, iset);
		movie_framecount_change(1);
	});
	recursing = false;
	signal_repaint();
}

void wxeditor_movie::_moviepanel::do_delete_controller(uint64_t row1, uint64_t row2, unsigned port,
	unsigned controller)
{
	if(!port && !controller)
		return;
	frame_controls* _fcontrols = &fcontrols;
	auto iset = controller_index_set(fcontrols, port, controller);
	recursing = true;
	if(row1 > row2) std::swap(row1, row2);
	uint64_t gapstart = row1;
	uint64_t gaplen = row2 - row1 + 1;
	runemufn([_fcontrols, iset, gapstart, gaplen, port, controller]() {
		//Insert enough lines for the pasted content.
		//TODO: Check that this won't alter the past.
		if(!movb.get_movie().readonly_mode())
			return;
		controller_frame_vector& fv = movb.get_movie().get_frame_vector();
		uint64_t vsize = fv.size();
		if(gapstart < real_first_editable(*_fcontrols, iset))
			return;
		if(gapstart > vsize)
			return;
		move_index_set(*_fcontrols, fv, gapstart + gaplen, gapstart, vsize - gapstart - gaplen, iset);
		zero_index_set(*_fcontrols, fv, vsize - gaplen, gaplen, iset);
	});
	recursing = false;
	signal_repaint();
}


wxeditor_movie::wxeditor_movie(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, wxT("lsnes: Edit movie"), wxDefaultPosition, wxSize(-1, -1))
{
	closing = false;
	Centre();
	wxFlexGridSizer* top_s = new wxFlexGridSizer(2, 1, 0, 0);
	SetSizer(top_s);

	wxBoxSizer* panel_s = new wxBoxSizer(wxHORIZONTAL);
	moviescroll = NULL;
	panel_s->Add(moviepanel = new _moviepanel(this), 1, wxGROW);
	panel_s->Add(moviescroll = new scroll_bar(this, wxID_ANY, true), 0, wxGROW);
	top_s->Add(panel_s, 1, wxGROW);

	moviescroll->set_page_size(lines_to_display);
	moviescroll->set_handler([this](scroll_bar& s) {
		this->moviepanel->moviepos = s.get_position();
		this->moviepanel->signal_repaint();
	});
	moviepanel->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(wxeditor_movie::on_keyboard_down), NULL, this);
	moviepanel->Connect(wxEVT_KEY_UP, wxKeyEventHandler(wxeditor_movie::on_keyboard_up), NULL, this);

	wxBoxSizer* pbutton_s = new wxBoxSizer(wxHORIZONTAL);
	pbutton_s->AddStretchSpacer();
	pbutton_s->Add(closebutton = new wxButton(this, wxID_OK, wxT("Close")), 0, wxGROW);
	closebutton->Connect(wxEVT_COMMAND_BUTTON_CLICKED,
		wxCommandEventHandler(wxeditor_movie::on_close), NULL, this);
	top_s->Add(pbutton_s, 0, wxGROW);

	moviepanel->SetFocus();
	moviescroll->Connect(wxEVT_SET_FOCUS, wxFocusEventHandler(wxeditor_movie::on_focus_wrong), NULL, this);
	closebutton->Connect(wxEVT_SET_FOCUS, wxFocusEventHandler(wxeditor_movie::on_focus_wrong), NULL, this);
	Connect(wxEVT_SET_FOCUS, wxFocusEventHandler(wxeditor_movie::on_focus_wrong), NULL, this);

	panel_s->SetSizeHints(this);
	pbutton_s->SetSizeHints(this);
	top_s->SetSizeHints(this);
	Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(wxeditor_movie::on_wclose));
	Fit();

	moviepanel->signal_repaint();
}

bool wxeditor_movie::ShouldPreventAppExit() const { return false; }

void wxeditor_movie::on_close(wxCommandEvent& e)
{
	movieeditor_open = NULL;
	Destroy();
	closing = true;
}

void wxeditor_movie::on_wclose(wxCloseEvent& e)
{
	bool wasc = closing;
	closing = true;
	movieeditor_open = NULL;
	if(!wasc)
		Destroy();
}

void wxeditor_movie::update()
{
	moviepanel->signal_repaint();
}

scroll_bar* wxeditor_movie::get_scroll()
{
	return moviescroll;
}

void wxeditor_movie::on_focus_wrong(wxFocusEvent& e)
{
	moviepanel->SetFocus();
}

void wxeditor_movie_display(wxWindow* parent)
{
	if(movieeditor_open)
		return;
	wxeditor_movie* v = new wxeditor_movie(parent);
	v->Show();
	movieeditor_open = v;
}

void wxeditor_movie::on_keyboard_down(wxKeyEvent& e)
{
	handle_wx_keyboard(e, true);
}

void wxeditor_movie::on_keyboard_up(wxKeyEvent& e)
{
	handle_wx_keyboard(e, false);
}

void wxeditor_movie_update()
{
	if(movieeditor_open)
		movieeditor_open->update();
}
