#pragma once

#include <memory>
#include "api/mediastreaminterface.h"

// A little helper class to make sure we always to proper locking and
// unlocking when working with QtVideoRenderer buffers.
template <typename T>
class AutoLock {
public:
	explicit AutoLock(T* obj) : obj_(obj) { obj_->Lock(); }
	~AutoLock() { obj_->Unlock(); }
protected:
	T* obj_;
};

class QtVideoRenderer : public rtc::VideoSinkInterface<webrtc::VideoFrame>
{
public:
	QtVideoRenderer(HWND wnd, int width, int height,
		webrtc::VideoTrackInterface* track_to_render);
	virtual ~QtVideoRenderer();

	void Lock() {
		::EnterCriticalSection(&buffer_lock_);
	}

	void Unlock() {
		::LeaveCriticalSection(&buffer_lock_);
	}

	// VideoSinkInterface implementation
	void OnFrame(const webrtc::VideoFrame& frame) override;

	const BITMAPINFO& bmi() const { return bmi_; }
	const uint8_t* image() const { return image_.get(); }

	HWND handle() const { return wnd_; }
	void sethandle(HWND wnd) { wnd_ = wnd; };

protected:
	void SetSize(int width, int height);

	void paintPic(uint8_t* pData, int srcWidth, int srcHeight);

	enum {
		SET_SIZE,
		RENDER_FRAME,
	};

	HWND wnd_;
	BITMAPINFO bmi_;
	std::unique_ptr<uint8_t[]> image_;
	std::unique_ptr<uint8_t[]> imageRGB24_;
	CRITICAL_SECTION buffer_lock_;
	rtc::scoped_refptr<webrtc::VideoTrackInterface> rendered_track_;

	qint64 m_nRenderDataBufLen = 0;
	uint8_t* m_pRenderDataBuf = NULL;
};

class QtWebrtcStream
{
public:
	QtWebrtcStream();
	virtual ~QtWebrtcStream();

	rtc::scoped_refptr<webrtc::VideoTrackInterface> GetVideoTrack() { return _videoTrack; };
	rtc::scoped_refptr<webrtc::AudioTrackInterface> GetAudioTrack() { return _audioTrack; };

public:
	void SetHwnd(HWND hwnd);
	HWND GetHwnd() { return _wnd; };
	void SetVideoTrack(rtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrack);
	void StartView();
	void StopView();

protected:
	rtc::scoped_refptr<webrtc::VideoTrackInterface> _videoTrack;
	rtc::scoped_refptr<webrtc::AudioTrackInterface> _audioTrack;
	std::unique_ptr<QtVideoRenderer> _renderer;
	HWND _wnd;
};
