/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "OpenGLTest.h"
#include "Vectors.h"

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

    float GetZoom() const
    {
        return mZoom;
    }

    void SetZoom(float zoom)
    {
        mZoom = zoom;

        CalculateOrthoMatrix(
            mZoom,
            mCamX,
            mCamY,
            mCanvasWidth,
            mCanvasHeight);
    }
    
    vec2f GetCameraPosition() const
    {
        return vec2f(mCamX, mCamY);
    }

    void SetCameraPosition(vec2f const & pos)
    {
        mCamX = pos.x;
        mCamY = pos.y;

        CalculateOrthoMatrix(
            mZoom,
            mCamX,
            mCamY,
            mCanvasWidth,
            mCanvasHeight);
    }

    int GetCanvasSizeWidth() const
    {
        return mCanvasWidth;
    }

    int GetCanvasSizeHeight() const
    {
        return mCanvasHeight;
    }

    void SetCanvasSize(int width, int height)
    {
        mCanvasWidth = width;
        mCanvasHeight = height;

        glViewport(0, 0, mCanvasWidth, mCanvasHeight);

        CalculateOrthoMatrix(
            mZoom,
            mCamX,
            mCamY,
            mCanvasWidth,
            mCanvasHeight);
    }

    float GetAmbientLightIntensity() const
    {
        return mAmbientLightIntensity;
    }

    void SetAmbientLightIntensity(float intensity)
    {
        mAmbientLightIntensity = intensity;
    }

