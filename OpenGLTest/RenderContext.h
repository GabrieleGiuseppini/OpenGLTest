/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "OpenGLTest.h"

#include <cassert>
#include <memory>
#include <string>
#include <vector>

class RenderContext
{
public:

    RenderContext();
    
    ~RenderContext();

public:

    void SetCanvasSize(int width, int height);

    void SetZoom(float zoom);
    
    void SetCameraPosition(int x, int y);

    void SetAmbientLightStrength(float strength);

public:

    void RenderStart();
    
    void RenderLandStart(size_t elements);

    inline void RenderLand(
        float left,
        float right,
        float leftTop,
        float rightTop,
        float bottom)
    {
        assert(mLandBufferSize + 1u <= mLandBufferMaxSize);

        LandElement * landElement = &(mLandBuffer[mLandBufferSize]);

        landElement->x1 = left;
        landElement->y1 = leftTop;
        landElement->x2 = right;
        landElement->y2 = rightTop;
        landElement->x3 = left;
        landElement->y3 = bottom;
        landElement->x4 = right;
        landElement->y4 = bottom;

        ++mLandBufferSize;
    }

    void RenderLandEnd();

    // TODOHERE

    inline void RenderWater(
        float x1,
        float y1,
        float x2,
        float y2)
    {
        // TODO: might want to have ad-hoc VertexShader fill-in the Z coordinate with -1, and
        //       ad-hoc FragmentShader to use fixed water color
        // TODO: store in ad-hoc buffer
    }

    inline void RenderSpring(
        float x1,
        float y1,
        float r1,
        float g1,
        float b1,
        float x2,
        float y2,
        float r2,
        float g2,
        float b2)
    {
        // TODO: might want to have ad-hoc VertexShader fill-in the Z coordinate with -1
        // TODO: store in ad-hoc buffer
    }

    inline void RenderStressedSpring(
        float x1,
        float y1,
        float x2,
        float y2)
    {
        // TODO: might want to have ad-hoc VertexShader fill-in the Z coordinate with -1, and
        //       ad-hoc FragmentShader to use fixed stressed color
        // TODO: store in ad-hoc buffer
    }


    //
    // Ship triangles
    //

    void RenderShipTrianglesStart(size_t elements);

    inline void RenderShipTriangle(
        float x1,
        float y1,
        float r1,
        float g1,
        float b1,
        float x2,
        float y2,
        float r2,
        float g2,
        float b2,
        float x3,
        float y3,
        float r3,
        float g3,
        float b3)
    {
        assert(mShipTriangleBufferSize + 1u <= mShipTriangleBufferMaxSize);

        ShipTriangleElement * shipTriangleElement = &(mShipTriangleBuffer[mShipTriangleBufferSize]);

        shipTriangleElement->x1 = x1;
        shipTriangleElement->y1 = y1;
        shipTriangleElement->r1 = r1;
        shipTriangleElement->g1 = g1;
        shipTriangleElement->b1 = b1;

        shipTriangleElement->x2 = x2;
        shipTriangleElement->y2 = y2;
        shipTriangleElement->r2 = r2;
        shipTriangleElement->g2 = g2;
        shipTriangleElement->b2 = b2;

        shipTriangleElement->x3 = x3;
        shipTriangleElement->y3 = y3;
        shipTriangleElement->r3 = r3;
        shipTriangleElement->g3 = g3;
        shipTriangleElement->b3 = b3;

        ++mShipTriangleBufferSize;
    }

    void RenderShipTrianglesEnd();

    void RenderEnd();

private:
    
    void CompileShader(
        char const * shaderSource,
        GLenum shaderType,
        GLuint shaderProgram);

    void LinkProgram(
        GLuint shaderProgram,
        std::string const & programName);

    GLint GetParameterLocation(
        GLuint shaderProgram,
        std::string const & parameterName);

    void CalculateOrthoMatrix(
        float zoom,
        float camX,
        float camY,
        int canvasWidth,
        int canvasHeight);

private:

    //
    // The shader programs
    //

    GLuint mLandShaderProgram;
    GLint mLandShaderAmbientLightStrengthParameter;
    GLint mLandShaderAmbientLightColorParameter;
    GLint mLandShaderLandColorParameter;
    GLint mLandShaderOrthoMatrixParameter;

    GLuint mShipTriangleShaderProgram;
    GLint mShipTriangleShaderAmbientLightStrengthParameter;
    GLint mShipTriangleShaderAmbientLightColorParameter;
    GLint mShipTriangleShaderOrthoMatrixParameter;

#pragma pack(push)
    struct LandElement
    {
        float x1;
        float y1;
        float x2;
        float y2;
        float x3;
        float y3;
        float x4;
        float y4;
    };

    struct ShipTriangleElement
    {
        float x1;
        float y1;
        float r1;
        float g1;
        float b1;

        float x2;
        float y2;
        float r2;
        float g2;
        float b2;

        float x3;
        float y3;
        float r3;
        float g3;
        float b3;
    };
#pragma pack(pop)

    std::unique_ptr<LandElement[]> mLandBuffer;
    size_t mLandBufferSize;
    size_t mLandBufferMaxSize;

    std::unique_ptr<ShipTriangleElement[]> mShipTriangleBuffer;
    size_t mShipTriangleBufferSize;
    size_t mShipTriangleBufferMaxSize;

    // The Ortho matrix
    float mOrthoMatrix[4][4];

    // The current render parameters
    float mZoom;
    float mCamX;
    float mCamY;
    int mCanvasWidth;
    int mCanvasHeight;
};
