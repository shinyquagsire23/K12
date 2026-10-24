// CScriptCompilerInternal's error-reporting half, compiled as its own translation unit.

#include "k2stdint.h"
#include "nwscriptinternal.h"

#include "port/libcstring.h"

// Assembles the "file(line): message" line and returns the error code it was passed. A release build
// prints nothing: the assembled string is discarded, though the CStr() call ahead of the destructor
// stays.
//
// A '!'-prefixed file name is a synthetic chunk, "!Chunk" or "!Conditional", whose first character
// is stripped for display; a real file gets the ".nss" suffix the script compiler looks it up under.
// Each shape then splits on the line number: at least 1 gives the parenthesised form, otherwise the
// bare one.
int CScriptCompilerInternal::OutputError(int nType, const CExoString& sFile, int nLine, const CExoString& sMsg)
{
    CExoString sMessage;
    if (sFile.Left(1) == "!")
    {
        if (nLine >= 1)
        {
#if KOTOR1  // K1: CExoString::GetLength is out of line
            int nLength = sFile.GetLength();
#else
            int nLength = sFile.m_sString ? static_cast<int>(strlen(sFile.m_sString)) : 0;
#endif
            sMessage.Format(const_cast<char*>("%s(%d): %s\n"), sFile.Right(nLength - 1).CStr(), nLine, sMsg.CStr());
        }
        else
        {
#if KOTOR1  // K1: CExoString::GetLength is out of line
            int nLength = sFile.GetLength();
#else
            int nLength = sFile.m_sString ? static_cast<int>(strlen(sFile.m_sString)) : 0;
#endif
            sMessage.Format(const_cast<char*>("%s: %s\n"), sFile.Right(nLength - 1).CStr(), sMsg.CStr());
        }
    }
    else
    {
        if (nLine >= 1)
            sMessage.Format(const_cast<char*>("%s.nss(%d): %s\n"), sFile.CStr(), nLine, sMsg.CStr());
        else
            sMessage.Format(const_cast<char*>("%s.nss: %s\n"), sFile.CStr(), sMsg.CStr());
    }

    sMessage.CStr();
    return nType;
}
