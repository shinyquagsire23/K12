#ifndef K2VIEWERAPI_H
#define K2VIEWERAPI_H

// The model viewer (platforms/viewer/, the k1view / k2view executables) links the whole engine and
// runs the game's own start-up -- the resource manager, Aurora, the client's services and options --
// then takes over: Start in place of the intro movies and the main menu, Frame in place of the
// client's and the server's main loops. The engine's frame around it (the clear, the console strings,
// the swap) is unchanged. The games leave g_pK2HostViewer null.

class K2HostViewer {
public:
    virtual ~K2HostViewer() {}
    virtual void Start() = 0;
    virtual int  Frame() = 0;   // nonzero ends the program
};

extern K2HostViewer* g_pK2HostViewer;

#endif // K2VIEWERAPI_H
