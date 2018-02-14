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

    void RenderShipTrianglesStart(size_t points, size_t triangles);

    inline void RenderShipTriangle_Point(
        float x,
        float y,
        float r,
        float g,
        float b)
    {
        assert(mShipTrianglePointBufferSize + 1u <= mShipTrianglePointBufferMaxSize);

        ShipTriangleElement_Point * shipTriangleElement_Point = &(mShipTrianglePointBuffer[mShipTrianglePointBufferSize]);

        shipTriangleElement_Point->x = x;
        shipTriangleElement_Point->y = y;
        shipTriangleElement_Point->r = r;
        shipTriangleElement_Point->g = g;
        shipTriangleElement_Point->b = b;

        ++mShipTrianglePointBufferSize;
    }

    inline void RenderShipTriangle_Triangle(
        int index1,
        int index2,
        int index3)
    {
        assert(mShipTriangleTriangleBufferSize + 1u <= mShipTriangleTriangleBufferMaxSize);

        ShipTriangleElement_Triangle * shipTriangleElement_Triangle = &(mShipTriangleTriangleBuffer[mShipTriangleTriangleBufferSize]);

        shipTriangleElement_Triangle->index1 = index1;
        shipTriangleElement_Triangle->index2 = index2;
        shipTriangleElement_Triangle->index3 = index3;

        ++mShipTriangleTriangleBufferSize;
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
    GLint mLandShaderAmbientLightParameter;
    GLint mLandShaderLandColorParameter;
    GLint mLandShaderOrthoMatrixParameter;
    GLuint mLandShaderVBO;

    GLuint mShipTriangleShaderProgram;
    GLint mShipTriangleShaderAmbientLightParameter;
    GLint mShipTriangleShaderOrthoMatrixParameter;
    GLuint mShipTriangleShaderPointVBO;
    GLuint mShipTriangleShaderTriangleVBO;

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

    struct ShipTriangleElement_Point
    {
        float x;
        float y;
        float r;
        float g;
        float b;
    };

    struct ShipTriangleElement_Triangle
    {
        int index1;
        int index2;
        int index3;
    };
#pragma pack(pop)

    std::unique_ptr<LandElement[]> mLandBuffer;
    size_t mLandBufferSize;
    size_t mLandBufferMaxSize;

    std::unique_ptr<ShipTriangleElement_Point[]> mShipTrianglePointBuffer;
    size_t mShipTrianglePointBufferSize;
    size_t mShipTrianglePointBufferMaxSize;
    std::unique_ptr<ShipTriangleElement_Triangle[]> mShipTriangleTriangleBuffer;
    size_t mShipTriangleTriangleBufferSize;
    size_t mShipTriangleTriangleBufferMaxSize;

    // The Ortho matrix
    float mOrthoMatrix[4][4];

    // The current render parameters
    float mZoom;
    float mCamX;
    float mCamY;
    int mCanvasWidth;
    int mCanvasHeight;
};
