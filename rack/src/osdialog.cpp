// osdialog's functions for Rack modules built for Oroboro Modular: the
// system's own file and message dialogs (Windows here; elsewhere they
// answer as a dialog that was closed, for now). Written for the SDK to
// osdialog's interface; none of osdialog's code.
//
// The plugin asks a module's menu choices on a thread of its own (a
// dialog waits there, and the plugin's window goes on drawing), so a
// dialog has no owner window: it opens on its own, in front.

#include "../include/osdialog.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#endif

int oroboro_osdialog_closed = 0;

extern "C" {

char* osdialog_strdup(const char* s) {
	return s ? osdialog_strndup(s, std::strlen(s)) : nullptr;
}

char* osdialog_strndup(const char* s, size_t n) {
	if (!s)
		return nullptr;
	char* d = static_cast<char*>(std::malloc(n + 1));
	if (!d)
		return nullptr;
	std::memcpy(d, s, n);
	d[n] = 0;
	return d;
}

// "Name:ext,ext;Other:ext" → a list of filters, each a list of patterns
osdialog_filters* osdialog_filters_parse(const char* str) {
	if (!str)
		return nullptr;
	osdialog_filters* first = nullptr;
	osdialog_filters** next = &first;
	std::string text = str;
	size_t at = 0;
	while (at <= text.size()) {
		size_t end = text.find(';', at);
		if (end == std::string::npos)
			end = text.size();
		std::string part = text.substr(at, end - at);
		size_t colon = part.find(':');
		if (!part.empty() && colon != std::string::npos) {
			osdialog_filters* filter = static_cast<osdialog_filters*>(std::calloc(1, sizeof(osdialog_filters)));
			filter->name = osdialog_strndup(part.c_str(), colon);
			osdialog_filter_patterns** pattern = &filter->patterns;
			std::string list = part.substr(colon + 1);
			size_t p = 0;
			while (p <= list.size()) {
				size_t comma = list.find(',', p);
				if (comma == std::string::npos)
					comma = list.size();
				if (comma > p) {
					osdialog_filter_patterns* one = static_cast<osdialog_filter_patterns*>(std::calloc(1, sizeof(osdialog_filter_patterns)));
					one->pattern = osdialog_strndup(list.c_str() + p, comma - p);
					*pattern = one;
					pattern = &one->next;
				}
				p = comma + 1;
			}
			*next = filter;
			next = &filter->next;
		}
		at = end + 1;
	}
	return first;
}

void osdialog_filters_free(osdialog_filters* filters) {
	while (filters) {
		osdialog_filters* next = filters->next;
		osdialog_filter_patterns* pattern = filters->patterns;
		while (pattern) {
			osdialog_filter_patterns* after = pattern->next;
			std::free(pattern->pattern);
			std::free(pattern);
			pattern = after;
		}
		std::free(filters->name);
		std::free(filters);
		filters = next;
	}
}

#ifdef _WIN32

static std::wstring wide(const char* s) {
	if (!s || !*s)
		return std::wstring();
	int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
	std::wstring w(n > 0 ? n - 1 : 0, L'\0');
	if (n > 1)
		MultiByteToWideChar(CP_UTF8, 0, s, -1, &w[0], n);
	return w;
}

static char* narrow(const wchar_t* w) {
	int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
	if (n <= 0)
		return nullptr;
	char* s = static_cast<char*>(std::malloc(n));
	if (s)
		WideCharToMultiByte(CP_UTF8, 0, w, -1, s, n, nullptr, nullptr);
	return s;
}

int osdialog_message(osdialog_message_level level, osdialog_message_buttons buttons, const char* message) {
	if (oroboro_osdialog_closed)
		return 0;
	UINT type = MB_SETFOREGROUND | MB_TOPMOST;
	type |= level == OSDIALOG_ERROR ? MB_ICONERROR : level == OSDIALOG_WARNING ? MB_ICONWARNING : MB_ICONINFORMATION;
	type |= buttons == OSDIALOG_OK_CANCEL ? MB_OKCANCEL : buttons == OSDIALOG_YES_NO ? MB_YESNO : MB_OK;
	int answer = MessageBoxW(nullptr, wide(message).c_str(), L"Oroboro Modular", type);
	return answer == IDOK || answer == IDYES;
}

char* osdialog_prompt(osdialog_message_level level, const char* message, const char* text) {
	// (Windows has no prompt of its own: as closed, for now)
	return nullptr;
}

int osdialog_color_picker(osdialog_color* color, int opacity) {
	if (oroboro_osdialog_closed || !color)
		return 0;
	static COLORREF custom[16] = {};
	CHOOSECOLORW choose = {};
	choose.lStructSize = sizeof(choose);
	choose.rgbResult = RGB(color->r, color->g, color->b);
	choose.lpCustColors = custom;
	choose.Flags = CC_FULLOPEN | CC_RGBINIT;
	if (!ChooseColorW(&choose))
		return 0;
	color->r = GetRValue(choose.rgbResult);
	color->g = GetGValue(choose.rgbResult);
	color->b = GetBValue(choose.rgbResult);
	return 1;
}

char* osdialog_file(osdialog_file_action action, const char* dir, const char* filename, osdialog_filters* filters) {
	if (oroboro_osdialog_closed)
		return nullptr;
	HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	char* chosen = nullptr;
	if (action == OSDIALOG_OPEN_DIR) {
		BROWSEINFOW browse = {};
		browse.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
		browse.lpszTitle = L"Choose a folder";
		LPITEMIDLIST list = SHBrowseForFolderW(&browse);
		if (list) {
			wchar_t path[MAX_PATH] = {};
			if (SHGetPathFromIDListW(list, path))
				chosen = narrow(path);
			CoTaskMemFree(list);
		}
	}
	else {
		// the filters, as Windows takes them: "Name\0*.a;*.b\0…\0\0", and all files
		std::wstring pattern;
		for (osdialog_filters* f = filters; f; f = f->next) {
			std::wstring list;
			for (osdialog_filter_patterns* p = f->patterns; p; p = p->next) {
				if (!list.empty())
					list += L";";
				list += L"*." + wide(p->pattern);
			}
			pattern += wide(f->name) + L" (" + list + L")" + std::wstring(1, L'\0') + list + std::wstring(1, L'\0');
		}
		pattern += std::wstring(L"All files") + std::wstring(1, L'\0') + L"*.*" + std::wstring(1, L'\0') + std::wstring(1, L'\0');
		std::vector<wchar_t> path(32768, L'\0');
		std::wstring name = wide(filename);
		if (!name.empty() && name.size() < path.size())
			std::copy(name.begin(), name.end(), path.begin());
		std::wstring folder = wide(dir);
		OPENFILENAMEW open = {};
		open.lStructSize = sizeof(open);
		open.lpstrFilter = pattern.c_str();
		open.lpstrFile = path.data();
		open.nMaxFile = (DWORD) path.size();
		open.lpstrInitialDir = folder.empty() ? nullptr : folder.c_str();
		open.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
		bool ok;
		if (action == OSDIALOG_SAVE) {
			open.Flags |= OFN_OVERWRITEPROMPT;
			ok = GetSaveFileNameW(&open);
		}
		else {
			open.Flags |= OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
			ok = GetOpenFileNameW(&open);
		}
		if (ok)
			chosen = narrow(path.data());
	}
	if (SUCCEEDED(com))
		CoUninitialize();
	return chosen;
}

#else

int osdialog_message(osdialog_message_level level, osdialog_message_buttons buttons, const char* message) { return 0; }
char* osdialog_prompt(osdialog_message_level level, const char* message, const char* text) { return nullptr; }
int osdialog_color_picker(osdialog_color* color, int opacity) { return 0; }
char* osdialog_file(osdialog_file_action action, const char* dir, const char* filename, osdialog_filters* filters) { return nullptr; }

#endif

} // extern "C"