public:

    void RenderStart();


    //
    // Land
    //

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


    //
    // Water
    //

    void RenderWaterStart(size_t elements);

    inline void RenderWater(
        float left,
        float right,
        float leftTop,
        float rightTop,
        float bottom)
    {
        assert(mWaterBufferSize + 1u <= mWaterBufferMaxSize);

        WaterElement * waterElement = &(mWaterBuffer[mWaterBufferSize]);

        waterElement->x1 = left;
        waterElement->y1 = leftTop;
        waterElement->x2 = right;
        waterElement->y2 = rightTop;
        waterElement->x3 = left;
        waterElement->y3 = bottom;
        waterElement->x4 = right;
        waterElement->y4 = bottom;

        ++mWaterBufferSize;
    }

    void RenderWaterEnd();


    //
    // Ship Points
    //

    void UploadShipPointStart(size_t points);

    inline void UploadShipPoint(
        float x,
        float y,
        float r,
        float g,
        float b)
    {
        assert(mShipPointBufferSize + 1u <= mShipPointBufferMaxSize);

        ShipPointElement * shipPointElement = &(mShipPointBuffer[mShipPointBufferSize]);

        shipPointElement->x = x;
        shipPointElement->y = y;
        shipPointElement->r = r;
        shipPointElement->g = g;
        shipPointElement->b = b;

        ++mShipPointBufferSize;
    }

    void UploadShipPointEnd();

    void RenderShipPoints();


    //
    // Springs
    //

    void RenderSpringsStart(size_t springs);

    inline void RenderSpring(
        int shipPointIndex1,
        int shipPointIndex2)
    {
        assert(mSpringBufferSize + 1u <= mSpringBufferMaxSize);

        SpringElement * springElement = &(mSpringBuffer[mSpringBufferSize]);

        springElement->shipPointIndex1 = shipPointIndex1;
        springElement->shipPointIndex2 = shipPointIndex2;

        ++mSpringBufferSize;
    }

    void RenderSpringsEnd();


    void RenderStressedSpringsStart(size_t maxSprings);

    inline void RenderStressedSpring(
        int shipPointIndex1,
        int shipPointIndex2)
    {
        assert(mStressedSpringBufferSize + 1u <= mStressedSpringBufferMaxSize);

        SpringElement * springElement = &(mStressedSpringBuffer[mStressedSpringBufferSize]);

        springElement->shipPointIndex1 = shipPointIndex1;
        springElement->shipPointIndex2 = shipPointIndex2;

        ++mStressedSpringBufferSize;
    }

    void RenderStressedSpringsEnd();


    //
    // Ship triangles
    //

    void RenderShipTrianglesStart(size_t triangles);

    inline void RenderShipTriangle(
        int shipPointIndex1,
        int shipPointIndex2,
        int shipPointIndex3)
    {
        assert(mShipTriangleBufferSize + 1u <= mShipTriangleBufferMaxSize);

        ShipTriangleElement * shipTriangleElement = &(mShipTriangleBuffer[mShipTriangleBufferSize]);

        shipTriangleElement->shipPointIndex1 = shipPointIndex1;
        shipTriangleElement->shipPointIndex2 = shipPointIndex2;
        shipTriangleElement->shipPointIndex3 = shipPointIndex3;

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
    // Land
    //

    GLuint mLandShaderProgram;
    GLint mLandShaderLandColorParameter;
    GLint mLandShaderAmbientLightIntensityParameter;
    GLint mLandShaderOrthoMatrixParameter;

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
#pragma pack(pop)

    std::unique_ptr<LandElement[]> mLandBuffer;
    size_t mLandBufferSize;
    size_t mLandBufferMaxSize;

    GLuint mLandVBO;


    //
    // Water
    //

    GLuint mWaterShaderProgram;
    GLint mWaterShaderWaterColorParameter;
    GLint mWaterShaderAmbientLightIntensityParameter;
    GLint mWaterShaderOrthoMatrixParameter;

#pragma pack(push)
    struct WaterElement
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
#pragma pack(pop)

    std::unique_ptr<WaterElement[]> mWaterBuffer;
    size_t mWaterBufferSize;
    size_t mWaterBufferMaxSize;

    GLuint mWaterVBO;


    //
    // Ship points
    //

    GLuint mShipPointShaderProgram;
    GLint mShipPointShaderOrthoMatrixParameter;

#pragma pack(push)
    struct ShipPointElement
    {
        float x;
        float y;
        float r;
        float g;
        float b;
    };
#pragma pack(pop)

    std::unique_ptr<ShipPointElement[]> mShipPointBuffer;
    size_t mShipPointBufferSize;
    size_t mShipPointBufferMaxSize;

    GLuint mShipPointVBO;


    //
    // Springs
    //

    GLuint mSpringShaderProgram;
    GLint mSpringShaderOrthoMatrixParameter;

#pragma pack(push)
    struct SpringElement
    {
        int shipPointIndex1;
        int shipPointIndex2;
    };
#pragma pack(pop)

    std::unique_ptr<SpringElement[]> mSpringBuffer;
    size_t mSpringBufferSize;
    size_t mSpringBufferMaxSize;

    GLuint mSpringVBO;


    //
    // Stressed springs
    //

    GLuint mStressedSpringShaderProgram;
    GLint mStressedSpringShaderAmbientLightIntensityParameter;
    GLint mStressedSpringShaderOrthoMatrixParameter;

    std::unique_ptr<SpringElement[]> mStressedSpringBuffer;
    size_t mStressedSpringBufferSize;
    size_t mStressedSpringBufferMaxSize;

    GLuint mStressedSpringVBO;


    //
    // Ship triangles
    //

    GLuint mShipTriangleShaderProgram;
    GLint mShipTriangleShaderOrthoMatrixParameter;

#pragma pack(push)
    struct ShipTriangleElement
    {
        int shipPointIndex1;
        int shipPointIndex2;
        int shipPointIndex3;
    };
#pragma pack(pop)

    std::unique_ptr<ShipTriangleElement[]> mShipTriangleBuffer;
    size_t mShipTriangleBufferSize;
    size_t mShipTriangleBufferMaxSize;

    GLuint mShipTriangleVBO;

private:

    // The Ortho matrix
    float mOrthoMatrix[4][4];

    // The current render parameters
    float mZoom;
    float mCamX;
    float mCamY;
    int mCanvasWidth;
    int mCanvasHeight;
    float mAmbientLightIntensity;
};
