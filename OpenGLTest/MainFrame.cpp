#include "OpenGLTest.h"

#include "MainFrame.h"

#include <wx/intl.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/string.h>

#include <cassert>
#include <chrono>
#include <sstream>

namespace /* anonymous */ {

	std::string GetWindowTitle()
	{
		return std::string("OpenGLTest 1.0");
	}
}

const long ID_MAIN_CANVAS = wxNewId();

const long ID_QUIT_MENUITEM = wxNewId();
const long ID_ABOUT_MENUITEM = wxNewId();

const long ID_GAME_TIMER = wxNewId();
const long ID_STATS_REFRESH_TIMER = wxNewId();

MainFrame::MainFrame()
	: mMouseInfo()
	, mFrameCount(0u)
{
	Create(
		nullptr, 
		wxID_ANY,
		GetWindowTitle(),		
		wxDefaultPosition, 
		wxDefaultSize, 
		wxDEFAULT_FRAME_STYLE, 
		_T("Main Frame"));

	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

	wxPanel* mainPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS);
	mainPanel->Bind(wxEVT_CHAR_HOOK, (wxObjectEventFunction)&MainFrame::OnKeyDown, this);

	wxBoxSizer * mainFrameSizer = new wxBoxSizer(wxHORIZONTAL);

	Connect(this->GetId(), wxEVT_CLOSE_WINDOW, (wxObjectEventFunction)&MainFrame::OnMainFrameClose);
	Connect(this->GetId(), wxEVT_PAINT, (wxObjectEventFunction)&MainFrame::OnPaint);


	//
	// Build main GL canvas
	//
	
	int mainGLCanvasAttributes[] = 
	{
		WX_GL_RGBA,
		WX_GL_DOUBLEBUFFER,
        WX_GL_DEPTH_SIZE,      16,
        WX_GL_STENCIL_SIZE,    0,

        // We want to use OpenGL 3.3, Core Profile        
        // TBD: Not now, my laptop does not support OpenGL 3 :-(
        // WX_GL_CORE_PROFILE,
        WX_GL_MAJOR_VERSION,    2,
        WX_GL_MINOR_VERSION,    0,

		0, 0 
	};

	mMainGLCanvas = std::make_unique<wxGLCanvas>(
		mainPanel, 
		ID_MAIN_CANVAS,
		mainGLCanvasAttributes,
		wxDefaultPosition,
		wxSize(640, 480),
		0L,
		_T("Main GL Canvas"));	

	mMainGLCanvas->Connect(wxEVT_SIZE, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasResize, 0, this);
	mMainGLCanvas->Connect(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasLeftDown, 0, this);
	mMainGLCanvas->Connect(wxEVT_LEFT_UP, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasLeftUp, 0, this);
	mMainGLCanvas->Connect(wxEVT_RIGHT_DOWN, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasRightDown, 0, this);
	mMainGLCanvas->Connect(wxEVT_RIGHT_UP, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasRightUp, 0, this);
	mMainGLCanvas->Connect(wxEVT_MOTION, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasMouseMove, 0, this);
	mMainGLCanvas->Connect(wxEVT_MOUSEWHEEL, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasMouseWheel, 0, this);
	
	mainFrameSizer->Add(
		mMainGLCanvas.get(),
		1,					// Proportion
		wxALL | wxEXPAND,	// Flags
		0);					// Border	

	// Take context for this canvas
	mMainGLCanvasContext = std::make_unique<wxGLContext>(mMainGLCanvas.get());
	mMainGLCanvasContext->SetCurrent(*mMainGLCanvas);

    //
    // Initialize OpenGL
    //

    InitOpenGL();

    glViewport(0, 0, mMainGLCanvas->GetSize().GetWidth(), mMainGLCanvas->GetSize().GetHeight());

    RenderSetup();


	//
	// Build menu
	//

	wxMenuBar * mainMenuBar = new wxMenuBar();
	

	// File

	wxMenu * fileMenu = new wxMenu();
	
	wxMenuItem* quitMenuItem = new wxMenuItem(fileMenu, ID_QUIT_MENUITEM, _("Quit\tAlt-F4"), _("Quit the application"), wxITEM_NORMAL);
	fileMenu->Append(quitMenuItem);
	Connect(ID_QUIT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnQuit);

	mainMenuBar->Append(fileMenu, _("&File"));


	// Help

	wxMenu * helpMenu = new wxMenu();

	wxMenuItem * aboutMenuItem = new wxMenuItem(helpMenu, ID_ABOUT_MENUITEM, _("About\tF1"), _("Show info about this application"), wxITEM_NORMAL);
	helpMenu->Append(aboutMenuItem);
	Connect(ID_ABOUT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnAboutMenuItemSelected);

	mainMenuBar->Append(helpMenu, _("Help"));

	SetMenuBar(mainMenuBar);


	//
	// Finalize frame
	//

	mainPanel->SetSizerAndFit(mainFrameSizer);
	
	Maximize();
	Centre();



	//
	// Initialize timers
	//
	
	mGameTimer = std::make_unique<wxTimer>(this, ID_GAME_TIMER);
	Connect(ID_GAME_TIMER, wxEVT_TIMER, (wxObjectEventFunction)&MainFrame::OnGameTimerTrigger);	
	mGameTimer->Start(0, true); 

	mStatsRefreshTimer = std::make_unique<wxTimer>(this, ID_STATS_REFRESH_TIMER);
	Connect(ID_STATS_REFRESH_TIMER, wxEVT_TIMER, (wxObjectEventFunction)&MainFrame::OnStatsRefreshTimerTrigger);	
	mStatsRefreshTimer->Start(1000, false);
}

