// A Metal GPU frame capture on the device, for GL bugs no state trace explains.
//
// The GLES driver runs on Metal, so a capture of the system device records the GPU work the frame's
// GL calls became. shim_window.cpp's SwapBuffers calls k2ios_gpu_capture(1) when a file k2gputrace
// appears in the game directory (K2_GL_TRACE=file) and k2ios_gpu_capture(0) after the next frame;
// the capture lands in Documents/k2frame.gputrace, which Xcode on the Mac opens (devicectl copies
// the bundle out). Needs MTL_CAPTURE_ENABLED=1 before Metal starts (main_ios.cpp) and
// MetalCaptureEnabled in Info.plist.
//
// Metal is looked up at run time, not linked: Metal.framework exports a data symbol `_depth`, which
// would bind the engine's own `depth` (auranim.cpp) to it.
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <dlfcn.h>
#include <stdio.h>

extern "C" void k2ios_gpu_capture(int bBegin)
{
    @autoreleasepool {
        static void* s_pMetal = dlopen("/System/Library/Frameworks/Metal.framework/Metal", RTLD_NOW);
        Class captureManagerClass = NSClassFromString(@"MTLCaptureManager");
        Class descriptorClass = NSClassFromString(@"MTLCaptureDescriptor");
        typedef id (*CreateDevice)(void);
        CreateDevice pfnCreateDevice = s_pMetal ? (CreateDevice)dlsym(s_pMetal, "MTLCreateSystemDefaultDevice") : 0;
        if (!captureManagerClass || !descriptorClass || !pfnCreateDevice) {
            fprintf(stderr, "k2: gpu capture: Metal not available\n");
            return;
        }
        MTLCaptureManager* pManager = [captureManagerClass sharedCaptureManager];
        if (!bBegin) {
            if (pManager.isCapturing) {
                [pManager stopCapture];
                fprintf(stderr, "k2: gpu capture: stopped\n");
            }
            return;
        }
        if (![pManager supportsDestination:MTLCaptureDestinationGPUTraceDocument]) {
            fprintf(stderr, "k2: gpu capture: no GPU trace document support (MTL_CAPTURE_ENABLED, MetalCaptureEnabled)\n");
            return;
        }
        NSString* pDocs = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
        NSString* pPath = [pDocs stringByAppendingPathComponent:@"k2frame.gputrace"];
        [[NSFileManager defaultManager] removeItemAtPath:pPath error:nil];
        MTLCaptureDescriptor* pDescriptor = [[descriptorClass alloc] init];
        pDescriptor.captureObject = pfnCreateDevice();
        pDescriptor.destination = MTLCaptureDestinationGPUTraceDocument;
        pDescriptor.outputURL = [NSURL fileURLWithPath:pPath];
        NSError* pError = nil;
        if (![pManager startCaptureWithDescriptor:pDescriptor error:&pError])
            fprintf(stderr, "k2: gpu capture: start failed: %s\n", pError.localizedDescription.UTF8String);
        else
            fprintf(stderr, "k2: gpu capture: started -> %s\n", pPath.UTF8String);
    }
}
