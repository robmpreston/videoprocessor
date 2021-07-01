/*
 * Copyright(C) 2021 Dennis Fleurbaaij <mail@dennisfleurbaaij.com>
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. If not, see < https://www.gnu.org/licenses/>.
 */

#include "stdafx.h"

#include <CNoopVideoFrameFormatter.h>
#include <microsoft_directshow/DirectShowTranslations.h>

#include "CVideoInfo1DirectShowRenderer.h"


CVideoInfo1DirectShowRenderer::CVideoInfo1DirectShowRenderer(
	GUID rendererCLSID,
	IRendererCallback& callback,
	HWND videoHwnd,
	HWND eventHwnd,
	UINT eventMsg,
	ITimingClock* timingClock,
	VideoStateComPtr& videoState,
	RendererTimestamp timestamp,
	bool useFrameQueue,
	size_t frameQueueMaxSize):
	ADirectShowRenderer(
		rendererCLSID,
		callback,
		videoHwnd,
		eventHwnd,
		eventMsg,
		timingClock,
		videoState,
		timestamp,
		useFrameQueue,
		frameQueueMaxSize,
		false /* useHDRDdata */)
{
}

void CVideoInfo1DirectShowRenderer::MediaTypeGenerate()
{

	GUID mediaSubType = TranslateToMediaSubType(m_videoState->pixelFormat);
	int bitCount = PixelFormatBitsPerPixel(m_videoState->pixelFormat);

	m_videoFramFormatter = new CNoopVideoFrameFormatter();
	m_videoFramFormatter->OnVideoState(m_videoState);


	// Build PMT
	assert(!m_pmt.pbFormat);
	ZeroMemory(&m_pmt, sizeof(AM_MEDIA_TYPE));

	m_pmt.formattype = FORMAT_VideoInfo;
	m_pmt.cbFormat = sizeof(VIDEOINFOHEADER);
	m_pmt.majortype = MEDIATYPE_Video;
	m_pmt.subtype = mediaSubType;
	m_pmt.bFixedSizeSamples = TRUE;
	m_pmt.bTemporalCompression = FALSE;

	m_pmt.pbFormat = (BYTE*)CoTaskMemAlloc(sizeof(VIDEOINFOHEADER));
	if (!m_pmt.pbFormat)
		throw std::runtime_error("Out of mem");

	VIDEOINFOHEADER* pvi = (VIDEOINFOHEADER*)m_pmt.pbFormat;
	ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));

	// Populate bitmap info header
	// https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader

	pvi->bmiHeader.biSizeImage = m_videoFramFormatter->GetOutFrameSize();
	pvi->bmiHeader.biBitCount = bitCount;
	pvi->bmiHeader.biCompression = m_pmt.subtype.Data1;
	pvi->bmiHeader.biWidth = m_videoState->displayMode->FrameWidth();
	pvi->bmiHeader.biHeight = ((long)m_videoState->displayMode->FrameHeight()) * (m_videoState->invertedVertical ? -1 : 1);
	pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pvi->bmiHeader.biPlanes = 1;
	pvi->bmiHeader.biClrImportant = 0;
	pvi->bmiHeader.biClrUsed = 0;
	pvi->AvgTimePerFrame = (REFERENCE_TIME)(UNITS / m_videoState->displayMode->RefreshRateHz());

	m_pmt.lSampleSize = DIBSIZE(pvi->bmiHeader);
}


void CVideoInfo1DirectShowRenderer::Connect()
{
	IEnumPins* pEnum = nullptr;
	IPin* pLiveSourceOutputPin = nullptr;
	IPin* pRendererInputPin = nullptr;

	if (FAILED(m_liveSource->EnumPins(&pEnum)))
		throw std::runtime_error("Failed to get livesource pin enumerator");

	if (pEnum->Next(1, &pLiveSourceOutputPin, nullptr) != S_OK)
	{
		pEnum->Release();

		throw std::runtime_error("Failed to run next on livesource pin");
	}

	pEnum->Release();
	pEnum = nullptr;

	if (FAILED(m_pRenderer->EnumPins(&pEnum)))
	{
		pLiveSourceOutputPin->Release();
		pRendererInputPin->Release();

		throw std::runtime_error("Failed to get livesource pin enumerator");
	}

	if (pEnum->Next(1, &pRendererInputPin, nullptr) != S_OK)
	{
		pLiveSourceOutputPin->Release();
		pRendererInputPin->Release();
		pEnum->Release();

		throw std::runtime_error("Failed to get livesource pin enumerator");
	}

	pEnum->Release();
	pEnum = nullptr;

	// Allow intermediary filters to help rendering
	if (FAILED(m_pGraph->Connect(pLiveSourceOutputPin, pRendererInputPin)))
	{
		pLiveSourceOutputPin->Release();
		pRendererInputPin->Release();

		throw std::runtime_error("Failed to connect pins");
	}

	pLiveSourceOutputPin->Release();
	pRendererInputPin->Release();
}

