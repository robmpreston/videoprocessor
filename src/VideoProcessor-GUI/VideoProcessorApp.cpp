/*
 * Copyright(C) 2021 Dennis Fleurbaaij <mail@dennisfleurbaaij.com>
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. If not, see < https://www.gnu.org/licenses/>.
 */
#define CONFIGURU_IMPLEMENTATION 1
#include <configuru.hpp>

#include <pch.h>

#include <winnt.h>
extern "C" {
#include <libavutil/log.h>
}

#include <VideoProcessorDlg.h>

#include "VideoProcessorApp.h"


BEGIN_MESSAGE_MAP(CVideoProcessorApp, CWinAppEx)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


CVideoProcessorApp videoProcessorApp;


void av_log_callback(void* ptr, int level, const char* fmt, va_list vargs)
{
	vprintf(fmt, vargs);
}


BOOL CVideoProcessorApp::InitInstance()
{
	// Setup ffmpeg logging
	av_log_set_callback(av_log_callback);
#ifdef _DEBUG
	av_log_set_level(AV_LOG_TRACE);
#endif

	CVideoProcessorDlg dlg;
	m_pMainWnd = &dlg;

	try
	{
		if (!CWinAppEx::InitInstance())
			throw std::runtime_error("Failed to initialize VideoProcessorApp");

		// COINIT_MULTITHREADED was used in the Blackmagic SDK examples,
		// using that without further investigation
		if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
			throw std::runtime_error("Failed to initialize com objects");

		// Parse configuration file
		Config config = configuru::parse_file("settings.json", JSON);

		bool isFullScreen = config.get_or("fullscreen", false);
		bool videoConversionOverride = config.get_or("videoConversionOverride", false);
		std::string colorSpace = config.get_or("colorSpace", "Follow Input");
		std::string rendererName = config.get_or("renderer", "");
		std::string frameOffset = config.get_or("frameOffset", "auto");
		std::string hdrLuminance = config.get_or("luminance", "Follow input");
		std::string hdrColorSpace = config.get_or("hdrColorSpace", "Follow input")
		std::string dsStartStopTime = config.get_or("dsStartStopTime", "Clock-Smart");
		std::string dsNominalRange = config.get_or("dsNominalRange", "Auto");
		std::string dsTransferFunction = config.get_or("dsTransferFunction", "Auto");
		std::string dsTransferMatrix = config.get_or("dsTransferMatrix", "Auto");
		std::string dsPrimaries = config.get_or("dsNominalRange", "Auto");

		if (isFullScreen) {
			dlg.StartFullScreen();
		}

		if (videoConversionOverride) {
			dlg.SetVideoConversionOverride();
		}
		if (rendererName != "") {
			dlg.rendererName(rendererName);
		}
		if (frameOffset == "auto")
		{
			dlg.StartFrameOffsetAuto();
		}
		else
		{
			dlg.StartFrameOffset(frameOffset);
		}

		dlg.SetColorSpace(colorSpace)
		dlg.SetHdrLuminance(hdrLuminance);
		dlg.SetHdrColorSpace(colorSpace);
		dlg.SetStartStopTime(dsStartStopTime);
		dlg.SetNominalRange(dsNominalRange);
		dlg.SetTransferFunction(dsTransferFunction);
		dlg.SetTransferMatrix(dsTransferMatrix);
		dlg.SetPrimaries(dsNominalRange);

		// Set set ourselves to high prio.
		if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
			throw std::runtime_error("Failed to set process priority");

		dlg.DoModal();
	}
	catch (std::runtime_error& e)
	{
		dlg.EndDialog(IDABORT);

		size_t size = strlen(e.what()) + 1;
		wchar_t* wtext = new wchar_t[size];
		size_t outSize;
		mbstowcs_s(&outSize, wtext, size, e.what(), size - 1);

		MessageBox(nullptr, wtext, TEXT("Fatal error"), MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);

		delete[] wtext;
	}

	CoUninitialize();

	return FALSE;
}


// Only here for debugging purposes where the application is compiled as a console application.
int main() {
	return _tWinMain(GetModuleHandle(NULL), NULL, GetCommandLine(), SW_SHOW);
}
