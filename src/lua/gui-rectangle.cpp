#include "lua/internal.hpp"
#include "core/render.hpp"

namespace
{
	struct render_object_rectangle : public render_object
	{
		render_object_rectangle(int32_t _x, int32_t _y, uint32_t _width, uint32_t _height,
			premultiplied_color _outline, premultiplied_color _fill, uint32_t _thickness) throw()
			: x(_x), y(_y), width(_width), height(_height), outline(_outline), fill(_fill),
			thickness(_thickness) {}
		~render_object_rectangle() throw() {}
		void operator()(struct screen& scr) throw()
		{
			outline.set_palette(scr);
			fill.set_palette(scr);
			int32_t xmin = 0;
			int32_t xmax = width;
			int32_t ymin = 0;
			int32_t ymax = height;
			clip_range(scr.originx, scr.width, x, xmin, xmax);
			clip_range(scr.originy, scr.height, y, ymin, ymax);
			for(int32_t r = ymin; r < ymax; r++) {
				uint32_t* rptr = scr.rowptr(y + r + scr.originy);
				size_t eptr = x + xmin + scr.originx;
				for(int32_t c = xmin; c < xmax; c++, eptr++)
					if(r < thickness || c < thickness || r >= height - thickness ||
						c >= width - thickness)
						outline.apply(rptr[eptr]);
					else
						fill.apply(rptr[eptr]);
			}
		}
	private:
		int32_t x;
		int32_t y;
		uint32_t width;
		uint32_t height;
		premultiplied_color outline;
		premultiplied_color fill;
		uint32_t thickness;
	};

	function_ptr_luafun gui_rectangle("gui.rectangle", [](lua_State* LS, const std::string& fname) -> int {
		if(!lua_render_ctx)
			return 0;
		int64_t outline = 0xFFFFFFU;
		int64_t fill = -1;
		uint32_t thickness = 1;
		int32_t x = get_numeric_argument<int32_t>(LS, 1, fname.c_str());
		int32_t y = get_numeric_argument<int32_t>(LS, 2, fname.c_str());
		uint32_t width = get_numeric_argument<uint32_t>(LS, 3, fname.c_str());
		uint32_t height = get_numeric_argument<uint32_t>(LS, 4, fname.c_str());
		get_numeric_argument<uint32_t>(LS, 5, thickness, fname.c_str());
		get_numeric_argument<int64_t>(LS, 6, outline, fname.c_str());
		get_numeric_argument<int64_t>(LS, 7, fill, fname.c_str());
		premultiplied_color poutline(outline);
		premultiplied_color pfill(fill);
		lua_render_ctx->queue->add(*new render_object_rectangle(x, y, width, height, poutline, pfill,
			thickness));
		return 0;
	});
}
