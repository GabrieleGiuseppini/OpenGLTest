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
const long ID_TRANSPARENT_WATER_MENUITEM = wxNewId();
const long ID_DRAW_ONLY_POINTS_MENUITEM = wxNewId();
const long ID_ABOUT_MENUITEM = wxNewId();

const long ID_GAME_TIMER = wxNewId();
const long ID_STATS_REFRESH_TIMER = wxNewId();

MainFrame::MainFrame()
	: mIsWaterTransparent(false)
    , mDrawOnlyPoints(false)
    , mMouseInfo()
	, mFrameCount(0u)
    , mCurrentTime(0.0f)
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
	// Build menu
	//

	wxMenuBar * mainMenuBar = new wxMenuBar();

    // Control

    wxMenu * controlMenu = new wxMenu();

    wxMenuItem* transparentWaterMenuItem = new wxMenuItem(controlMenu, ID_TRANSPARENT_WATER_MENUITEM, _("Transparent Water\tW"), _("Make water transparent"), wxITEM_CHECK);
    controlMenu->Append(transparentWaterMenuItem);
    transparentWaterMenuItem->Check(false);
    this->Bind(
        wxEVT_MENU,
        [this](wxCommandEvent & event)
        {
            this->mIsWaterTransparent = event.IsChecked();
        },
        ID_TRANSPARENT_WATER_MENUITEM);

    wxMenuItem* drawOnlyPointsMenuItem = new wxMenuItem(controlMenu, ID_DRAW_ONLY_POINTS_MENUITEM, _("Draw Only Points\tP"), _("Draw only points"), wxITEM_CHECK);
    controlMenu->Append(drawOnlyPointsMenuItem);
    drawOnlyPointsMenuItem->Check(false);
    this->Bind(
        wxEVT_MENU,
        [this](wxCommandEvent & event)
        {
            this->mDrawOnlyPoints = event.IsChecked();
        },
        ID_DRAW_ONLY_POINTS_MENUITEM);

    mainMenuBar->Append(controlMenu, _("&Control"));



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

    try
    {
        //
        // Initialize OpenGL
        //

        InitOpenGL();

        mRenderContext = std::unique_ptr<RenderContext>(new RenderContext());

        //
        // Initialize timers
        //

        mGameTimer = std::make_unique<wxTimer>(this, ID_GAME_TIMER);
        Connect(ID_GAME_TIMER, wxEVT_TIMER, (wxObjectEventFunction)&MainFrame::OnGameTimerTrigger);
        mGameTimer->Start(0, true);

        mStatsRefreshTimer = std::make_unique<wxTimer>(this, ID_STATS_REFRESH_TIMER);
        Connect(ID_STATS_REFRESH_TIMER, wxEVT_TIMER, (wxObjectEventFunction)&MainFrame::OnStatsRefreshTimerTrigger);
        mStatsRefreshTimer->Start(1000, false);


        //
        // Create world
        //

        CreateWorld();
    }
    catch (std::exception const & ex)
    {
        wxMessageBox(ex.what(), "ERROR");

        Close();
    }
}

MainFrame::~MainFrame()
{
}

//
// App event handlers
//

void MainFrame::OnMainFrameClose(wxCloseEvent & /*event*/)
{
    if (!!mGameTimer)
	    mGameTimer->Stop();
    if (!!mStatsRefreshTimer)
	    mStatsRefreshTimer->Stop();

	Destroy();
}

void MainFrame::OnQuit(wxCommandEvent & /*event*/)
{
	Close();
}

void MainFrame::OnPaint(wxPaintEvent& event)
{
	event.Skip();
}

