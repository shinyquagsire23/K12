#ifndef CRESGFFFIELDIDHASH_H
#define CRESGFFFIELDIDHASH_H

// CRC32 table and lookup used to intern GFF field names; static-utility only, never instantiated.
class CResGFFFieldIDHash {
public:
    static void Initialize();
    static unsigned int Hash(const char* pszName, unsigned int nLength);

private:
    static unsigned int m_nCRC32[256];
};

#endif // CRESGFFFIELDIDHASH_H
