/**
 * @file   main.cpp
 * @brief  Entry point for Camoto Studio.
 *
 * Copyright (C) 2010-2013 Adam Nielsen <malvineous@shikadi.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef WIN32
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#ifndef WIN32
#include <config.h>
#endif

#include <wx/app.h>
#include <wx/cmdline.h>
#include <wx/cshelp.h>
#include <wx/image.h>
#include <wx/stdpaths.h>

#include <png.h>
#include "main.hpp"
#include "studio.hpp"

paths path;
config_data config;

#ifdef WIN32
// Redirect stdout and stderr to the VS output window
#include <windows.h>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/tee.hpp>

using namespace std;
namespace io = boost::iostreams;

struct DebugSink
{
    typedef char char_type;
    typedef io::sink_tag category;

    std::vector<char> _vec;

    std::streamsize write(const char *s, std::streamsize n)
    {
        _vec.assign(s, s+n);
        _vec.push_back(0); // we must null-terminate for WINAPI
        OutputDebugStringA(&_vec[0]);
        return n;
    }
};
#endif // WIN32

class CamotoApp: public wxApp
{
	public:
		static const wxCmdLineEntryDesc cmdLineDesc[];

#ifdef WIN32
		// Redirect stdout and stderr to the VS output window
		typedef io::tee_device<DebugSink, std::streambuf> TeeDevice;
		TeeDevice device;
		io::stream_buffer<TeeDevice> buf;

		CamotoApp()
			:	device(DebugSink(), *std::cout.rdbuf()),
				buf(device)
		{
			_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
			std::cout.rdbuf(&this->buf);
			std::cerr.rdbuf(&buf);
		}
#if PNG_LIBPNG_VER != 10512
#define STRING2(x) #x
#define STRING(x) STRING2(x)
#pragma message ("libpng headers are version " STRING(PNG_LIBPNG_VER) )
#error Wrong version of libpng headers are being picked up, should be 1
#endif

#else
		CamotoApp()
		{
		}
#endif // WIN32

		void OnInitCmdLine(wxCmdLineParser& parser)
		{
			parser.SetDesc(CamotoApp::cmdLineDesc);
			return;
		}

		virtual bool OnCmdLineParsed(wxCmdLineParser& parser)
		{
			wxSimpleHelpProvider *helpProvider = new wxSimpleHelpProvider();
			wxHelpProvider::Set(helpProvider);

			wxImage::AddHandler(new wxPNGHandler());

			wxFileName next;
#ifdef WIN32
			// Use the 'data' subdir in the executable's dir
			next.AssignDir(wxStandardPaths::Get().GetDataDir());
			next.AppendDir(_T("data"));
			::path.dataRoot = next.GetFullPath();
#else
			// Use the value given to the configure script by --datadir
			::path.dataRoot = _T(DATA_PATH);
#endif
			std::cout << "[init] Data root is " << ::path.dataRoot.mb_str() << "\n";

			next.AssignDir(::path.dataRoot);
			next.AppendDir(_T("games"));
			::path.gameData = next.GetFullPath();

			next.AssignDir(::path.gameData);
			next.AppendDir(_T("screenshots"));
			::path.gameScreenshots = next.GetFullPath();

			next.AssignDir(::path.gameData);
			next.AppendDir(_T("icons"));
			::path.gameIcons = next.GetFullPath();

			next.AssignDir(::path.dataRoot);
			next.AppendDir(_T("icons"));
			::path.guiIcons = next.GetFullPath();

			next.AssignDir(::path.dataRoot);
			next.AppendDir(_T("maps"));
			::path.mapIndicators = next.GetFullPath();

			// Load the user's preferences
			wxConfigBase *configFile = wxConfigBase::Get(true);
			configFile->Read(_T("camoto/dosbox"), &::config.dosboxPath,
#ifndef __WXMSW__
				_T("/usr/bin/dosbox")
#else
				_T("dosbox.exe")
#endif
			);
			configFile->Read(_T("camoto/pause"), &::config.dosboxExitPause, false);
			configFile->Read(_T("camoto/lastpath"), &::path.lastUsed, wxEmptyString);
			configFile->Read(_T("camoto/mididev"), &::config.midiDevice, 0);
			configFile->Read(_T("camoto/pcmdelay"), &::config.pcmDelay, 0);

			Studio *f;
			wxString filename;
			if (parser.Found(_T("project"), &filename)) {
				f = new Studio(true);
				if (!::wxFileExists(filename)) {
					wxMessageDialog dlg(f, _("The supplied project file does not exist!"
						"  The --project option must be given the full path (and "
						"filename) of the 'project.camoto' file inside the project "
						"directory."),
						_("Open project"), wxOK | wxICON_ERROR);
					dlg.ShowModal();
				} else {
					f->openProject(filename);
				}
			} else if (parser.Found(_T("music"), &filename)) {
				f = new Studio(false);
				//f->loadMusic(filename);
				wxMessageDialog dlg(f, _("Sorry, standalone music editor not yet "
					"implemented!"),
					_("Open song"), wxOK | wxICON_ERROR);
				dlg.ShowModal();
			} else {
				f = new Studio(true);
			}
			f->Show(true);
			return true;
		}

		virtual bool OnInit()
		{
			std::cout << CAMOTO_HEADER "\n";
			if (!wxApp::OnInit()) return false;
			return true;
		}

		virtual int OnExit()
		{
			wxConfigBase *configFile = wxConfigBase::Get(true);
			configFile->Write(_T("camoto/lastpath"), ::path.lastUsed);
			configFile->Flush();
			return 0;
		}
};

const wxCmdLineEntryDesc CamotoApp::cmdLineDesc[] = {
	{wxCMD_LINE_OPTION, NULL, _("project"), _("Open the given project"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL},
	{wxCMD_LINE_OPTION, NULL, _("music"), _("Open a song in a standalone editor"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL},
	{wxCMD_LINE_NONE, NULL, NULL, NULL, (wxCmdLineParamType)NULL, 0}
};

IMPLEMENT_APP(CamotoApp)

void yield()
{
	wxGetApp().Yield();
	return;
}