void MainFrame::OnKeyDown(wxKeyEvent & event)
{
    if (event.GetKeyCode() == '+')
    {
        mRenderContext->SetZoom(mRenderContext->GetZoom() - 2.0f);
    }
    else if (event.GetKeyCode() == '-')
    {
        mRenderContext->SetZoom(mRenderContext->GetZoom() + 2.0f);
    }
    else if (event.GetKeyCode() == 314)
    {
        // Left
        mRenderContext->SetCameraPosition(vec2f(
            mRenderContext->GetCameraPosition().x - 10.0f, 
            mRenderContext->GetCameraPosition().y));
    }
    else if (event.GetKeyCode() == 315)
    {
        // Up
        mRenderContext->SetCameraPosition(vec2f(
            mRenderContext->GetCameraPosition().x,
            mRenderContext->GetCameraPosition().y + 10.0f));
    }
    else if (event.GetKeyCode() == 316)
    {
        // Right
        mRenderContext->SetCameraPosition(vec2f(
            mRenderContext->GetCameraPosition().x + 10.0f,
            mRenderContext->GetCameraPosition().y));
    }
    else if (event.GetKeyCode() == 317)
    {
        mRenderContext->SetCameraPosition(vec2f(
            mRenderContext->GetCameraPosition().x,
            mRenderContext->GetCameraPosition().y - 10.0f));
    }

	event.Skip();
}

void MainFrame::OnGameTimerTrigger(wxTimerEvent & /*event*/)
{
    // Make the timer for the next step start now
    mGameTimer->Start(0, true);

    //
    // Calculate ambient light intensity
    //

    static auto startTime = std::chrono::steady_clock::now();
    auto phase = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
    mRenderContext->SetAmbientLightIntensity((1.0f + sinf(static_cast<float>(phase) / 2500.0f)) / 2.0f);

    //
    // Render
    //

    assert(nullptr != mRenderContext);
    mRenderContext->RenderStart();

    //
    // Land
    //

    static constexpr int LeftLand = -140;
    static constexpr int RightLand = 140;
    static constexpr float SeaDepth = 60.0f;

    mRenderContext->RenderLandStart(RightLand - LeftLand);

    for (int i = LeftLand; i < RightLand; ++i)
    {
        mRenderContext->RenderLand(
            static_cast<float>(i),
            static_cast<float>(i + 1), 
            GetOceanFloorHeight(static_cast<float>(i), SeaDepth),
            GetOceanFloorHeight(static_cast<float>(i + 1), SeaDepth),
            -SeaDepth);
    }    
    
    mRenderContext->RenderLandEnd();

    if (mIsWaterTransparent)
    {
        RenderWater();
    }

    //
    // Upload points
    //

    mRenderContext->UploadShipPointStart(WorldWidth * WorldHeight);

    int currentIndex = 0;
    for (int c = 0; c < WorldWidth; ++c)
    {
        for (int r = 0; r < WorldHeight; ++r)
        {
            Point * a = &(mPoints[c][r]);
            vec3f Colour = a->GetColour(mRenderContext->GetAmbientLightIntensity());

            mRenderContext->UploadShipPoint(
                a->Position.x,
                a->Position.y,
                Colour.x,
                Colour.y,
                Colour.z);

            a->RenderIndex = currentIndex;

            ++currentIndex;
        }
    }

    mRenderContext->UploadShipPointEnd();


    if (mDrawOnlyPoints)
    {
        mRenderContext->RenderShipPoints();
    }
    else
    {
        //
        // Springs
        //

        mRenderContext->RenderSpringsStart(mSprings.size());

        for (Spring const & spring : mSprings)
        {
            mRenderContext->RenderSpring(
                spring.PointA->RenderIndex,
                spring.PointB->RenderIndex);
        }

        mRenderContext->RenderSpringsEnd();


        mRenderContext->RenderStressedSpringsStart(mSprings.size());

        for (Spring const & spring : mSprings)
        {
            if (spring.IsStressed)
            {
                mRenderContext->RenderStressedSpring(
                    spring.PointA->RenderIndex,
                    spring.PointB->RenderIndex);
            }
        }

        mRenderContext->RenderStressedSpringsEnd();


        //
        // Triangles
        //

        mRenderContext->RenderShipTrianglesStart(mTriangles.size());

        for (Triangle const & triangle : mTriangles)
        {
            mRenderContext->RenderShipTriangle(
                triangle.PointA->RenderIndex,
                triangle.PointB->RenderIndex,
                triangle.PointC->RenderIndex);
        }

        mRenderContext->RenderShipTrianglesEnd();
    }

    if (!mIsWaterTransparent)
    {
        RenderWater();
    }

    //
    // End
    //

    mRenderContext->RenderEnd();

    mMainGLCanvas->SwapBuffers();

    ++mFrameCount;
    mCurrentTime += 0.2f;
}

