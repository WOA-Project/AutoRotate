#include "pch.h"
#include <Windows.h>
#include <iostream>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Devices::Sensors;

//
// Subject: Change screen orientation while following current auto rotation settings
//
// Parameters:
//
//             Orientation:
//             - DMDO_270     (Portrait)
//             - DMDO_90      (Portrait flipped)
//             - DMDO_180     (Landscape)
//             - DMDO_DEFAULT (Landscape flipped)
//
// Returns: void
//
void ChangeOrientation(int Orientation)
{
	HKEY key;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AutoRotation", NULL, KEY_READ, &key) == ERROR_SUCCESS)
	{
		unsigned long type = REG_DWORD, size = 8;

		//
		// Check if we are supposed to use the mobile behavior, if we do, then prevent the screen from rotating to Portrait (flipped)
		//
		if (Orientation == DMDO_90)
		{
			unsigned int mobilebehavior = 0;
			RegQueryValueEx(key, L"MobileBehavior", NULL, &type, (LPBYTE)& mobilebehavior, &size);
			if (mobilebehavior == 1)
			{
				return;
			}
		}

		//
		// Check if rotation is enabled
		//
		unsigned int enabled = 0;
		RegQueryValueEx(key, L"Enable", NULL, &type, (LPBYTE)& enabled, &size);

		if (enabled == 1)
		{
			//
			// Get the current display settings
			//
			DEVMODE mode;
			EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &mode);

			//
			// In order to switch from portrait to landscape and vice versa we need to swap the resolution width and height
			// So we check for that
			//
			if ((mode.dmDisplayOrientation + Orientation) % 2 == 1)
			{
				int temp = mode.dmPelsHeight;
				mode.dmPelsHeight = mode.dmPelsWidth;
				mode.dmPelsWidth = temp;
			}

			//
			// Change the display orientation and save to the registry the changes (1 parameter for ChangeDisplaySettings)
			//
			if (mode.dmFields | DM_DISPLAYORIENTATION)
			{
				mode.dmDisplayOrientation = Orientation;
				ChangeDisplaySettingsEx(NULL, &mode, NULL, CDS_UPDATEREGISTRY | CDS_GLOBAL, NULL);
			}
		}
	}
}

int main()
{
    init_apartment();

	//
	// Get the default simple orientation sensor on the system
	//
	SimpleOrientationSensor sensor = SimpleOrientationSensor::GetDefault();

	//
	// If no sensor is found return 1
	//
	if (sensor == NULL)
	{
		return 1;
	}

	//
	// Set sensor present for windows to show the auto rotation toggle in action center
	//
	HKEY key;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AutoRotation", NULL, KEY_WRITE, &key) == ERROR_SUCCESS)
	{
		RegSetValueEx(key, L"SensorPresent", NULL, REG_DWORD, (LPBYTE)1, 8);
	}

	//
	// Subscribe to sensor events
	//
	sensor.OrientationChanged([](IInspectable const& /*sender*/, SimpleOrientationSensorOrientationChangedEventArgs const& args)
	{
			switch (args.Orientation())
			{
				// Portrait
				case SimpleOrientation::NotRotated:
				{
					ChangeOrientation(DMDO_270);
					break;
				}
				// Portrait (flipped)
				case SimpleOrientation::Rotated180DegreesCounterclockwise:
				{
					ChangeOrientation(DMDO_90);
					break;
				}
				// Landscape
				case SimpleOrientation::Rotated90DegreesCounterclockwise:
				{
					ChangeOrientation(DMDO_180);
					break;
				}
				// Landscape (flipped)
				case SimpleOrientation::Rotated270DegreesCounterclockwise:
				{
					ChangeOrientation(DMDO_DEFAULT);
					break;
				}
			}
	});

	// Wait indefinetly
	while (true)
	{
		Sleep(3000);
	}
}
