#ifndef IOSLOCALIZEDSTRINGS_H
#define IOSLOCALIZEDSTRINGS_H

// A per-language string table read from a text file (IosLocalizedStrings.cpp).
class IosLocalizedStringsImpl;

class IosLocalizedStrings
{
public:
    IosLocalizedStrings(const char* pFileName, bool bLoadNow);
    ~IosLocalizedStrings();
    const char* GetString(int nIndex, int nStringsPerLanguage);

private:
    IosLocalizedStringsImpl* m_pImpl;
};

#endif // IOSLOCALIZEDSTRINGS_H
