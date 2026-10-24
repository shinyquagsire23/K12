#include "GraphicsSettings.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

GraphicsSettings::MyGraphicsCard GraphicsSettings::s_myGraphicsCard;

void GraphicsSettings::LoadCardQualitySettings()
{
    std::ifstream file;
    file.open("devicedefaults.ini");

    std::string line;
    int nVendor = 0;

    while (std::getline(file, line))
    {
        if (line[0] == ';')
            continue;

        size_t nVendorPos = line.find("[vendor=");
        if (nVendorPos != std::string::npos)
        {
            nVendor = static_cast<int>(strtol(line.c_str() + nVendorPos + strlen("[vendor="), NULL, 16));
            continue;
        }

        if (nVendor != s_myGraphicsCard.m_nVendorId)
            continue;
        if (static_cast<int>(strtol(line.c_str(), NULL, 16)) != s_myGraphicsCard.m_nDeviceId)
            continue;

        size_t nEqualsPos = line.find("=");
        if (nEqualsPos != std::string::npos)
        {
            s_myGraphicsCard.m_nQualityLevel = static_cast<int>(strtol(line.c_str() + nEqualsPos + 1, NULL, 10));
            break;
        }
    }
}

void GraphicsSettings::GetDeviceFromGL()
{
}

void GraphicsSettings::Initialize()
{
    s_myGraphicsCard.m_nVendorId = 0;
    s_myGraphicsCard.m_nDeviceId = 0;
    s_myGraphicsCard.m_nQualityLevel = -1;
    GetDeviceFromGL();
    LoadCardQualitySettings();
}

int GraphicsSettings::GetQualityLevel()
{
    return s_myGraphicsCard.m_nQualityLevel;
}