MainFrame::~MainFrame()
{
}

//
// App event handlers
//

void MainFrame::OnMainFrameClose(wxCloseEvent & /*event*/)
{
	mGameTimer->Stop();
	mStatsRefreshTimer->Stop();

	Destroy();

    RenderCleanup();
}

void MainFrame::OnQuit(wxCommandEvent & /*event*/)
{
	Close();
}

void MainFrame::OnPaint(wxPaintEvent& event)
{
	//Render();

	event.Skip();
}

void MainFrame::OnKeyDown(wxKeyEvent & event)
{
	event.Skip();
}

void MainFrame::OnGameTimerTrigger(wxTimerEvent & /*event*/)
{
	// Render
    if (Render())
    {
        // Make the timer for the next step start now
        mGameTimer->Start(0, true);

        ++mFrameCount;
    }
}

void MainFrame::OnStatsRefreshTimerTrigger(wxTimerEvent & /*event*/)
{
	std::wostringstream ss;
	ss << GetWindowTitle();
	ss << "  FPS: " << mFrameCount;

	SetTitle(ss.str());

	mFrameCount = 0u;
}

//
// Main canvas event handlers
//

void MainFrame::OnMainGLCanvasResize(wxSizeEvent & event)
{
    glViewport(0, 0, event.GetSize().GetWidth(), event.GetSize().GetHeight());
}

void MainFrame::OnMainGLCanvasLeftDown(wxMouseEvent & /*event*/)
{
    // Remember the mouse button is down
    mMouseInfo.ldown = true;
}

void MainFrame::OnMainGLCanvasLeftUp(wxMouseEvent & /*event*/)
{
	// Remember the mouse button is not down anymore
	mMouseInfo.ldown = false;
}

void MainFrame::OnMainGLCanvasRightDown(wxMouseEvent & /*event*/)
{
    // Remember the mouse button is down
	mMouseInfo.rdown = true;
}

void MainFrame::OnMainGLCanvasRightUp(wxMouseEvent & /*event*/)
{
    // Remember the mouse button is not down anymore
	mMouseInfo.rdown = false;
}

void MainFrame::OnMainGLCanvasMouseMove(wxMouseEvent& event)
{
	mMouseInfo.x = event.GetX();
	mMouseInfo.y = event.GetY();
}

void MainFrame::OnMainGLCanvasMouseWheel(wxMouseEvent& event)
{
    event.Skip();
}


//
// Menu event handlers
//

void MainFrame::OnAboutMenuItemSelected(wxCommandEvent & /*event*/)
{
	wxMessageBox("Yeah!", L"OpenGLTest");
}

/////////////////////////////////////////////////////////////////////////////////////////////////

//   GGGGG  RRRR        A     PPPP     H     H  IIIIIII    CCC      SSS
//  GG      R   RR     A A    P   PP   H     H     I      CC CC   SS   SS
// GG       R    RR   A   A   P    PP  H     H     I     CC    C  S
// G        R   RR   A     A  P   PP   H     H     I     C        SS
// G        RRRR     AAAAAAA  PPPP     HHHHHHH     I     C          SSS
// G  GGGG  R RR     A     A  P        H     H     I     C             SS
// GG    G  R   R    A     A  P        H     H     I     CC    C        S
//  GG  GG  R    R   A     A  P        H     H     I      CC CC   SS   SS
//   GGGG   R     R  A     A  P        H     H  IIIIIII    CCC      SSS

