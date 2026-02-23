//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "application.h"
#include "editor.h"
#include "map_window.h"
#include "main_menubar.h"
#include "artprovider.h"
#include "settings.h"

#include <wx/evtloop.h>
#include <wx/snglinst.h>

#if defined(__LINUX__) || defined(__WINDOWS__)
#	include <GL/glut.h>
#endif

#include "../brushes/icon/rme_icon.xpm"

wxDEFINE_EVENT(EVT_UPDATE_MENUS, wxCommandEvent);
wxDEFINE_EVENT(EVT_UPDATE_ACTIONS, wxCommandEvent);

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
	EVT_IDLE(MainFrame::OnIdle)
	EVT_CLOSE(MainFrame::OnExit)
	EVT_COMMAND(wxID_ANY, EVT_UPDATE_MENUS, MainFrame::OnUpdateMenus)
	EVT_COMMAND(wxID_ANY, EVT_UPDATE_ACTIONS, MainFrame::OnUpdateActions)
END_EVENT_TABLE()

wxIMPLEMENT_APP(Application);

Application::~Application()
{
	// Destroy
}

bool Application::OnInit()
{
#if defined(_DEBUG) && defined(__WINDOWS__)
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	std::cout << "This is free software: you are free to change and redistribute it." << std::endl;
	std::cout << "There is NO WARRANTY, to the extent permitted by law." << std::endl;
	std::cout << "Check LICENSE.txt in your RME distribution for details." << std::endl;
	mt_seed(time(nullptr));
	srand(time(nullptr));

	// Tell that we are the real thing
	wxAppConsole::SetInstance(this);
	wxArtProvider::Push(new ArtProvider());

#if defined(__LINUX__) || defined(__WINDOWS__)
	{
		int glutArgc = 1;
		glutInit(&glutArgc, argv);
	}
#endif

	// Load some internal stuff
	g_settings.load();
	FixVersionDiscrapencies();
	g_editor.LoadHotkeys();

	// Image handlers
	//wxImage::AddHandler(newd wxBMPHandler);
	wxImage::AddHandler(newd wxPNGHandler);
	wxImage::AddHandler(newd wxJPEGHandler);
	wxImage::AddHandler(newd wxTGAHandler);

	g_editor.gfx.loadEditorSprites();

#ifndef _DEBUG
	//wxHandleFatalExceptions(true);
#endif

    file_to_open = wxEmptyString;
	if(argc == 2){
		file_to_open = argv[1];
	}

	g_editor.root = newd MainFrame(__RME_APPLICATION_NAME__, wxDefaultPosition, wxSize(700, 600));
	g_editor.root->SetMinSize(wxSize(700, 600));

	SetTopWindow(g_editor.root);
	g_editor.SetTitle("");
	g_editor.LoadRecentFiles();
	g_editor.LoadPerspective();

    wxIcon icon(rme_icon);
    g_editor.root->SetIcon(icon);

    if(g_settings.getInteger(Config::WELCOME_DIALOG) == 1 && file_to_open == wxEmptyString) {
        g_editor.ShowWelcomeDialog(icon);
    } else {
        g_editor.root->Show();
    }

	// Set idle event handling mode
	wxIdleEvent::SetMode(wxIDLE_PROCESS_SPECIFIED);

	return true;
}

int Application::OnExit()
{
	return 1;
}

void Application::OnFatalException()
{
	// no-op
}

void Application::OnEventLoopEnter(wxEventLoopBase *loop) {
	if(loop->IsMain() && file_to_open != wxEmptyString){
		g_editor.OpenProject(file_to_open);
		file_to_open.Clear();
	}
}

#ifdef __WXMAC__
void Application::MacOpenFiles(const wxArrayString &fileNames)
{
	if(!fileNames.IsEmpty()) {
		g_editor.OpenProject(fileNames.Item(0));
	}
}
#endif

