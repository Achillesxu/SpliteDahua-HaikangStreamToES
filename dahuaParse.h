/*
*author:achilles_xu
*date:2015-5-22
*description:separate the dahua data stream to vedio and audio elementary stream
*
*
*
***/

#ifndef DAHUA_PARSE_H
#define DAHUA_PARSE_H

#include "SpliteStreamGetES.h"

//当前解析组帧，如果视频帧不完整就舍弃，可以将完整帧的计数输出，当前接口没有相应的变量
//应该设置大华解析的结构体


//大华视频FD-----I帧，FC-----P帧，F1-----未知，音频F0
//含有大华头
//大华缓冲头尾字节数
#define DAHUA_VIDEO_I_HEADSIZE 40
#define DAHUA_VIDEO_I_TAILSIZE 8
//需要测试
#define DAHUA_VIDEO_PB_HEADSIZE 40

//#define DAHUA_VIDEO_PB_HEADSIZE 32
#define DAHUA_VIDEO_PB_TAILSIZE 8

#define DAHUA_AUDIO_HEADSIZE 36
#define DAHUA_AUDIO_TAILSIZE 8

#define VEDIO_CLOCK 90000

//缓冲长度
#define MAX_BUF_SIZE (1024*1024)

typedef struct{
	char *buf;
	int cur_len;

}Unit_t;

class DahuaParse :public SpliteStreamGetES
{
private:
	Unit_t *m_menUnit;
	unsigned int m_iPreFrameLen;
	unsigned int m_iSufFrameLen;
	bool m_bComplete;
	bool m_bFrameType;//是i frame吗？
	//unsigned int m_iAudioCnt;
	unsigned int m_iVedioFstCnt;
	unsigned int m_iVedioCurCnt;
	unsigned int m_ivFrameRate;
	unsigned int m_ivFrameRateInc;
	bool m_bFirstIFrame;

	char *m_pIFrameContent;
	int   m_iIFrameLen;
	
	DahuaParse(const DahuaParse &dahPar){}
	DahuaParse& operator=(const DahuaParse &dahPar){}
public:
	DahuaParse();
	~DahuaParse();
	virtual int InputBufData(char* frame_buf, int frame_len);
//here we dont support the audio info in sdp
	int getSDPStr(std::string &sdpStr, bool withAudio = false);

};


#endif