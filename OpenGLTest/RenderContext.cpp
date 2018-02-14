/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RenderContext.h"

#include "GameException.h"

#include <cstring>

RenderContext::RenderContext()
    : mLandShaderProgram(0)
    , mLandShaderAmbientLightStrengthParameter(0)
    , mLandShaderAmbientLightColorParameter(0)
    , mLandShaderLandColorParameter(0)
    , mLandShaderOrthoMatrixParameter(0)
    , mShipTriangleShaderProgram(0)
    , mShipTriangleShaderAmbientLightStrengthParameter(0)
    , mShipTriangleShaderAmbientLightColorParameter(0)
    , mShipTriangleShaderOrthoMatrixParameter(0)
    , mLandBuffer()
    , mLandBufferSize(0u)
    , mLandBufferMaxSize(0u)
    , mShipTriangleBuffer()
    , mShipTriangleBufferSize(0u)
    , mShipTriangleBufferMaxSize(0u)
    , mZoom(70.0f)
    , mCamX(0.0f)
    , mCamY(0.0f)
    , mCanvasWidth(100)
    , mCanvasHeight(100)
{
    //
    // Check OpenGL version
    //

    int versionMaj = 0;
    int versionMin = 0;
    char const * glVersion = (char*)glGetString(GL_VERSION);
    if (nullptr == glVersion)
    {
        throw GameException("OpenGL completely not supported");
    }

    sscanf(glVersion, "%d.%d", &versionMaj, &versionMin);
    if (versionMaj < 2)
    {
        throw GameException("This game requires at least OpenGL 2.0 support; the version currently supported by your computer is " + std::string(glVersion));
    }



    //
    // Create land program
    //

    mLandShaderProgram = glCreateProgram();

    char const * landVertexShaderSource = R"(
        attribute vec2 InputPos;
        uniform mat4 ParamOrthoMatrix;
        void main()
        {
            gl_Position = ParamOrthoMatrix * vec4(InputPos.xy, -1.0, 1.0);
        }
    )";

    CompileShader(landVertexShaderSource, GL_VERTEX_SHADER, mLandShaderProgram);

    char const * landFragmentShaderSource = R"(
        uniform vec3 ParamLandColor;
        uniform float ParamAmbientLightStrength;
        uniform vec3 ParamAmbientLightColor;
        void main()
        {
            vec3 ambientLight = ParamAmbientLightStrength * ParamAmbientLightColor;
            gl_FragColor = vec4(ambientLight * ParamLandColor, 1.0);
        } 
    )";

    CompileShader(landFragmentShaderSource, GL_FRAGMENT_SHADER, mLandShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(mLandShaderProgram, 0, "InputPos");

    // Link
    LinkProgram(mLandShaderProgram, "Land");

    // Get uniform locations
    mLandShaderLandColorParameter = GetParameterLocation(mLandShaderProgram, "ParamLandColor");
    mLandShaderAmbientLightStrengthParameter = GetParameterLocation(mLandShaderProgram, "ParamAmbientLightStrength");
    mLandShaderAmbientLightColorParameter = GetParameterLocation(mLandShaderProgram, "ParamAmbientLightColor");
    mLandShaderOrthoMatrixParameter = GetParameterLocation(mLandShaderProgram, "ParamOrthoMatrix");



    //
    // Create ship triangle program
    //

    mShipTriangleShaderProgram = glCreateProgram();

    char const * shipTriangleShaderSource = R"(
        attribute vec2 InputPos;
        attribute vec3 InputCol;
        varying vec3 InternalCol;
        uniform mat4 ParamOrthoMatrix;
        void main()
        {
            InternalCol = InputCol;
            gl_Position = ParamOrthoMatrix * vec4(InputPos.xy, -1.0, 1.0);
        }
    )";

    CompileShader(shipTriangleShaderSource, GL_VERTEX_SHADER, mShipTriangleShaderProgram);

    char const * shipTriangleFragmentShaderSource = R"(
        varying vec3 InternalCol;
        uniform float ParamAmbientLightStrength;
        uniform vec3 ParamAmbientLightColor;
        void main()
        {
            vec3 ambientLight = ParamAmbientLightStrength * ParamAmbientLightColor;
            gl_FragColor = vec4(ambientLight * InternalCol, 1.0);
        } 
    )";

    CompileShader(shipTriangleFragmentShaderSource, GL_FRAGMENT_SHADER, mShipTriangleShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(mShipTriangleShaderProgram, 0, "InputPos");
    glBindAttribLocation(mShipTriangleShaderProgram, 1, "InputCol");

    // Link
    LinkProgram(mShipTriangleShaderProgram, "ShipTriangle");

    // Get uniform locations
    mShipTriangleShaderAmbientLightStrengthParameter = GetParameterLocation(mShipTriangleShaderProgram, "ParamAmbientLightStrength");
    mShipTriangleShaderAmbientLightColorParameter = GetParameterLocation(mShipTriangleShaderProgram, "ParamAmbientLightColor");
    mShipTriangleShaderOrthoMatrixParameter = GetParameterLocation(mShipTriangleShaderProgram, "ParamOrthoMatrix");


    //char const * vertexShaderSource = R"(
    //    attribute vec3 InputPos;
    //    attribute vec3 InputCol;
    //    varying vec3 InternalColor;
    //    uniform mat4 ParamOrthoMatrix;
    //    void main()
    //    {
    //        InternalColor = InputCol;
    //        gl_Position = ParamOrthoMatrix * vec4(InputPos.xyz, 1.0);
    //    }
    //)";

    //char const * fragmentShaderSource = R"(
    //    varying vec3 InternalColor;
    //    uniform float ParamAmbientLightStrength;
    //    uniform vec3 ParamAmbientLightColor;
    //    void main()
    //    {
    //        vec3 ambientLight = ParamAmbientLightStrength * ParamAmbientLightColor;
    //        gl_FragColor = vec4(ambientLight * InternalColor, 1.0);
    //    } 
    //)";


    //
    // Initialize ortho matrix
    //

    std::memset(&(mOrthoMatrix[0][0]), 0.0f, sizeof(mOrthoMatrix));
}

