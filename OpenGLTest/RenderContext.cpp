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
    , mLandShaderLandColorParameter(0)
    , mLandShaderOrthoMatrixParameter(0)
    , mLandShaderVBO(0)
    , mShipTriangleShaderProgram(0)
    , mShipTriangleShaderOrthoMatrixParameter(0)
    , mShipTriangleShaderPointVBO(0)
    , mShipTriangleShaderTriangleVBO(0)
    , mLandBuffer()
    , mLandBufferSize(0u)
    , mLandBufferMaxSize(0u)
    , mShipTrianglePointBuffer()
    , mShipTrianglePointBufferSize(0u)
    , mShipTrianglePointBufferMaxSize(0u)
    , mShipTriangleTriangleBuffer()
    , mShipTriangleTriangleBufferSize(0u)
    , mShipTriangleTriangleBufferMaxSize(0u)
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
        attribute vec2 inputPos;
        uniform mat4 paramOrthoMatrix;
        void main()
        {
            gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);
        }
    )";

    CompileShader(landVertexShaderSource, GL_VERTEX_SHADER, mLandShaderProgram);

    char const * landFragmentShaderSource = R"(
        uniform vec3 paramLandColor;
        void main()
        {
            gl_FragColor = vec4(paramLandColor.xyz, 1.0);
        } 
    )";

    CompileShader(landFragmentShaderSource, GL_FRAGMENT_SHADER, mLandShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(mLandShaderProgram, 0, "inputPos");

    // Link
    LinkProgram(mLandShaderProgram, "Land");

    // Get uniform locations
    mLandShaderLandColorParameter = GetParameterLocation(mLandShaderProgram, "paramLandColor");
    mLandShaderOrthoMatrixParameter = GetParameterLocation(mLandShaderProgram, "paramOrthoMatrix");

    // Create VBO
    glGenBuffers(1, &mLandShaderVBO);

    glUseProgram(mLandShaderProgram);

    // Set hardcoded parameters
    glUniform3f(mLandShaderLandColorParameter, 0.5f, 0.5f, 0.5f);

    glUseProgram(0);


    //
    // Create ship triangle program
    //

    mShipTriangleShaderProgram = glCreateProgram();

    char const * shipTriangleShaderSource = R"(

        // Inputs
        attribute vec2 inputPos;
        attribute vec3 inputCol;

        // Outputs
        varying vec3 vertexCol;

        // Params
        uniform mat4 paramOrthoMatrix;

        void main()
        {
            vertexCol = inputCol;

            gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);
        }
    )";

    CompileShader(shipTriangleShaderSource, GL_VERTEX_SHADER, mShipTriangleShaderProgram);

    char const * shipTriangleFragmentShaderSource = R"(

        // Inputs from previous shader
        varying vec3 vertexCol;

        void main()
        {
            gl_FragColor = vec4(vertexCol.xyz, 1.0);
        } 
    )";

    CompileShader(shipTriangleFragmentShaderSource, GL_FRAGMENT_SHADER, mShipTriangleShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(mShipTriangleShaderProgram, 0, "inputPos");
    glBindAttribLocation(mShipTriangleShaderProgram, 1, "inputCol");

    // Link
    LinkProgram(mShipTriangleShaderProgram, "ShipTriangle");

    // Get uniform locations
    mShipTriangleShaderOrthoMatrixParameter = GetParameterLocation(mShipTriangleShaderProgram, "paramOrthoMatrix");

    // Create VBOs
    glGenBuffers(1, &mShipTriangleShaderPointVBO);
    glGenBuffers(1, &mShipTriangleShaderTriangleVBO);

    glUseProgram(mShipTriangleShaderProgram);

    // Set hardcoded parameters    
        
    glUseProgram(0);

    //
    // Initialize ortho matrix
    //

    std::memset(&(mOrthoMatrix[0][0]), 0.0f, sizeof(mOrthoMatrix));
}

RenderContext::~RenderContext()
{
    glUseProgram(0);

    if (0 != mLandShaderVBO)
    {
        glDeleteBuffers(1, &mLandShaderVBO);
    }

    if (0 != mLandShaderProgram)
    {        
        glDeleteProgram(mLandShaderProgram);
    }

    if (0 != mShipTriangleShaderPointVBO)
    {
        glDeleteBuffers(1, &mShipTriangleShaderPointVBO);
    }

    if (0 != mShipTriangleShaderTriangleVBO)
    {
        glDeleteBuffers(1, &mShipTriangleShaderTriangleVBO);
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
        // TODO: first free then alloc
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
    glUniformMatrix4fv(mLandShaderOrthoMatrixParameter, 1, GL_FALSE, &(mOrthoMatrix[0][0]));

    // Upload land buffer 
    glBindBuffer(GL_ARRAY_BUFFER, mLandShaderVBO);
    glBufferData(GL_ARRAY_BUFFER, mLandBufferSize * sizeof(LandElement), mLandBuffer.get(), GL_STATIC_DRAW);

    // Describe InputPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4 * mLandBufferSize);

    // Stop using program
    glUseProgram(0);
}

void RenderContext::RenderShipTrianglesStart(size_t points, size_t triangles)
{
    if (points != mShipTrianglePointBufferMaxSize)
    {
        // Realloc
        // TODO: first free then alloc
        mShipTrianglePointBuffer.reset(new ShipTriangleElement_Point[points]);
        mShipTrianglePointBufferMaxSize = points;
    }

    mShipTrianglePointBufferSize = 0u;

    if (triangles != mShipTriangleTriangleBufferMaxSize)
    {
        // Realloc
        // TODO: first free then alloc
        mShipTriangleTriangleBuffer.reset(new ShipTriangleElement_Triangle[triangles]);
        mShipTriangleTriangleBufferMaxSize = triangles;
    }

    mShipTriangleTriangleBufferSize = 0u;
}

void RenderContext::RenderShipTrianglesEnd()
{
    assert(mShipTrianglePointBufferSize == mShipTrianglePointBufferMaxSize);
    assert(mShipTriangleTriangleBufferSize == mShipTriangleTriangleBufferMaxSize);
    
    // Use program
    glUseProgram(mShipTriangleShaderProgram);

    // Set parameters
    glUniformMatrix4fv(mShipTriangleShaderOrthoMatrixParameter, 1, GL_FALSE, &(mOrthoMatrix[0][0]));

    // Upload ship points buffer 
    glBindBuffer(GL_ARRAY_BUFFER, mShipTriangleShaderPointVBO);
    glBufferData(GL_ARRAY_BUFFER, mShipTrianglePointBufferSize * sizeof(ShipTriangleElement_Point), mShipTrianglePointBuffer.get(), GL_STATIC_DRAW);

    // Describe InputPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (2 + 3) * sizeof(float), (void*)(0));
    glEnableVertexAttribArray(0);
    // Describe InputCol
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, (2 + 3) * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Upload ship triangles buffer 
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mShipTriangleShaderTriangleVBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mShipTriangleTriangleBufferSize * sizeof(ShipTriangleElement_Triangle), mShipTriangleTriangleBuffer.get(), GL_STATIC_DRAW);

    // Set blending function
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw
    glDrawElements(GL_TRIANGLES, 3 * mShipTriangleTriangleBufferSize, GL_UNSIGNED_INT, 0);

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
