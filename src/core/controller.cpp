#include "lsnes.hpp"
#include <snes/snes.hpp>
#include <ui-libsnes/libsnes.hpp>

#include "core/command.hpp"
#include "core/controller.hpp"
#include "core/dispatch.hpp"
#include "core/framebuffer.hpp"
#include "core/mainloop.hpp"
#include "core/misc.hpp"
#include "core/window.hpp"

#include <map>
#include <sstream>

namespace
{
	std::map<std::string, std::pair<unsigned, unsigned>> buttonmap;

	void init_buttonmap()
	{
		static int done = 0;
		if(done)
			return;
		for(unsigned i = 0; i < 8; i++)
			for(unsigned j = 0; j < MAX_LOGICAL_BUTTONS; j++) {
				std::ostringstream x;
				x << (i + 1) << get_logical_button_name(j);
				buttonmap[x.str()] = std::make_pair(i, j);
			}
		done = 1;
	}

	//Do button action.
	void do_button_action(unsigned ui_id, unsigned button, short newstate, bool autoh)
	{
		int x = controls.lcid_to_pcid(ui_id);
		if(x < 0) {
			messages << "No such controller #" << (ui_id + 1) << std::endl;
			return;
		}
		int bid = controls.button_id(x, button);
		if(bid < 0) {
			messages << "Invalid button for controller type" << std::endl;
			return;
		}
		if(autoh) {
			controls.autohold(x, bid, controls.autohold(x, bid) ^ newstate);
			information_dispatch::do_autohold_update(x, bid, controls.autohold(x, bid));
		} else
			controls.button(x, bid, newstate);
	}

	function_ptr_command<tokensplitter&> autofire("autofire", "Set autofire pattern",
		"Syntax: autofire <buttons|->...\nSet autofire pattern\n",
		[](tokensplitter& t) throw(std::bad_alloc, std::runtime_error) {
			if(!t)
				throw std::runtime_error("Need at least one frame for autofire");
			std::vector<controller_frame> new_autofire_pattern;
			init_buttonmap();
			while(t) {
				std::string fpattern = t;
				if(fpattern == "-")
					new_autofire_pattern.push_back(controls.get_blank());
				else {
					controller_frame c(controls.get_blank());
					while(fpattern != "") {
						size_t split = fpattern.find_first_of(",");
						std::string button = fpattern;
						std::string rest;
						if(split < fpattern.length()) {
							button = fpattern.substr(0, split);
							rest = fpattern.substr(split + 1);
						}
						if(!buttonmap.count(button)) {
							std::ostringstream x;
							x << "Invalid button '" << button << "'";
							throw std::runtime_error(x.str());
						}
						auto g = buttonmap[button];
						int x = controls.lcid_to_pcid(g.first);
						if(x < 0) {
							std::ostringstream x;
							x << "No such controller #" << (g.first + 1) << std::endl;
							throw std::runtime_error(x.str());
						}
						int bid = controls.button_id(x, g.second);
						if(bid < 0) {
							std::ostringstream x;
							x << "Invalid button for controller type" << std::endl;
							throw std::runtime_error(x.str());
						}
						c.axis(x, bid, true);
						fpattern = rest;
					}
					new_autofire_pattern.push_back(c);
				}
			}
			controls.autofire(new_autofire_pattern);
		});

	class button_action : public command
	{
	public:
		button_action(const std::string& cmd, int _type, unsigned _controller, std::string _button)
			throw(std::bad_alloc)
			: command(cmd)
		{
			commandn = cmd;
			type = _type;
			controller = _controller;
			button = _button;
		}
		~button_action() throw() {}
		void invoke(const std::string& args) throw(std::bad_alloc, std::runtime_error)
		{
			if(args != "")
				throw std::runtime_error("This command does not take parameters");
			init_buttonmap();
			if(!buttonmap.count(button))
				return;
			auto i = buttonmap[button];
			do_button_action(i.first, i.second, (type != 1) ? 1 : 0, (type == 2));
			update_movie_state();
			information_dispatch::do_status_update();
		}
		std::string get_short_help() throw(std::bad_alloc)
		{
			return "Press/Unpress button";
		}
		std::string get_long_help() throw(std::bad_alloc)
		{
			return "Syntax: " + commandn + "\n"
				"Presses/Unpresses button\n";
		}
		std::string commandn;
		unsigned controller;
		int type;
		std::string button;
	};

	class button_action_helper
	{
	public:
		button_action_helper()
		{
			for(size_t i = 0; i < MAX_LOGICAL_BUTTONS; ++i)
				for(int j = 0; j < 3; ++j)
					for(unsigned k = 0; k < 8; ++k) {
						std::string x, y;
						char cstr[2] = {0, 0};
						cstr[0] = 49 + k;
						switch(j) {
						case 0:
							x = "+controller";
							break;
						case 1:
							x = "-controller";
							break;
						case 2:
							x = "controllerh";
							break;
						};
						x = x + cstr + get_logical_button_name(i);
						y = cstr + get_logical_button_name(i);
						our_commands.insert(new button_action(x, j, k, y));
					}
		}
		~button_action_helper()
		{
			for(auto i : our_commands)
				delete i;
			our_commands.clear();
		}
		std::set<command*> our_commands;
	} bah;
}

controller_state controls;