RenderContext::~RenderContext()
{
    glUseProgram(0);

    if (0 != mLandShaderProgram)
    {        
        glDeleteProgram(mLandShaderProgram);
    }

    if (0 != mShipTriangleShaderProgram)
    {
        glDeleteProgram(mShipTriangleShaderProgram);
    }
}

void RenderContext::SetCanvasSize(int width, int height)
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

void RenderContext::SetZoom(float zoom)
{
    mZoom = zoom;

    CalculateOrthoMatrix(
        mZoom,
        mCamX,
        mCamY,
        mCanvasWidth,
        mCanvasHeight);
}

void RenderContext::SetCameraPosition(int x, int y)
{
    mCamX = x;
    mCamY = y;

    CalculateOrthoMatrix(
        mZoom,
        mCamX,
        mCamY,
        mCanvasWidth,
        mCanvasHeight);
}

//////////////////////////////////////////////////////////////////////////////////

void RenderContext::RenderStart()
{
    //
    // Clear canvas 
    //

    glClearColor(0.529f, 0.808f, 0.980f, 1.0f); // (cornflower blue)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RenderContext::RenderLandStart(size_t elements)
{
    if (elements != mLandBufferMaxSize)
    {
        // Realloc
        mLandBuffer.reset(new LandElement[elements]);
        mLandBufferMaxSize = elements;
    }

    mLandBufferSize = 0u;    
}

void RenderContext::RenderLandEnd()
{
    assert(mLandBufferSize == mLandBufferMaxSize);

    // Use program
    glUseProgram(mLandShaderProgram);

    // Set parameters
    glUniform3f(mLandShaderAmbientLightColorParameter, 1.0f, 1.0f, 1.0f);
    glUniform1f(mLandShaderAmbientLightStrengthParameter, 1.0f);
    glUniform3f(mLandShaderLandColorParameter, 0.5f, 0.5f, 0.5f);
    glUniformMatrix4fv(mLandShaderOrthoMatrixParameter, 1, GL_FALSE, &(mOrthoMatrix[0][0]));

    // Upload land buffer 
    unsigned int myVBO;
    glGenBuffers(1, &myVBO);
    glBindBuffer(GL_ARRAY_BUFFER, myVBO);
    glBufferData(GL_ARRAY_BUFFER, mLandBufferSize * sizeof(LandElement), mLandBuffer.get(), GL_STATIC_DRAW);

    // Describe InputPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4 * mLandBufferSize);

    // Stop using program
    glUseProgram(0);
}

void RenderContext::RenderShipTrianglesStart(size_t elements)
{
    if (elements != mShipTriangleBufferMaxSize)
    {
        // Realloc
        mShipTriangleBuffer.reset(new ShipTriangleElement[elements]);
        mShipTriangleBufferMaxSize = elements;
    }

    mShipTriangleBufferSize = 0u;
}

void RenderContext::RenderShipTrianglesEnd()
{
    assert(mShipTriangleBufferSize == mShipTriangleBufferMaxSize);
    
    // Use program
    glUseProgram(mShipTriangleShaderProgram);

    // Set parameters
    glUniform3f(mShipTriangleShaderAmbientLightColorParameter, 1.0f, 1.0f, 1.0f);
    glUniform1f(mShipTriangleShaderAmbientLightStrengthParameter, 1.0f);
    glUniformMatrix4fv(mShipTriangleShaderOrthoMatrixParameter, 1, GL_FALSE, &(mOrthoMatrix[0][0]));

    // Upload ship triangles buffer 
    unsigned int myVBO;
    glGenBuffers(1, &myVBO);
    glBindBuffer(GL_ARRAY_BUFFER, myVBO);
    glBufferData(GL_ARRAY_BUFFER, mShipTriangleBufferSize * sizeof(ShipTriangleElement), mShipTriangleBuffer.get(), GL_STATIC_DRAW);

    // Describe InputPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (2 + 3) * sizeof(float), (void*)(0));
    glEnableVertexAttribArray(0);
    // Describe InputCol
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, (2 + 3) * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);


    //TODOTEST
    // Smooth lines and points
    glEnable(GL_LINE_SMOOTH);
    // Set blending function
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Set primitives' thickness
    glPointSize(0.15f * mCanvasHeight / mZoom);
    glLineWidth(0.1f * mCanvasHeight / mZoom);


    // Draw
    glDrawArrays(GL_TRIANGLES, 0, 3 * mShipTriangleBufferSize);

    // Stop using program
    glUseProgram(0);
}