////bool MainFrame::Render_Works()
////{
////    int compileSuccess;
////    char infoLog[1024];
////
////    //
////    // Clear canvas 
////    //
////
////    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
////    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
////
////
////    //
////    // Array buffer
////    //
////
////    unsigned int myVBO;
////    glGenBuffers(1, &myVBO);
////    glBindBuffer(GL_ARRAY_BUFFER, myVBO);
////
////    float vertices[] = {
////        -0.5f, -0.5f, 0.0f,
////        0.5f, -0.5f, 0.0f,
////        0.0f,  0.5f, 0.0f
////    };
////
////    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
////
////    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
////    glEnableVertexAttribArray(0);
////
////
////    //
////    // Program
////    //
////
////    GLhandleARB myProgram;
////    myProgram = glCreateProgramObjectARB();
////
////    //
////    // Vertex shader
////    //
////    {
////        GLhandleARB myVertexShader;
////        myVertexShader = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
////
////        const char *vertexShaderSource = R"(
////        attribute vec3 aPos;
////        varying vec4 color;
////    
////        void main()
////        {
////            gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
////            color = gl_Color;
////        }
////        )";
////
////        glShaderSourceARB(myVertexShader, 1, &vertexShaderSource, NULL);
////        glCompileShaderARB(myVertexShader);
////
////        // Check
////        glGetObjectParameterivARB(myVertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &compileSuccess);
////        if (!compileSuccess)
////        {
////            glGetShaderInfoLog(myVertexShader, sizeof(infoLog), NULL, infoLog);
////            wxMessageBox(infoLog, "ERROR Compiling vertex shader");
////            return false;
////        }
////
////        glAttachObjectARB(myProgram, myVertexShader);
////        glDeleteObjectARB(myVertexShader);
////    }
////
////
////    //
////    // Fragment shader
////    //
////
////    {
////        GLhandleARB myFragmentShader;
////        myFragmentShader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
////
////        const char *fragmentShaderSource = R"(
////        varying vec4 color;
////
////        void main()
////        {
////            vec4 color = vec4(1.0, 0.5, 0.2, 1.0);
////            gl_FragColor = color;
////        } 
////        )";
////
////        glShaderSourceARB(myFragmentShader, 1, &fragmentShaderSource, NULL);
////        glCompileShaderARB(myFragmentShader);
////
////        // Check
////        glGetObjectParameterivARB(myFragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &compileSuccess);
////        if (!compileSuccess)
////        {
////            glGetShaderInfoLog(myFragmentShader, sizeof(infoLog), NULL, infoLog);
////            wxMessageBox(infoLog, "ERROR Compiling fragment shader");
////            return false;
////        }
////
////        glAttachObjectARB(myProgram, myFragmentShader);
////        glDeleteObjectARB(myFragmentShader);
////    }
////
////
////    //
////    // Link program
////    //
////    {
////        glLinkProgramARB(myProgram);
////        glGetObjectParameterivARB(myProgram, GL_OBJECT_LINK_STATUS_ARB, &compileSuccess);
////
////        if (!compileSuccess)
////        {
////            // Get the error message and display it.
////            glGetInfoLogARB(myProgram, sizeof(infoLog), nullptr, infoLog);
////            wxMessageBox(infoLog, "ERROR Linking program");
////            return false;
////        }
////    }
////
////    //
////    // Draw
////    //
////
////    glUseProgramObjectARB(myProgram);
////
////
////    glDrawArrays(GL_TRIANGLES, 0, 3);
////
////    // Flush all the draw operations and flip the back buffer onto the screen.	
////    glFlush();
////    mMainGLCanvas->SwapBuffers();
////
////    glUseProgramObjectARB(0);
////    glDeleteObjectARB(myProgram);
////
////    return true;
////}

