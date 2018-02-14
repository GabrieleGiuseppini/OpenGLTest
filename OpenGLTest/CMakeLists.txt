
#
# OpenGLTest application
#

set  (OPEN_GL_TEST_SOURCES
	GameException.h
	MainApp.cpp
	MainFrame.cpp
	MainFrame.h
	OpenGLTest.h
	RenderContext.cpp
	RenderContext.h
	Vectors.cpp
	Vectors.h)

source_group(" " FILES ${OPEN_GL_TEST_SOURCES})

add_executable (OpenGLTest ${OPEN_GL_TEST_SOURCES})

target_include_directories(OpenGLTest PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_compile_definitions(OpenGLTest PRIVATE "${wxWidgets_DEFINITIONS}")
target_link_libraries (OpenGLTest
	GladLib
	${OPENGL_LIBRARIES}
	${wxWidgets_LIBRARIES}
	${ADDITIONAL_LIBRARIES})


if (MSVC)
	set_target_properties(OpenGLTest PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
else (MSVC)
endif (MSVC)


#
# Set VS properties
#

if (MSVC)

	set (VS_STARTUP_PROJECT OpenGLTest)
	
	set_target_properties(
		OpenGLTest
		PROPERTIES
			# Set debugger working directory to binary output directory
			VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"

			# Set output directory to binary output directory - VS will add the configuration type
			RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	)

endif (MSVC)