void MainFrame::OnStatsRefreshTimerTrigger(wxTimerEvent & /*event*/)
{
	std::wostringstream ss;
	ss << GetWindowTitle();
	ss << "  FPS: " << mFrameCount << ", Triangles: " << mTriangles.size();

	SetTitle(ss.str());

	mFrameCount = 0u;
}

//
// Main canvas event handlers
//

void MainFrame::OnMainGLCanvasResize(wxSizeEvent & event)
{
    if (!!mRenderContext)
    {
        mRenderContext->SetCanvasSize(event.GetSize().GetWidth(), event.GetSize().GetHeight());
    }
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
            varying vec3 InternalColor1;
            uniform mat4 ParamProjTrans1;
            void main()
            {
                InternalColor1 = InputCol1;
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
            varying vec3 InternalColor1;
            uniform float ParamAmbientLightStrength;
            uniform vec3 ParamAmbientLightColor;
            void main()
            {
                vec3 ambientLight = ParamAmbientLightStrength * ParamAmbientLightColor;
                gl_FragColor = vec4(ambientLight * InternalColor1, 1.0);
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

bool MainFrame::RenderOld()
{
    //
    // Clear canvas 
    //

    glClearColor(0.529f, 0.808f, 0.980f, 1.0f); // (cornflower blue)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //
    // Array buffer
    //

    float vertices[] = {
        // positions         // colors
        0.5f, -0.1f, -1.0f,  1.0f, 0.0f, 0.0f,  // bottom right
        -0.5f, -0.1f, -1.0f,  0.0f, 1.0f, 0.0f,  // bottom left
        0.0f,  0.5f, -1.0f,  0.0f, 0.0f, 1.0f,   // top 
        0.0f,  -0.5f, -1.0f,  0.0f, 0.0f, 1.0f   // bottom
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
    int ambientLightStrengthLocation = glGetUniformLocation(mShaderProgram, "ParamAmbientLightStrength");
    glUniform1f(ambientLightStrengthLocation, phaseValue);

    int ambientLightColorLocation = glGetUniformLocation(mShaderProgram, "ParamAmbientLightColor");
    glUniform3f(ambientLightColorLocation, 1.0, 1.0, 1.0);


    float translateX = phaseValue;
    float translateY = phaseValue;

    float zoom = 1.0f;
    float right = 1024.0f / 768.0f * zoom;
    float left = -right;
    float top = zoom;
    float bottom = -zoom;
    float zNear = 1.0f;
    float zFar = 1000.0f;

    float orthoTrans[4][4] =
    {
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 1
    };

    orthoTrans[0][0] = 2.0f / (right - left);
    orthoTrans[1][1] = 2.0f / (top - bottom);
    orthoTrans[2][2] = -2.0f / (zFar - zNear);
    orthoTrans[3][0] = -(right + left) / (right - left) + translateX;
    orthoTrans[3][1] = -(top + bottom) / (top - bottom) + translateY;
    orthoTrans[3][2] = -(zFar + zNear) / (zFar - zNear);

    int projTrans1Location = glGetUniformLocation(mShaderProgram, "ParamProjTrans1");
    glUniformMatrix4fv(projTrans1Location, 1, GL_FALSE, &orthoTrans[0][0]);


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

void MainFrame::CreateWorld()
{
    // 
    // Create points
    //

    for (int c = 0; c < WorldWidth; ++c)
    {
        float x = static_cast<float>(c) - static_cast<float>(WorldWidth) / 2.0f;

        for (int r = 0; r < WorldHeight; ++r)
        {            
            float y = static_cast<float>(r) - static_cast<float>(WorldHeight) / 2.0f;

            mPoints[c][r].Position = vec2f(x, y);

            if (r == 0 || r == WorldHeight - 1 || c == 0 || c == WorldWidth - 1
                || r == WorldHeight / 2 || c == WorldWidth / 2)
            {
                mPoints[c][r].Colour = vec3f(0.2f, 0.2f, 0.2f);
            }
            else if ((r == WorldHeight / 3 || r == WorldHeight * 2 / 3)
                && (c >= WorldWidth / 3 && c <= WorldWidth * 2 / 3))
            {
                mPoints[c][r].Colour = vec3f(0.6f, 0.2f, 0.2f);
            }
            else if ((r >= WorldHeight / 3 && r <= WorldHeight * 2 / 3)
                && (c == WorldWidth / 3 || c == WorldWidth * 2 / 3))
            {
                mPoints[c][r].Colour = vec3f(0.6f, 0.2f, 0.2f);
            }
            else
            {
                mPoints[c][r].Colour = vec3f(0.9f, 0.9f, 0.9f);
            }


            float distance = mPoints[c][r].Position.length();

            if (distance > 20.0f && distance < 40.0f)
            {
                float d = (distance - 30.0f) / 10.0f; // -1 <= d <= 1
                mPoints[c][r].Water = 1.0f - (d * d);
            }
            else
            {
                mPoints[c][r].Water = 0.0f;
            }

            if (distance == 0)
            {
                mPoints[c][r].Light = 1.0f;
            }
            else if (distance < 10.0f)
            {
                mPoints[c][r].Light = 1.0f / (distance * distance);
            }
            else
            { 
                mPoints[c][r].Light = 0.0f;
            }
        }
    }


    //
    // Create springs and triangles
    //

    static const int Directions[8][2] = {
        { 1,  0 },	// E
        { 1, -1 },	// NE
        { 0, -1 },	// N
        { -1, -1 },	// NW
        { -1,  0 },	// W
        { -1,  1 },	// SW
        { 0,  1 },	// S
        { 1,  1 }	// SE
    };

    for (int c = 0; c < WorldWidth; ++c)
    {
        for (int r = 0; r < WorldHeight; ++r)
        {
            Point * pA = &(mPoints[c][r]);

            for (int i = 0; i < 4; ++i)
            {
                int adjc1 = c + Directions[i][0];
                int adjr1 = r + Directions[i][1];
                
                if (adjc1 >= 0 && adjc1 < WorldWidth && adjr1 >= 0)
                {
                    //
                    // Create a<->b spring
                    // 

                    Point * pB = &(mPoints[adjc1][adjr1]);

                    bool isStressed = (0 == (adjc1 % 10) && 0 == (adjr1 % 10));

                    mSprings.emplace_back(pA, pB, isStressed);

                    int adjc2 = c + Directions[i + 1][0];
                    int adjr2 = r + Directions[i + 1][1];
                    
                    if (adjc2 >= 0 && adjc2 < WorldWidth && adjr2 >= 0)
                    {
                        if (adjc2 >= 20 && adjc2 < WorldWidth - 20 && adjr2 >= 20 && adjr2 < WorldHeight - 20)
                        {
                            //
                            // Create a<->b<->c triangle
                            //

                            Point * pC = &(mPoints[adjc2][adjr2]);

                            mTriangles.emplace_back(pA, pB, pC);
                        }
                    }
                }
            }
        }
    }
}

float MainFrame::GetOceanFloorHeight(float x, float seaDepth) const
{
    float const c1 = sinf(x * 0.05f) * 6.f;
    float const c2 = sinf(x * 0.15f) * 2.f;
    float const c3 = sin(x * 0.011f) * 25.f;
    return -seaDepth + (c1 + c2 - c3) + 33.f;
}

float MainFrame::GetWaterHeight(float x, float waveHeight) const
{
    float const c1 = sinf(x * 0.1f + mCurrentTime) * 0.5f;
    float const c2 = sinf(x * 0.3f - mCurrentTime * 1.1f) * 0.3f;
    return (c1 + c2) * waveHeight;
}

void MainFrame::RenderWater()
{
    //
    // Water
    //

    static constexpr int LeftWater = -140;
    static constexpr int RightWater = 140;
    static constexpr float WaveHeight = 2.0f;
    static constexpr float SeaDepth = 60.0f;

    mRenderContext->RenderWaterStart(RightWater - LeftWater);

    for (int i = LeftWater; i < RightWater; ++i)
    {
        mRenderContext->RenderWater(
            static_cast<float>(i),
            static_cast<float>(i + 1),
            GetWaterHeight(static_cast<float>(i), WaveHeight),
            GetWaterHeight(static_cast<float>(i + 1), WaveHeight),
            -SeaDepth);
    }

    mRenderContext->RenderWaterEnd();
}