void RenderContext::RenderEnd()
{
    glFlush();
}

////////////////////////////////////////////////////////////////////////////////////

void RenderContext::CompileShader(
    char const * shaderSource,
    GLenum shaderType,
    GLuint shaderProgram) 
{
    // Compile
    unsigned int shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSource, NULL);
    glCompileShader(shader);

    // Check
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
        throw GameException("ERROR Compiling vertex shader: " + std::string(infoLog));
    }

    // Attach to program
    glAttachShader(shaderProgram, shader);

    // Delete shader
    glDeleteShader(shader);
}

void RenderContext::LinkProgram(
    GLuint shaderProgram,
    std::string const & programName)
{
    glLinkProgram(shaderProgram);

    // Check
    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetShaderInfoLog(shaderProgram, sizeof(infoLog), NULL, infoLog);
        throw GameException("ERROR linking " + programName + " shader program: " + std::string(infoLog));
    }
}

GLint RenderContext::GetParameterLocation(
    GLuint shaderProgram,
    std::string const & parameterName)
{
    GLint parameterLocation = glGetUniformLocation(shaderProgram, parameterName.c_str());
    if (parameterLocation == -1)
    { 
        throw GameException("ERROR retrieving location of parameter \"" + parameterName + "\"");
    }

    return parameterLocation;
}

void RenderContext::CalculateOrthoMatrix(
    float zoom,
    float camX,
    float camY,
    int canvasWidth,
    int canvasHeight)
{
    float halfHeight = zoom;
    float halfWidth = static_cast<float>(canvasWidth) / static_cast<float>(canvasHeight) * halfHeight;
    static constexpr float zFar = 1000.0f;
    static constexpr float zNear = 1.0f;

    mOrthoMatrix[0][0] = 1.0f / (halfWidth);
    mOrthoMatrix[1][1] = 1.0f / (halfHeight);
    mOrthoMatrix[2][2] = -2.0f / (zFar - zNear);
    mOrthoMatrix[3][0] = camX; // TBD: probably it has to be minus
    mOrthoMatrix[3][1] = camY; // TBD: probably it has to be minus
    mOrthoMatrix[3][2] = -(zFar + zNear) / (zFar - zNear);
    mOrthoMatrix[3][3] = 1.0f;
}
