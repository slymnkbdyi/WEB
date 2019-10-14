/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** rtspaudiocapturer.h
**
** -------------------------------------------------------------------------*/

#pragma once

#include "rtc_base/ref_counted_object.h"

#include "rtspconnectionclient.h"

#include "liveaudiosource.h"

class RTSPAudioSource : public LiveAudioSource<RTSPConnection> {
	public:
		static rtc::scoped_refptr<RTSPAudioSource> Create(rtc::scoped_refptr<webrtc::AudioDecoderFactory> audioDecoderFactory, const std::string & uri, const std::map<std::string,std::string> & opts) {
			rtc::scoped_refptr<RTSPAudioSource> source(new rtc::RefCountedObject<RTSPAudioSource>(audioDecoderFactory, uri, opts));
			return source;
		}
		
	protected:
		RTSPAudioSource(rtc::scoped_refptr<webrtc::AudioDecoderFactory> audioDecoderFactory, const std::string & uri, const std::map<std::string,std::string> & opts); 
		virtual ~RTSPAudioSource();

};