bool MainFrame::RenderSetup()
{
    int compileSuccess;
    char infoLog[1024];


    //
    // Shader program
    //

    mShaderProgram = glCreateProgram();

    {
        //
        // Vertex shader
        //

        unsigned int myVertexShader = glCreateShader(GL_VERTEX_SHADER);

        const char *vertexShaderSource = R"(
            attribute vec3 InputPos1;
            attribute vec3 InputCol1;
            varying vec4 InternalColor1;
            uniform mat4 ParamProjTrans1;
            void main()
            {
                InternalColor1 = vec4(InputCol1.xyz, 1.0);
                gl_Position = ParamProjTrans1 * vec4(InputPos1.xyz, 1.0);
            }
            )";

        glShaderSource(myVertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(myVertexShader);

        // Check
        glGetShaderiv(myVertexShader, GL_COMPILE_STATUS, &compileSuccess);
        if (!compileSuccess)
        {
            glGetShaderInfoLog(myVertexShader, sizeof(infoLog), NULL, infoLog);
            wxMessageBox(infoLog, "ERROR Compiling vertex shader");
            return false;
        }

        // Attach
        glAttachShader(mShaderProgram, myVertexShader);

        glDeleteShader(myVertexShader);
    }

    {
        //
        // Fragment shader
        //

        unsigned int myFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

        const char *fragmentShaderSource = R"(
            varying vec4 InternalColor1;
            uniform vec4 ParamBaseColor1;
            void main()
            {
                gl_FragColor = InternalColor1 + ParamBaseColor1;
            } 
            )";

        glShaderSource(myFragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(myFragmentShader);

        // Check
        glGetShaderiv(myFragmentShader, GL_COMPILE_STATUS, &compileSuccess);
        if (!compileSuccess)
        {
            glGetShaderInfoLog(myFragmentShader, sizeof(infoLog), NULL, infoLog);
            wxMessageBox(infoLog, "ERROR Compiling fragment shader");
            return false;
        }

        // Attach
        glAttachShader(mShaderProgram, myFragmentShader);

        glDeleteShader(myFragmentShader);
    }



    //
    // Link program
    //

    // Bind attribute locations
    glBindAttribLocation(mShaderProgram, 0, "InputPos1");
    glBindAttribLocation(mShaderProgram, 1, "InputCol1");

    // Link
    glLinkProgram(mShaderProgram);
    
    // Check
    glGetProgramiv(mShaderProgram, GL_LINK_STATUS, &compileSuccess);
    if (!compileSuccess)
    {
        glGetShaderInfoLog(mShaderProgram, sizeof(infoLog), NULL, infoLog);
        wxMessageBox(infoLog, "ERROR Linking program");
        return false;
    }



    //
    // Use program
    //

    glUseProgram(mShaderProgram);


    return true;
}

bool MainFrame::Render()
{
    //
    // Clear canvas 
    //

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //
    // Array buffer
    //

    float vertices[] = {
        // positions         // colors
        0.5f, -0.1f, 0.0f,  1.0f, 0.0f, 0.0f,  // bottom right
        -0.5f, -0.1f, 0.0f,  0.0f, 1.0f, 0.0f,  // bottom left
        0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,   // top 
        0.0f,  -0.5f, 0.0f,  0.0f, 0.0f, 1.0f   // bottom
    };
    
    unsigned int myVBO;
    glGenBuffers(1, &myVBO);
    glBindBuffer(GL_ARRAY_BUFFER, myVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Col
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    //
    // Setup elements
    //

    unsigned int indices[] =
    {
        0, 1, 2,   // first triangle
        0, 1, 3    // second triangle
    };

    unsigned int myEBO;
    glGenBuffers(1, &myEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, myEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);




    //
    // Set params
    //

    static auto startTime = std::chrono::steady_clock::now();
    auto phase = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count() % 5000;
    float phaseValue = static_cast<float>(phase) / 5000.0f;
    int baseColor1Location = glGetUniformLocation(mShaderProgram, "ParamBaseColor1");
    glUniform4f(baseColor1Location, 0.0f, phaseValue, 0.0f, 1.0f);

    float projTrans1[4][4] =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    float translateX = phaseValue;
    float translateY = phaseValue;
    projTrans1[3][0] = translateX;
    projTrans1[3][1] = translateY;

    int projTrans1Location = glGetUniformLocation(mShaderProgram, "ParamProjTrans1");
    glUniformMatrix4fv(projTrans1Location, 1, GL_FALSE, &projTrans1[0][0]);

    
    //
    // Draw
    //

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    //glDrawArrays(GL_TRIANGLES, 0, 3);

    glFlush();

    mMainGLCanvas->SwapBuffers();

    return true;
}

void MainFrame::RenderCleanup()
{
    glUseProgram(0);
}


