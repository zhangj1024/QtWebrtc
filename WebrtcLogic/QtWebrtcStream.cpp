#include "stdafx.h"
#include "QtWebrtcStream.h"
#include "api/video/i420_buffer.h"
#include "rtc_base/arraysize.h"
#include "libyuv/convert_argb.h"

QtVideoRenderer::QtVideoRenderer(
	HWND wnd, int width, int height,
	webrtc::VideoTrackInterface* track_to_render)
	: wnd_(wnd), rendered_track_(track_to_render) {
	::InitializeCriticalSection(&buffer_lock_);
	ZeroMemory(&bmi_, sizeof(bmi_));
	bmi_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi_.bmiHeader.biPlanes = 1;
	bmi_.bmiHeader.biBitCount = 32;
	bmi_.bmiHeader.biCompression = BI_RGB;
	bmi_.bmiHeader.biWidth = width;
	bmi_.bmiHeader.biHeight = -height;
	bmi_.bmiHeader.biSizeImage = width * height *
		(bmi_.bmiHeader.biBitCount >> 3);
	rendered_track_->AddOrUpdateSink(this, rtc::VideoSinkWants());
}

QtVideoRenderer::~QtVideoRenderer() {
	rendered_track_->RemoveSink(this);
	::DeleteCriticalSection(&buffer_lock_);
}

void QtVideoRenderer::SetSize(int width, int height) {
	AutoLock<QtVideoRenderer> lock(this);

	if (width == bmi_.bmiHeader.biWidth && height == abs(bmi_.bmiHeader.biHeight)) {
		return;
	}

	bmi_.bmiHeader.biWidth = width;
	bmi_.bmiHeader.biHeight = -height;
	bmi_.bmiHeader.biSizeImage = width * height *
		(bmi_.bmiHeader.biBitCount >> 3);
	image_.reset(new uint8_t[bmi_.bmiHeader.biSizeImage]);
	imageRGB24_.reset(new uint8_t[width * height * 3]);
}

void ARGBToRGB(const uint8_t* src, uint8_t* dst, int width, int height)
{
	qint64 length = width * height;
	for (qint64 i = 0; i < length; i++)
	{
		dst[0 + i * 3] = src[0 + i * 4];
		dst[1 + i * 3] = src[1 + i * 4];
		dst[2 + i * 3] = src[2 + i * 4];
	}
}

bool _ResizeWithMendBlack(uint8_t* pDst, uint8_t* pSrc, uint32_t uDstLen, uint32_t uSrcLen, const SIZE & dstSize, const SIZE & srcSize, uint32_t bpp)
{
	if (!pDst || !pSrc)
		return false;

	if (uDstLen == 0 || uDstLen != dstSize.cx * dstSize.cy * bpp)
		return false;

	if (uSrcLen == 0 || uSrcLen != srcSize.cx * srcSize.cy * bpp)
		return false;

	if (dstSize.cx < srcSize.cx)
		return false;

	if (dstSize.cy < srcSize.cy)
		return false;

	uint32_t dstLineblockSize = dstSize.cx * bpp;
	uint32_t srcLineblockSize = srcSize.cx * bpp;

	int mendCxLeftEnd = dstSize.cx > srcSize.cx ? (dstSize.cx - srcSize.cx) / 2 : 0;
	int mendCyToEnd = dstSize.cy > srcSize.cy ? (dstSize.cy - srcSize.cy) / 2 : 0;

	for (int y = 0; y < dstSize.cy; y++)
	{
		if (y >= mendCyToEnd && y < mendCyToEnd + srcSize.cy)
		{
			if (mendCxLeftEnd > 0)
				memcpy(pDst + (bpp * mendCxLeftEnd), pSrc, srcLineblockSize);
			else
				memcpy(pDst, pSrc, srcLineblockSize);
			pSrc += srcLineblockSize;
		}
		pDst += dstLineblockSize;
	}
	return true;
}

void QtVideoRenderer::paintPic(uint8_t* pData, int srcWidth, int srcHeight)
{
	if (pData == NULL)
	{
		return;
	}

	RECT rc;
	::GetClientRect(handle(), &rc);

	int winWidth = rc.right;
	int winHeight = rc.bottom;

	int dstWidth = 0;
	int dstHeight = 0;

	float fWinRate = winWidth / (float)winHeight;
	float fSrcRate = srcWidth / (float)srcHeight;

	if (fWinRate > fSrcRate)//高不变，宽填充黑边
	{
		dstWidth = (int)(srcHeight * winWidth / (float)winHeight);
		dstHeight = srcHeight;
	}
	else//宽不变,高填充黑边
	{
		dstWidth = srcWidth;
		dstHeight = (int)(srcWidth * winHeight / (float)winWidth);
	}
	dstWidth = (dstWidth + 3) / 4 * 4; //保证宽度是4的倍数。

	if (m_nRenderDataBufLen != dstWidth * dstHeight * 3)
	{
		if (m_pRenderDataBuf != NULL)
		{
			delete m_pRenderDataBuf;
			m_pRenderDataBuf = NULL;
		}

		m_nRenderDataBufLen = dstWidth * dstHeight * 3; //RGB24
		m_pRenderDataBuf = new uint8_t[m_nRenderDataBufLen];
	}
	memset(m_pRenderDataBuf, 0, m_nRenderDataBufLen);//清除上一次渲染

	SIZE srcSize = { srcWidth, srcHeight };
	SIZE dstSize = { dstWidth, dstHeight };

	_ResizeWithMendBlack(m_pRenderDataBuf, pData, m_nRenderDataBufLen, srcWidth * srcHeight * 3, dstSize, srcSize, 3);

	BITMAPINFO Bitmap;
	memset(&Bitmap, 0, sizeof(BITMAPINFO));
	Bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	Bitmap.bmiHeader.biWidth = dstWidth;
	Bitmap.bmiHeader.biHeight = -dstHeight;

	Bitmap.bmiHeader.biBitCount = 3 * 8;//COLOR_FORMAT_RGB24	
	Bitmap.bmiHeader.biPlanes = 1;
	Bitmap.bmiHeader.biCompression = BI_RGB;//COLOR_FORMAT_RGB24	
	Bitmap.bmiHeader.biSizeImage = 0;
	Bitmap.bmiHeader.biClrUsed = 0;
	Bitmap.bmiHeader.biXPelsPerMeter = 0;
	Bitmap.bmiHeader.biYPelsPerMeter = 0;
	Bitmap.bmiHeader.biClrImportant = 0;

	HWND hWnd = wnd_;
	HDC hDC = GetDC(hWnd);
	HDC hMemDC = CreateCompatibleDC(hDC);
	HBITMAP hMemBitmap = CreateCompatibleBitmap(hDC, winWidth, winHeight);
	SelectObject(hMemDC, hMemBitmap);

	SetStretchBltMode(hMemDC, HALFTONE);
	SetBrushOrgEx(hMemDC, 0, 0, NULL);
	StretchDIBits(hMemDC, 0, 0, winWidth, winHeight, 0, 0, dstWidth, dstHeight,
		m_pRenderDataBuf, &Bitmap, DIB_RGB_COLORS, SRCCOPY);

	BitBlt(hDC, 0, 0, winWidth, winHeight, hMemDC, 0, 0, SRCCOPY);

	DeleteObject(hMemBitmap);
	DeleteObject(hMemDC);
	ReleaseDC(hWnd, hDC);
}

