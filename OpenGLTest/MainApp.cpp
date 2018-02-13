#include "MainFrame.h"

#include <wx/app.h>
#include <wx/msgdlg.h>

class MainApp : public wxApp
{
public:
	virtual bool OnInit() override;
};

IMPLEMENT_APP(MainApp);

bool MainApp::OnInit()
{
    //
    // Create frame and start
    //

	MainFrame* frame = new MainFrame();
    frame->Show();
    SetTopWindow(frame);

	return true;
}