void Application::FixVersionDiscrapencies()
{
	// Here the registry should be fixed, if the version has been changed
	if(g_settings.getInteger(Config::VERSION_ID) < MAKE_VERSION_ID(1, 0, 5)) {
		g_settings.setInteger(Config::USE_MEMCACHED_SPRITES_TO_SAVE, 0);
	}

	wxString ss = wxstr(g_settings.getString(Config::SCREENSHOT_DIRECTORY));
	if(ss.empty()) {
		wxUniChar sep = wxFileName::GetPathSeparator();
		ss << wxStandardPaths::Get().GetDocumentsDir();
#ifdef __WINDOWS__
		ss << sep << "My Pictures" << sep << "RME";
#else
		ss << sep << "Pictures" << sep << "RME";
#endif
	}
	g_settings.setString(Config::SCREENSHOT_DIRECTORY, nstr(ss));

	// Set registry to newest version
	g_settings.setInteger(Config::VERSION_ID, __RME_VERSION_ID__);
}

void Application::Unload()
{
	g_editor.CloseProject();
	g_editor.SaveHotkeys();
	g_editor.SaveRecentFiles();
	g_settings.save();
}


MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size) :
	wxFrame(NULL, wxID_ANY, title, pos, size, wxDEFAULT_FRAME_STYLE)
{
	// Receive idle events
	SetExtraStyle(wxWS_EX_PROCESS_IDLE);

#if wxCHECK_VERSION(3, 1, 0)
	// Make sure ShowFullScreen() uses the full screen API on macOS
	EnableFullScreenView(true);
#endif

	// Creates the file-dropdown menu
	g_editor.menubar = newd MainMenuBar(this);
	g_editor.menubar->LoadDefault();

	g_editor.aui_manager = newd wxAuiManager(this);
	g_editor.toolbar = newd MainToolBar(this, g_editor.aui_manager);
	g_editor.mapWindow = newd MapWindow(this);
	g_editor.aui_manager->AddPane(g_editor.mapWindow, wxAuiPaneInfo().CenterPane().Floatable(false).CloseButton(false).PaneBorder(false));

	g_editor.aui_manager->Update();
	g_editor.UpdateMenubar();

	CreateStatusBar(5);
	SetStatusText(wxString("Welcome to ") << __RME_APPLICATION_NAME__ << " version " << __RME_VERSION__);
}

MainFrame::~MainFrame(){
	// no-op
}

void MainFrame::PrepareDC(wxDC& dc)
{
	dc.SetLogicalOrigin(0, 0 );
	dc.SetAxisOrientation(1, 0);
	dc.SetUserScale(1.0, 1.0);
	dc.SetMapMode(wxMM_TEXT);
}

#ifdef __WINDOWS__
bool MainFrame::MSWTranslateMessage(WXMSG *msg)
{
	if(g_editor.AreHotkeysEnabled()) {
		if(wxFrame::MSWTranslateMessage(msg))
			return true;
	} else {
		if(wxWindow::MSWTranslateMessage(msg))
			return true;
	}
	return false;
}
#endif


void MainFrame::OnUpdateMenus(wxCommandEvent&)
{
	g_editor.UpdateMenubar();
	g_editor.UpdateMinimap(true);
}

void MainFrame::OnUpdateActions(wxCommandEvent&)
{
	g_editor.toolbar->UpdateButtons();
	g_editor.RefreshActions();
}

void MainFrame::UpdateFloorMenu()
{
	g_editor.menubar->UpdateFloorMenu();
}

void MainFrame::UpdateIndicatorsMenu()
{
	g_editor.menubar->UpdateIndicatorsMenu();
}

void MainFrame::OnIdle(wxIdleEvent& event)
{
	// no-op
}

void MainFrame::OnExit(wxCloseEvent& event)
{
	if(g_editor.IsProjectOpen()) {
		if(!g_editor.CloseProject() && event.CanVeto()) {
			event.Veto();
			return;
		}
	}

	Application &app = static_cast<Application&>(wxGetApp());
	app.Unload();
	app.ExitMainLoop();
}

