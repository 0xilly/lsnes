#include "lua.hpp"
#include "sdmp.hpp"
#include "settings.hpp"
#include "misc.hpp"
#include <iomanip>
#include <cassert>
#include <cstring>
#include <sstream>
#include <zlib.h>
#include "misc.hpp"
#include "avsnoop.hpp"
#include "command.hpp"

namespace
{
	class sdmp_avsnoop : public av_snooper
	{
	public:
		sdmp_avsnoop(const std::string& prefix, bool ssflag) throw(std::bad_alloc)
			: av_snooper("SDMP")
		{
			dumper = new sdump_dumper(prefix, ssflag);
		}

		~sdmp_avsnoop() throw()
		{
			delete dumper;
		}

		void frame(struct lcscreen& _frame, uint32_t fps_n, uint32_t fps_d, const uint32_t* raw, bool hires,
			bool interlaced, bool overscan, unsigned region) throw(std::bad_alloc, std::runtime_error)
		{
			unsigned flags = 0;
			dumper->frame(raw, (hires ? SDUMP_FLAG_HIRES : 0) | (interlaced ? SDUMP_FLAG_INTERLACED : 0) |
				(overscan ? SDUMP_FLAG_OVERSCAN : 0) | (region == SNOOP_REGION_PAL ? SDUMP_FLAG_PAL :
				0));
		}

		void sample(short l, short r) throw(std::bad_alloc, std::runtime_error)
		{
			dumper->sample(l, r);
		}

		void end() throw(std::bad_alloc, std::runtime_error)
		{
			dumper->end();
		}
	private:
		sdump_dumper* dumper;
	};

	sdmp_avsnoop* vid_dumper;

	function_ptr_command<const std::string&> jmd_dump("dump-sdmp", "Start sdmp capture",
		"Syntax: dump-sdmp <prefix>\nStart SDMP capture to <prefix>\n",
		[](const std::string& prefix) throw(std::bad_alloc, std::runtime_error) {
			if(prefix == "")
				throw std::runtime_error("Expected prefix");
			if(vid_dumper)
				throw std::runtime_error("SDMP Dump already in progress");
			try {
				vid_dumper = new sdmp_avsnoop(prefix, false);
			} catch(std::bad_alloc& e) {
				throw;
			} catch(std::exception& e) {
				std::ostringstream x;
				x << "Error starting SDMP dump: " << e.what();
				throw std::runtime_error(x.str());
			}
			messages << "Dumping SDMP to " << prefix << std::endl;
		});

	function_ptr_command<const std::string&> jmd_dumpss("dump-sdmpss", "Start SS sdmp capture",
		"Syntax: dump-sdmpss <file>\nStart SS SDMP capture to <file>\n",
		[](const std::string& prefix) throw(std::bad_alloc, std::runtime_error) {
			if(prefix == "")
				throw std::runtime_error("Expected filename");
			if(vid_dumper)
				throw std::runtime_error("SDMP Dump already in progress");
			try {
				vid_dumper = new sdmp_avsnoop(prefix, true);
			} catch(std::bad_alloc& e) {
				throw;
			} catch(std::exception& e) {
				std::ostringstream x;
				x << "Error starting SDMP dump: " << e.what();
				throw std::runtime_error(x.str());
			}
			messages << "Dumping SDMP to " << prefix << std::endl;
		});

	function_ptr_command<> end_avi("end-sdmp", "End SDMP capture",
		"Syntax: end-sdmp\nEnd a SDMP capture.\n",
		[]() throw(std::bad_alloc, std::runtime_error) {
			if(!vid_dumper)
				throw std::runtime_error("No SDMP video dump in progress");
			try {
				vid_dumper->end();
				messages << "SDMP Dump finished" << std::endl;
			} catch(std::bad_alloc& e) {
				throw;
			} catch(std::exception& e) {
				messages << "Error ending SDMP dump: " << e.what() << std::endl;
			}
			delete vid_dumper;
			vid_dumper = NULL;
		});
}
