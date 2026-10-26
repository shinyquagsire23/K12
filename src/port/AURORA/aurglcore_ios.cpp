#include "aurglcore_ios.h"
#include "port/aslglapi.h"


extern int g_nScreenWidth;
extern int g_nScreenHeight;

RenderModificationQuadSetup::RenderModificationQuadSetup()
{
    GLRender::Disable(Aurora::kCapAlphaTest);
    GLRender::Disable(Aurora::kCapDepthTest);
    GLRender::Disable(Aurora::kCapLighting);
    ASLgl::glDepthMask(0);
    ASLgl::glMatrixMode(GL_PROJECTION);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    ASLgl::glMatrixMode(GL_MODELVIEW);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
}

RenderModificationQuadSetup::~RenderModificationQuadSetup()
{
    // inlined GLRender::SetBlendFunc(): blendTranslationTable is not visible here (the call loads it, LENGTH +4)
    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapDepthTest);
    GLRender::Enable(Aurora::kCapLighting);
    ASLgl::glDepthMask(1);
    ASLgl::glMatrixMode(GL_MODELVIEW);
    ASLgl::glPopMatrix();
    ASLgl::glMatrixMode(GL_PROJECTION);
    ASLgl::glPopMatrix();
    ASLgl::glMatrixMode(GL_MODELVIEW);
}

extern "C" const unsigned char* glGetString(unsigned int eName);
#include "port/crtapi.h"
#include "port/libcstring.h"
#include "port/libcstdlib.h"

// Scan the GL_EXTENSIONS string for pszExtension. The match path returns without freeing the
// duplicate; the fall-through frees it.
bool glExtensionSupported(const char* pszExtension)
{
    char* pszList = _strdup(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));
    char* pszToken = strtok(pszList, " ");
    while (pszToken != 0)
    {
        if (strcmp(pszExtension, pszToken) == 0)
            return true;
        pszToken = strtok(0, " ");
    }
    free(pszList);
    return false;
}
