#include "OpenGLTest.h"
#include "RenderContext.h"
#include "Vectors.h"

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

    bool RenderSetup();
    bool RenderOld();
    void RenderCleanup();

    unsigned int mShaderProgram;

private:

    std::unique_ptr<RenderContext> mRenderContext;

private:

    void CreateWorld();
    float GetOceanFloorHeight(float x, float seaDepth) const;

    struct Point
    {
        vec2f Position;
        vec3f Colour;
        float Water;
        float Light;

        int RenderIndex;

        vec3f GetColour()
        {
            static constexpr vec3f LightPointColour = vec3f(1.0f, 1.0f, 0.25f);
            static constexpr vec3f WetPointColour = vec3f(0.0f, 0.0f, 0.8f);

            float const colorWetness = fminf(Water, 1.0f) * 0.7f;

            vec3f colour1 = Colour * (1.0f - colorWetness)
                + WetPointColour * colorWetness;

            //if (Light == 0.0f)
            //    return colour1;

            float const colorLightness = fminf(Light, 1.0f) * 0.95f;

            return colour1 * (1.0f - colorLightness)
                + LightPointColour * colorLightness;
        }
    };

    struct Spring
    {
        Point * const PointA;
        Point * const PointB;

        Spring(Point * a, Point * b)
            : PointA(a)
            , PointB(b)
        {}
    };

    struct Triangle
    {
        Point * const PointA;
        Point * const PointB;
        Point * const PointC;

        Triangle(Point * a, Point * b, Point * c)
            : PointA(a)
            , PointB(b)
            , PointC(c)
        {}
    };

    static constexpr int WorldWidth = 100;
    static constexpr int WorldHeight = 70;
    Point mPoints[WorldWidth][WorldHeight];
    std::vector<Spring> mSprings;
    std::vector<Triangle> mTriangles;

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
