/*
*author:achilles_xu
*date:2015-5-21
*
*
*/
#ifndef GETRTPPACKET_H
#define GETRTPPACKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <WinSock2.h>

//调试信息
//#define DEBUG 1

//  海康的ps流解析支持输出音视频类型，类型定义，海康私有（待定）
//  stream_type  =  0x1B (H264/AVC)
//  stream_id    =  0xE0 (video)
//
//  stream_type  =  0x0F (ISO 13818-7 Audio - ADTS) -- AAC
//  stream_type  =  0x90 (User Private)             -- G.711 alaw
//  stream_type  =  0x91 (User Private)             -- G.711 ulaw
//  stream_type  =  0x96 (User Private)             -- G.726
//  stream_id    =  0xC0 (audio)
//

enum SpliteStreamGetES_MediaType{
	SpliteStreamGetES_MediaType_VEDIOTYPE = 0,
	SpliteStreamGetES_MediaType_AUDIOTYPE,
	SpliteStreamGetES_MediaType_NONEM
};
enum SpliteStreamGetES_FrameType{
	SpliteStreamGetES_FrameType_IFRAME = 0,
	SpliteStreamGetES_FrameType_PBFRAME,
	SpliteStreamGetES_FrameType_NONEF
};

typedef struct{
	SpliteStreamGetES_MediaType mType;
	SpliteStreamGetES_FrameType fType;
}SpliteStreamESInfo;


typedef void(*GetSpliteStreamESCallB)(char *buffer, int size, SpliteStreamESInfo Info, unsigned __int64 ltimestamp, void *p);


class SpliteStreamGetES{
public:
	//接口函数
	SpliteStreamGetES()
	{
		getESCB = 0;
		getp = 0;
	}
	virtual void setCallBack(GetSpliteStreamESCallB getES, void *p)
	{
		getESCB = getES;
		getp = p;
	}
	virtual ~SpliteStreamGetES()
	{
		getESCB = NULL;
		getp = NULL;

	}
	virtual int InputBufData(char* frame_buf, int frame_len) = 0;

	void *getp;
protected:
	GetSpliteStreamESCallB getESCB;
	
};

#endif







