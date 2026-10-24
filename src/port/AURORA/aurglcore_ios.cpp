#include "aurglcore_ios.h"
#include "port/aslglapi.h"


extern unsigned int enableTranslationTable[];
extern int g_nScreenWidth;
extern int g_nScreenHeight;

// Indices into enableTranslationTable.
enum { kCapAlphaTest = 1, kCapLighting = 5, kCapDepthTest = 11 };

RenderModificationQuadSetup::RenderModificationQuadSetup()
{
    ASLgl::glDisable(enableTranslationTable[kCapAlphaTest]);
    ASLgl::glDisable(enableTranslationTable[kCapDepthTest]);
    ASLgl::glDisable(enableTranslationTable[kCapLighting]);
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
    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ASLgl::glEnable(enableTranslationTable[kCapAlphaTest]);
    ASLgl::glEnable(enableTranslationTable[kCapDepthTest]);
    ASLgl::glEnable(enableTranslationTable[kCapLighting]);
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
