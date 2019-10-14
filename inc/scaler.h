#pragma once

#include "media/base/video_broadcaster.h"
#include "api/media_stream_interface.h"

class Scaler :  public rtc::VideoSinkInterface<webrtc::VideoFrame>,  public rtc::VideoSourceInterface<webrtc::VideoFrame> 
{
public:

    Scaler(rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> videoSource, const std::map<std::string, std::string> &opts) :
                m_videoSource(videoSource),
                m_width(0), m_height(0), 
                m_roi_x(0), m_roi_y(0), m_roi_width(0), m_roi_height(0) 
    {
        if (opts.find("width") != opts.end())
        {
            m_width = std::stoi(opts.at("width"));
        }
        if (opts.find("height") != opts.end())
        {
            m_height = std::stoi(opts.at("height"));
        }
        if (opts.find("roi_x") != opts.end())
        {
            m_roi_x = std::stoi(opts.at("roi_x"));
            if (m_roi_x < 0)
            {
                RTC_LOG(LS_ERROR) << "Ignore roi_x=" << m_roi_x << ", it muss be >=0";
                m_roi_x = 0;
            }
        }
        if (opts.find("roi_y") != opts.end())
        {
            m_roi_y = std::stoi(opts.at("roi_y"));
            if (m_roi_y < 0)
            {
                RTC_LOG(LS_ERROR) << "Ignore roi_<=" << m_roi_y << ", it muss be >=0";
                m_roi_y = 0;
            }
        }
        if (opts.find("roi_width") != opts.end())
        {
            m_roi_width = std::stoi(opts.at("roi_width"));
            if (m_roi_width <= 0)
            {
                RTC_LOG(LS_ERROR) << "Ignore roi_width<=" << m_roi_width << ", it muss be >0";
                m_roi_width = 0;
            }
        }
        if (opts.find("roi_height") != opts.end())
        {
            m_roi_height = std::stoi(opts.at("roi_height"));
            if (m_roi_height <= 0)
            {
                RTC_LOG(LS_ERROR) << "Ignore roi_height<=" << m_roi_height << ", it muss be >0";
                m_roi_height = 0;
            }
        }
        const rtc::VideoSinkWants wants;
        m_videoSource->AddOrUpdateSink(this,wants);
    }

    virtual ~Scaler()
    {
        m_videoSource->RemoveSink(this);
    }

    void OnFrame(const webrtc::VideoFrame &frame) override
    {
        if (m_roi_x >= frame.width())
        {
            RTC_LOG(LS_ERROR) << "The ROI position protrudes beyond the right edge of the image. Ignore roi_x.";
            m_roi_x = 0;
        }
        if (m_roi_y >= frame.height())
        {
            RTC_LOG(LS_ERROR) << "The ROI position protrudes beyond the bottom edge of the image. Ignore roi_y.";
            m_roi_y = 0;
        }
        if (m_roi_width != 0 && (m_roi_width + m_roi_x) > frame.width())
        {
            RTC_LOG(LS_ERROR) << "The ROI protrudes beyond the right edge of the image. Ignore roi_width.";
            m_roi_width = 0;
        }
        if (m_roi_height != 0 && (m_roi_height + m_roi_y) > frame.height())
        {
            RTC_LOG(LS_ERROR) << "The ROI protrudes beyond the bottom edge of the image. Ignore roi_height.";
            m_roi_height = 0;
        }

        if (m_roi_width == 0)
        {
            m_roi_width = frame.width() - m_roi_x;
        }
        if (m_roi_height == 0)
        {
            m_roi_height = frame.height() - m_roi_y;
        }

        // source image is croped but destination image size is not set
        if ((m_roi_width != frame.width() || m_roi_height != frame.height()) && (m_height == 0 && m_width == 0))
        {
            m_height = m_roi_height;
            m_width = m_roi_width;
        }

        if ((m_height == 0) && (m_width == 0))
        {
            m_broadcaster.OnFrame(frame);
        }
        else
        {
            int height = m_height;
            int width = m_width;
            if (height == 0)
            {
                height = (m_roi_height * width) / m_roi_width;
            }
            else if (width == 0)
            {
                width = (m_roi_width * height) / m_roi_height;
            }
            int stride_y = width;
            int stride_uv = (width + 1) / 2;
            rtc::scoped_refptr<webrtc::I420Buffer> scaled_buffer = webrtc::I420Buffer::Create(width, height, stride_y, stride_uv, stride_uv);
            if (m_roi_width != frame.width() || m_roi_height != frame.height())
            {
                scaled_buffer->CropAndScaleFrom(*frame.video_frame_buffer()->ToI420(), m_roi_x, m_roi_y, m_roi_width, m_roi_height);
            }
            else
            {
                scaled_buffer->ScaleFrom(*frame.video_frame_buffer()->ToI420());
            }
            webrtc::VideoFrame scaledFrame = webrtc::VideoFrame(scaled_buffer, frame.timestamp(),
                                                          frame.render_time_ms(), webrtc::kVideoRotation_0);

            m_broadcaster.OnFrame(scaledFrame);
        }
    }

    void AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame> *sink, const rtc::VideoSinkWants &wants) override
    {
        m_broadcaster.AddOrUpdateSink(sink, wants);
    }

    void RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame> *sink) override
    {
        m_broadcaster.RemoveSink(sink);
    }

    rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> m_videoSource;
    rtc::VideoBroadcaster  m_broadcaster;

    int                    m_width;
    int                    m_height;
    int                    m_roi_x;
    int                    m_roi_y;
    int                    m_roi_width;
    int                    m_roi_height;    
};

#include "pc/video_track_source.h"
template<class T>
class Filter : public webrtc::VideoTrackSource {
public:
	static rtc::scoped_refptr<Filter> Create(rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> videoSource, const std::map<std::string, std::string> &opts) {
		std::unique_ptr<T> source = absl::WrapUnique(new T(videoSource, opts));
		if (!source) {
			return nullptr;
		}
		return new rtc::RefCountedObject<Filter>(std::move(source));
	}

protected:
	explicit Filter(std::unique_ptr<T> source)
		: webrtc::VideoTrackSource(/*remote=*/false), m_source(std::move(source)) {}

private:
	rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
		return m_source.get();
	}
	std::unique_ptr<T> m_source;
};