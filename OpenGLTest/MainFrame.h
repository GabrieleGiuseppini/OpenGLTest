#include "OpenGLTest.h"

#include <wx/filedlg.h>
#include <wx/frame.h>
#include <wx/glcanvas.h>
#include <wx/menu.h>
#include <wx/timer.h>

#include <cstdint>
#include <memory>

/*
 * The main window of the OpenGLTest GUI.
 */
class MainFrame : public wxFrame
{
public:

	MainFrame();

	virtual ~MainFrame();

private:

	//
	// Canvas
	//

	std::unique_ptr<wxGLCanvas> mMainGLCanvas;
	std::unique_ptr<wxGLContext> mMainGLCanvasContext;

	//
	// Timers
	//

	std::unique_ptr<wxTimer> mGameTimer;
	std::unique_ptr<wxTimer> mStatsRefreshTimer;

private:

	//
	// Event handlers
	//

	// App
	void OnMainFrameClose(wxCloseEvent& event);
	void OnQuit(wxCommandEvent& event);
	void OnPaint(wxPaintEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnGameTimerTrigger(wxTimerEvent& event);
	void OnStatsRefreshTimerTrigger(wxTimerEvent& event);

	// Main GL canvas
	void OnMainGLCanvasResize(wxSizeEvent& event);
	void OnMainGLCanvasLeftDown(wxMouseEvent& event);
	void OnMainGLCanvasLeftUp(wxMouseEvent& event);
	void OnMainGLCanvasRightDown(wxMouseEvent& event);
	void OnMainGLCanvasRightUp(wxMouseEvent& event);
	void OnMainGLCanvasMouseMove(wxMouseEvent& event);
	void OnMainGLCanvasMouseWheel(wxMouseEvent& event);

	// Menu
	void OnAboutMenuItemSelected(wxCommandEvent& event);

private:

    bool Render();

private:

	struct MouseInfo
	{
		bool ldown;
		bool rdown;
		int x;
		int y;

		MouseInfo()
			: ldown(false)
			, rdown(false)
			, x(0)
			, y(0)
		{
		}
	};
	
	MouseInfo mMouseInfo;

	uint64_t mFrameCount;	
};
