#ifndef VERTEXPROGRAM_H
#define VERTEXPROGRAM_H

class Part;

// AURORA's ARB/NV vertex-program wrapper: the program source, the GL program name and the upload
// state for one program.
class VertexProgram {
public:
    VertexProgram(char* program, unsigned int* declaration);
    void Enable(Part* part, bool flag);
    // Derives the light budget from the program text.
    void FindNumberOfAllowedLights();
    ~VertexProgram();

    void*         m_pReserved1;
    char*         m_pProgramText;
    char          m_szProgram[8192];
    unsigned int* m_pDeclaration;
    unsigned int  m_uProgramARB;
    int           m_nAllowedLights;
    int           m_nInstalledStamp;
    int           m_bProgramLoaded;
};

// Every live VertexProgram, so a GL context loss can reload them all.
#include "List.h"
class VertexProgramManager {
public:
    static void AddVertexProgram(VertexProgram* vp);
    static void RemoveVertexProgram(VertexProgram* vp);
    static void ResetVertexPrograms();

    static List<VertexProgram*> cm_listExistentVertexPrograms;
};

#endif // VERTEXPROGRAM_H