void QtVideoRenderer::OnFrame(const webrtc::VideoFrame& video_frame)
{
#pragma region MyRegion
	AutoLock<QtVideoRenderer> lock(this);
	rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(
		video_frame.video_frame_buffer()->ToI420());
	if (video_frame.rotation() != webrtc::kVideoRotation_0) {
		buffer = webrtc::I420Buffer::Rotate(*buffer, video_frame.rotation());
	}

	SetSize(buffer->width(), buffer->height());

	RTC_DCHECK(image_.get() != NULL);
	libyuv::I420ToARGB(buffer->DataY(), buffer->StrideY(),
		buffer->DataU(), buffer->StrideU(),
		buffer->DataV(), buffer->StrideV(),
		image_.get(),
		bmi_.bmiHeader.biWidth *
		bmi_.bmiHeader.biBitCount / 8,
		buffer->width(), buffer->height());

	const BITMAPINFO& bmi = this->bmi();
	int height = abs(bmi.bmiHeader.biHeight);
	int width = bmi.bmiHeader.biWidth;

#if 1
	ARGBToRGB(image_.get(), imageRGB24_.get(), width, height);

	paintPic(imageRGB24_.get(), width, height);
#else

#pragma endregion

	InvalidateRect(wnd_, NULL, TRUE);

	PAINTSTRUCT ps;
	::BeginPaint(handle(), &ps);

	RECT rc;
	::GetClientRect(handle(), &rc);


	const uint8_t* image = this->image();
	if (image != NULL) {
		HDC dc_mem = ::CreateCompatibleDC(ps.hdc);
		::SetStretchBltMode(dc_mem, HALFTONE);

		// Set the map mode so that the ratio will be maintained for us.
		HDC all_dc[] = { ps.hdc, dc_mem };
		for (int i = 0; i < arraysize(all_dc); ++i) {
			SetMapMode(all_dc[i], MM_ISOTROPIC);
			SetWindowExtEx(all_dc[i], width, height, NULL);
			SetViewportExtEx(all_dc[i], rc.right, rc.bottom, NULL);
		}

		HBITMAP bmp_mem = ::CreateCompatibleBitmap(ps.hdc, rc.right, rc.bottom);
		HGDIOBJ bmp_old = ::SelectObject(dc_mem, bmp_mem);

		POINT logical_area = { rc.right, rc.bottom };
		DPtoLP(ps.hdc, &logical_area, 1);

		HBRUSH brush = ::CreateSolidBrush(RGB(0, 0, 0));
		RECT logical_rect = { 0, 0, logical_area.x, logical_area.y };
		::FillRect(dc_mem, &logical_rect, brush);
		::DeleteObject(brush);

		int x = (logical_area.x / 2) - (width / 2);
		int y = (logical_area.y / 2) - (height / 2);

		StretchDIBits(dc_mem, x, y, width, height,
			0, 0, width, height, image, &bmi, DIB_RGB_COLORS, SRCCOPY);

		BitBlt(ps.hdc, 0, 0, logical_area.x, logical_area.y,
			dc_mem, 0, 0, SRCCOPY);

		// Cleanup.
		::SelectObject(dc_mem, bmp_old);
		::DeleteObject(bmp_mem);
		::DeleteDC(dc_mem);
	}
#endif
}

QtWebrtcStream::QtWebrtcStream()
{
}

QtWebrtcStream::~QtWebrtcStream()
{
	StopView();
	_audioTrack = NULL;
	_videoTrack = NULL;
	_renderer = NULL;
}

void QtWebrtcStream::SetHwnd(HWND hwnd)
{
	_wnd = hwnd;
}

void QtWebrtcStream::SetVideoTrack(rtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrack)
{
	_videoTrack = videoTrack;
}

void QtWebrtcStream::StartView()
{
	if (_videoTrack != NULL)
	{
		_renderer.reset(new QtVideoRenderer(_wnd, 1, 1, _videoTrack));
	}
}

void QtWebrtcStream::StopView()
{
	_renderer.reset();
}

