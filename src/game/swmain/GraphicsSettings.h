#pragma once

// The port's graphics-card quality table. Only a quality level is needed. LoadCardQualitySettings
// parses a devicedefaults.ini whose sections carry a vendor/device pair and whose quality= key is
// the level, and the GL-query entry point is a stub (the macOS build's GetDeviceFromGL is a real
// CGL query).
struct GraphicsSettings
{
    // The one card record, filled in by LoadCardQualitySettings.
    struct MyGraphicsCard
    {
        int m_nVendorId;
        int m_nDeviceId;
        int m_nQualityLevel;
    };

    static MyGraphicsCard s_myGraphicsCard;

    static void Initialize();
    static void GetDeviceFromGL();
    static int  GetQualityLevel();
    static void LoadCardQualitySettings();
};
