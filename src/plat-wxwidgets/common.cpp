#include "plat-wxwidgets/common.hpp"

#ifdef __WXMAC__
#include <ApplicationServices/ApplicationServices.h>
#endif

wxString towxstring(const std::string& str)
{
	return wxString(str.c_str(), wxConvUTF8);
}

std::string tostdstring(const wxString& str)
{
	return std::string(str.mb_str(wxConvUTF8));
}

void foreground_application()
{
#ifdef __WXMAC__
	ProcessSerialNumber PSN;
	GetCurrentProcess(&PSN);
	TransformProcessType(&PSN,kProcessTransformToForegroundApplication);
#endif
}

long primary_window_style = wxMINIMIZE_BOX | wxSYSTEM_MENU | wxCAPTION | wxCLIP_CHILDREN | wxCLOSE_BOX;
long secondary_window_style = wxMINIMIZE_BOX | wxSYSTEM_MENU | wxCAPTION | wxCLIP_CHILDREN;

wxFrame* window1;
wxFrame* window2;
