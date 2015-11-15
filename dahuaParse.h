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

//��ǰ������֡�������Ƶ֡�����������������Խ�����֡�ļ����������ǰ�ӿ�û����Ӧ�ı���
//Ӧ�����ô󻪽����Ľṹ��


//����ƵFD-----I֡��FC-----P֡��F1-----δ֪����ƵF0
//���д�ͷ
//�󻪻���ͷβ�ֽ���
#define DAHUA_VIDEO_I_HEADSIZE 40
#define DAHUA_VIDEO_I_TAILSIZE 8
//��Ҫ����
#define DAHUA_VIDEO_PB_HEADSIZE 40

//#define DAHUA_VIDEO_PB_HEADSIZE 32
#define DAHUA_VIDEO_PB_TAILSIZE 8

#define DAHUA_AUDIO_HEADSIZE 36
#define DAHUA_AUDIO_TAILSIZE 8

#define VEDIO_CLOCK 90000

//���峤��
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
	bool m_bFrameType;//��i frame��
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