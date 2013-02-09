#ifndef _customfont__hpp__included__
#define _customfont__hpp__included__

#include <vector>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <map>
#include "framebuffer.hpp"

class ligature_key
{
public:
	ligature_key(const std::vector<uint32_t>& key) throw(std::bad_alloc);
	const std::vector<uint32_t>& get() const throw() { return ikey; }
	size_t length() const throw() { return ikey.size(); }
	bool operator<(const ligature_key& key) const throw();
	bool operator<=(const ligature_key& key) const throw() { return !(key < *this); }
	bool operator==(const ligature_key& key) const throw();
	bool operator!=(const ligature_key& key) const throw() { return !(key == *this); }
	bool operator>=(const ligature_key& key) const throw() { return !(*this < key); }
	bool operator>(const ligature_key& key) const throw() { return key < *this; }
private:
	std::vector<uint32_t> ikey;
};

struct font_glyph_data
{
	font_glyph_data();
	font_glyph_data(std::istream& s);
	unsigned width;
	unsigned height;
	unsigned stride;
	std::vector<uint32_t> glyph;	//Bitpacked, element breaks between rows.
	void render(framebuffer<false>& fb, int32_t x, int32_t y, premultiplied_color fg, premultiplied_color bg)
		const;
	void render(framebuffer<true>& fb, int32_t x, int32_t y, premultiplied_color fg, premultiplied_color bg) const;
};

struct custom_font
{
public:
	custom_font();
	custom_font(const std::string& file);
	void add(const ligature_key& key, const font_glyph_data& glyph) throw(std::bad_alloc);
	ligature_key best_ligature_match(const std::vector<uint32_t>& codepoints, size_t start) const
		throw(std::bad_alloc);
	const font_glyph_data& lookup_glyph(const ligature_key& key) const throw();
	unsigned get_rowadvance() const throw() { return rowadvance; }
private:
	std::map<ligature_key, font_glyph_data> glyphs;
	unsigned rowadvance;
};

#endif