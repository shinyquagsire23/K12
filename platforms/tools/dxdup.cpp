// dxdup.cpp: what the display shows, frame by frame (Windows; DXGI desktop duplication of output 0).
// It is the flicker probe for the "Windows/NVIDIA: one-refresh flashes" bug. `shot:` reads the back
// buffer before the swap and cannot see a frame the presentation path mangles; this can.
//   cl /O2 /EHsc dxdup.cpp            (an x64 developer prompt)
//   dxdup <outdir> <seconds> [saveEvery] [fullSpikes]
// Must run in the interactive session (schtasks /it, or a console on the desktop), not over sshd.
// Writes <outdir>\frames.csv (ms, accumulated frames, mean R G B, a 16x9 grid of mean luma) for every
// frame the desktop presents, a 1/6-scale PPM every saveEvery-th frame, and for each one-frame spike
// (frame n-1 differs from n-2 and from n by > 0.06 grid luma while n-2 and n agree) the three
// frames, the first <fullSpikes> of them also at full size. Prints one `spike at` line per spike.
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <stdio.h>
#include <vector>
#include <math.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "user32.lib")
static void SavePPM(const char* dir, int idx, const char* tag, const std::vector<unsigned char>& thumb, int w, int h) {
    char path[512]; sprintf_s(path, "%s\\f%05d%s.ppm", dir, idx, tag);
    FILE* f; if (fopen_s(&f, path, "wb")) return;
    fprintf(f, "P6\n%d %d\n255\n", w, h); fwrite(thumb.data(), 1, thumb.size(), f); fclose(f);
}
static std::vector<unsigned char> full0, full1, full2;
static void SaveFull(const char* dir, int idx, const std::vector<unsigned char>& bgra, int w, int h) {
    char path[512]; sprintf_s(path, "%s\\full%05d.ppm", dir, idx);
    FILE* f; if (fopen_s(&f, path, "wb")) return;
    fprintf(f, "P6\n%d %d\n255\n", w, h); std::vector<unsigned char> row(w * 3);
    for (int y = 0; y < h; y++) { for (int x = 0; x < w; x++) { const unsigned char* p = &bgra[((size_t)y * w + x) * 4]; row[x*3] = p[2]; row[x*3+1] = p[1]; row[x*3+2] = p[0]; } fwrite(row.data(), 1, row.size(), f); }
    fclose(f);
}
int main(int argc, char** argv) {
    if (argc < 3) { printf("usage\n"); return 1; }
    SetProcessDPIAware();
    const char* dir = argv[1]; double secs = atof(argv[2]); int every = argc > 3 ? atoi(argv[3]) : 0; int nFull = argc > 4 ? atoi(argv[4]) : 0;
    CreateDirectoryA(dir, 0);
    ID3D11Device* dev; ID3D11DeviceContext* ctx; D3D_FEATURE_LEVEL fl;
    if (FAILED(D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, 0, 0, 0, D3D11_SDK_VERSION, &dev, &fl, &ctx))) { printf("dev\n"); return 1; }
    IDXGIDevice* dd; dev->QueryInterface(__uuidof(IDXGIDevice), (void**)&dd);
    IDXGIAdapter* ad; dd->GetAdapter(&ad);
    IDXGIOutput* out; if (FAILED(ad->EnumOutputs(0, &out))) { printf("out\n"); return 1; }
    IDXGIOutput1* out1; out->QueryInterface(__uuidof(IDXGIOutput1), (void**)&out1);
    IDXGIOutputDuplication* dup; HRESULT hr = out1->DuplicateOutput(dev, &dup);
    if (FAILED(hr)) { printf("dup %08lx\n", hr); return 1; }
    DXGI_OUTDUPL_DESC od; dup->GetDesc(&od);
    int W = od.ModeDesc.Width, H = od.ModeDesc.Height;
    printf("mode %dx%d fmt %d rot %d\n", W, H, od.ModeDesc.Format, od.Rotation);
    D3D11_TEXTURE2D_DESC td = {}; td.Width = W; td.Height = H; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = od.ModeDesc.Format; td.SampleDesc.Count = 1; td.Usage = D3D11_USAGE_STAGING; td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D* st; dev->CreateTexture2D(&td, 0, &st);
    char csv[512]; sprintf_s(csv, "%s\\frames.csv", dir); FILE* fc; fopen_s(&fc, csv, "w");
    LARGE_INTEGER fq, t0, tn; QueryPerformanceFrequency(&fq); QueryPerformanceCounter(&t0);
    const int GX = 16, GY = 9; int sw = W / 6, sh = H / 6;
    std::vector<unsigned char> thumb(sw * sh * 3), prevThumb, prev2Thumb;
    double prevGrid[GX * GY] = {}, prev2Grid[GX * GY] = {}, prevMean = -1; int n = 0, jumps = 0;
    for (;;) {
        QueryPerformanceCounter(&tn); double t = (tn.QuadPart - t0.QuadPart) * 1000.0 / fq.QuadPart;
        if (t > secs * 1000) break;
        DXGI_OUTDUPL_FRAME_INFO fi; IDXGIResource* res;
        hr = dup->AcquireNextFrame(200, &fi, &res);
        if (hr == DXGI_ERROR_WAIT_TIMEOUT) continue;
        if (FAILED(hr)) { printf("acq %08lx\n", hr); break; }
        if (fi.LastPresentTime.QuadPart == 0) { res->Release(); dup->ReleaseFrame(); continue; }
        ID3D11Texture2D* tex; res->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex);
        ctx->CopyResource(st, tex); tex->Release(); res->Release();
        D3D11_MAPPED_SUBRESOURCE m; ctx->Map(st, 0, D3D11_MAP_READ, 0, &m);
        double grid[GX * GY] = {}; int gcnt[GX * GY] = {}; double sr = 0, sg = 0, sb = 0; int cnt = 0;
        bool f16 = od.ModeDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT;
        for (int y = 0; y < sh; y++) for (int x = 0; x < sw; x++) {
            int px = x * 6, py = y * 6; double r, g, b;
            const unsigned char* row = (const unsigned char*)m.pData + (size_t)py * m.RowPitch;
            if (f16) { r = g = b = 0; } else { const unsigned char* p = row + px * 4; b = p[0] / 255.0; g = p[1] / 255.0; r = p[2] / 255.0; }
            unsigned char* s = &thumb[(y * sw + x) * 3]; s[0] = (unsigned char)(r * 255); s[1] = (unsigned char)(g * 255); s[2] = (unsigned char)(b * 255);
            double l = 0.299 * r + 0.587 * g + 0.114 * b; int gi = (py * GY / H) * GX + (px * GX / W);
            grid[gi] += l; gcnt[gi]++; sr += r; sg += g; sb += b; cnt++;
        }
        if (nFull > 0) { size_t sz = (size_t)W * H * 4; full2.swap(full1); full1.swap(full0); full0.resize(sz);
            for (int y = 0; y < H; y++) memcpy(&full0[(size_t)y * W * 4], (const unsigned char*)m.pData + (size_t)y * m.RowPitch, (size_t)W * 4); }
        ctx->Unmap(st, 0); dup->ReleaseFrame();
        double mean = (0.299 * sr + 0.587 * sg + 0.114 * sb) / cnt;
        fprintf(fc, "%.2f,%u,%.4f,%.4f,%.4f", t, fi.AccumulatedFrames, sr / cnt, sg / cnt, sb / cnt);
        for (int i = 0; i < GX * GY; i++) { grid[i] /= gcnt[i]; fprintf(fc, ",%.3f", grid[i]); }
        fprintf(fc, "\n");
        // spike: frame n-1 differs from n-2 and from n, while n-2 and n agree (a one-frame flash)
        if (n >= 2) {
            double d10 = 0, d12 = 0, d02 = 0;
            for (int i = 0; i < GX * GY; i++) {
                d10 = max(d10, fabs(prevGrid[i] - grid[i])); d12 = max(d12, fabs(prevGrid[i] - prev2Grid[i]));
                d02 = max(d02, fabs(grid[i] - prev2Grid[i]));
            }
            if (d10 > 0.06 && d12 > 0.06 && d02 < 0.5 * min(d10, d12) && jumps < 80) {
                SavePPM(dir, n - 2, "s0", prev2Thumb, sw, sh); SavePPM(dir, n - 1, "s1", prevThumb, sw, sh); SavePPM(dir, n, "s2", thumb, sw, sh); jumps++;
                if (nFull > 0) { nFull--; SaveFull(dir, n - 1, full1, W, H); SaveFull(dir, n - 2, full2, W, H); SaveFull(dir, n, full0, W, H); }
                printf("spike at %d t=%.1f d10 %.3f d12 %.3f d02 %.3f\n", n - 1, t, d10, d12, d02);
            }
        }
        if (every > 0 && n % every == 0) SavePPM(dir, n, "", thumb, sw, sh);
        memcpy(prev2Grid, prevGrid, sizeof grid); memcpy(prevGrid, grid, sizeof grid);
        prev2Thumb = prevThumb;
        prevThumb = thumb; prevMean = mean; n++; fflush(stdout);
    }
    fclose(fc); printf("frames %d jumps %d\n", n, jumps); return 0;
}
