/*******************************************************
 * Copyright (c) 2015-2019, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

// Parts of this code sourced from SnopyDogy
// https://gist.github.com/SnopyDogy/a9a22497a893ec86aa3e

#include <fg/window.h>
#include <fg/exception.h>

#include <common.hpp>
#include <err_common.hpp>

namespace fg
{

static int gWindowCounter = 0;   // Window Counter

static void windowErrorCallback(int pError, const char* pDescription)
{
    fputs(pDescription, stderr);
}

static void keyboardCallback(GLFWwindow* pWind, int pKey, int scancode, int pAction, int pMods)
{
    if (pKey == GLFW_KEY_ESCAPE && pAction == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(pWind, GL_TRUE);
    }
}

#define GLFW_THROW_ERROR(msg, err) \
    throw fg::Error("Window constructor", __LINE__, msg, err);

Window::Window(int pWidth, int pHeight, const char* pTitle, const Window* pWindow)
    : mWidth(pWidth), mHeight(pHeight), mWindow(NULL), mFont(NULL),
      mRows(0), mCols(0), mGLEWContext(NULL)
{
    CheckGL("Begin Window::Window");
    glfwSetErrorCallback(windowErrorCallback);

    if (!glfwInit()) {
        std::cerr << "ERROR: GLFW wasn't able to initalize\n";
        GLFW_THROW_ERROR("glfw initilization failed", fg::FG_ERR_GL_ERROR)
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // create glfw window
    // if pWindow is not null, then the window created in this
    // constructor call will share the context with pWindow
    GLFWwindow* temp = glfwCreateWindow(pWidth, pHeight, pTitle, NULL,
                                        (pWindow!=NULL ? pWindow->window() : NULL));
    if (temp == NULL) {
        std::cerr<<"Error: Could not Create GLFW Window!\n";
        GLFW_THROW_ERROR("glfw window creation failed", fg::FG_ERR_GL_ERROR)
    }
    mWindow = temp;

    // create glew context so that it will bind itself to windows
    if (pWindow==NULL) {
    GLEWContext* tmp = new GLEWContext();
    if (tmp == NULL) {
        std::cerr<<"Error: Could not create GLEW Context!\n";
        glfwDestroyWindow(mWindow);
        GLFW_THROW_ERROR("GLEW context creation failed", fg::FG_ERR_GL_ERROR)
    }
    mGLEWContext = tmp;
    } else {
        mGLEWContext = pWindow->glewContext();
    }

    // Set context (before glewInit())
    MakeContextCurrent(this);
    CheckGL("After MakeContextCurrent");

    //GLEW Initialization - Must be done
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        char buffer[128];
        sprintf(buffer, "GLEW init failed: Error: %s\n", glewGetErrorString(err));
        glfwDestroyWindow(mWindow);
        delete mGLEWContext;
        GLFW_THROW_ERROR(buffer, fg::FG_ERR_GL_ERROR);
    }
    err = glGetError();
    if (err!=GL_NO_ERROR && err!=GL_INVALID_ENUM) {
        // ignore this error, as GLEW is known to
        // have this issue with 3.2+ core profiles
        // they are yet to fix this in GLEW
        ForceCheckGL("GLEW initilization failed");
        GLFW_THROW_ERROR("GLEW initilization failed", fg::FG_ERR_GL_ERROR)
    }

    glfwSetKeyCallback(mWindow, keyboardCallback);
#ifdef WINDOWS_OS
    mCxt = glfwGetWGLContext(mWindow);
    mDsp = GetDC(glfwGetWin32Window(mWindow));
#endif
#ifdef LINUX_OS
    mCxt = glfwGetGLXContext(mWindow);
    mDsp = glfwGetX11Display();
#endif
    mID = gWindowCounter++; //set ID and Increment Counter!
    CheckGL("End Window::Window");
}

Window::~Window()
{
    MakeContextCurrent(this);
    if (mWindow!=NULL) glfwDestroyWindow(mWindow);
    mCells.clear();
}

void Window::draw(const Image& pImage)
{
    float pos[2] = {0.0, 0.0};
    const float color[4] = {1.0, 1.0, 1.0, 1.0};
    CheckGL("Begin drawImage");
    MakeContextCurrent(this);

    int wind_width, wind_height;
    glfwGetWindowSize(window(), &wind_width, &wind_height);
    glViewport(0, 0, wind_width, wind_height);
    pos[0] = wind_width-128.0f;

    // clear color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.2, 0.2, 0.2, 1.0);

    pImage.render();
    // draw Forge tag
    mFont->setOthro2D(wind_width, wind_height);
    pos[1] = 30.0f;
    mFont->render(pos, color, "powered by", 12);
    pos[1] -= 20.0f;
    mFont->render(pos, color, "Forge", 16);

    glfwSwapBuffers(window());
    glfwPollEvents();
    CheckGL("End drawImage");
}

void Window::draw(const Plot& pHandle)
{
    CheckGL("Begin draw(Plot)");
    MakeContextCurrent(this);

    int wind_width, wind_height;
    glfwGetWindowSize(window(), &wind_width, &wind_height);
    glViewport(0, 0, wind_width, wind_height);

    glClearColor(1.0, 1.0, 1.0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    pHandle.render(0, 0, wind_width, wind_height);

    glfwSwapBuffers(window());
    glfwPollEvents();
    CheckGL("End draw(Plot)");
}

void Window::draw(const Histogram& pHist)
{
    CheckGL("Begin draw(Histogram)");
    MakeContextCurrent(this);

    int wind_width, wind_height;
    glfwGetWindowSize(window(), &wind_width, &wind_height);
    glViewport(0, 0, wind_width, wind_height);
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    pHist.render(0, 0, wind_width, wind_height);

    glfwSwapBuffers(window());
    glfwPollEvents();
    CheckGL("End draw(Histogram)");
}

void Window::grid(int pRows, int pCols)
{
    mRows = pRows;
    mCols = pCols;
    mCells.clear();
}

void Window::bind(int pColId, int pRowId, const void* pRenderablePtr, Renderable pType)
{
    int idx = (pColId-1) + (pRowId-1)*mCols;
    auto cell = std::make_pair(pRenderablePtr, pType);
    mCells.emplace(idx, cell);
}

void Window::show()
{
    CheckGL("Begin show");
    MakeContextCurrent(this);

    int wind_width, wind_height;
    glfwGetWindowSize(window(), &wind_width, &wind_height);
    glViewport(0, 0, wind_width, wind_height);

    // calculate cell width and height
    uint wid_step = wind_width/ mCols;
    uint hei_step = wind_height/ mRows;
    int numRenderables = mCells.size();

    // clear color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.2, 0.2, 0.2, 1.0);

    for (int r=0; r<mRows; ++r) {
        for (int c=0; c<mCols; ++c) {

            int idx = mCols-1-c + r*mCols;
            if (idx<numRenderables) {

                auto item = mCells.find(idx);
                if (item == mCells.end()) {
                    printf("Cell id not found in the grid, skipping it\n");
                    continue;
                }

                const void* pointer = item->second.first;
                Renderable renderType = item->second.second;

                int x_off = r * wid_step;
                int y_off = c * hei_step;

                // set viewport to render sub image
                glViewport(x_off, y_off, wid_step, hei_step);

                /* FIXME as of now, only fg::Image::render doesn't ask
                 * for any input parameters */
                switch(renderType) {
                    case FG_IMAGE:
                        ((const fg::Image*)pointer)->render();
                        break;
                    case FG_PLOT:
                        ((const fg::Plot*)pointer)->render(x_off, y_off, wid_step, hei_step);
                        break;
                    case FG_HIST:
                        ((const fg::Histogram*)pointer)->render(x_off, y_off, wid_step, hei_step);
                        break;
                }
            }
        }
    }

    glfwSwapBuffers(window());
    glfwPollEvents();
    CheckGL("End show");
}

void makeCurrent(Window* pWindow)
{
    MakeContextCurrent(pWindow);
}

}