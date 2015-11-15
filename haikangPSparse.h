/*
*author:achilles_xu
*date:2015-5-22
*description:we get the input data which is ps stream which is not standard ps stream because it is without systemheader
*addtion:we change the interface to add timestamp
*
*
***/
#ifndef HAIKANG_PS_PARSE_H
#define HAIKANG_PS_PARSE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <set>

//为了使用函数      ntohs()
#include <winsock2.h> 

#include "SpliteStreamGetES.h"

typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

#define MAX_PS_LENGTH  (0x100000)
#define MAX_PES_LENGTH (0xFFFF)
#define MAX_ES_LENGTH  (0x100000)

enum MediaTypeP
{
	AUDIO_T = 0,
	VEDIO_T,
	UNKNOW_T
};

enum PSStatus
{
	ps_padding, //未知状态
	ps_ps,      //ps状态
	ps_sh,
	ps_psm,
	ps_pes,
	ps_pes_video,
	ps_pes_audio,
	ps_pes_last,
	ps_pes_lost, //pes包丢失数据（udp传输的rtp丢包）
	ps_pes_jump,   //需要跳过的pes packet
	ps_pes_none
};

enum NalType
{
	I_Nal = 5,
	sei_Nal = 6,
	sps_Nal = 7,
	pps_Nal = 8,
	other,
	unknow = 0
};

#pragma pack(1)

//ps header
typedef struct ps_header{
	unsigned char pack_start_code[4];  //'0x000001BA'

	unsigned char system_clock_reference_base21 : 2;
	unsigned char marker_bit : 1;
	unsigned char system_clock_reference_base1 : 3;
	unsigned char fix_bit : 2;    //'01'

	unsigned char system_clock_reference_base22;

	unsigned char system_clock_reference_base31 : 2;
	unsigned char marker_bit1 : 1;
	unsigned char system_clock_reference_base23 : 5;

	unsigned char system_clock_reference_base32;
	unsigned char system_clock_reference_extension1 : 2;
	unsigned char marker_bit2 : 1;
	unsigned char system_clock_reference_base33 : 5; //system_clock_reference_base 33bit

	unsigned char marker_bit3 : 1;
	unsigned char system_clock_reference_extension2 : 7; //system_clock_reference_extension 9bit

	unsigned char program_mux_rate1;

	unsigned char program_mux_rate2;
	unsigned char marker_bit5 : 1;
	unsigned char marker_bit4 : 1;
	unsigned char program_mux_rate3 : 6;

	unsigned char pack_stuffing_length : 3;
	unsigned char reserved : 5;

	uint16_t ffs_sign;//FF FF
	uint32_t ps_cnt;

}ps_header_t;  //14
//system header
typedef struct sh_header
{
	unsigned char system_header_start_code[4]; //32

	unsigned char header_length[2];            //16 uimsbf

	uint32_t marker_bit1 : 1;   //1  bslbf
	uint32_t rate_bound : 22;   //22 uimsbf
	uint32_t marker_bit2 : 1;   //1 bslbf
	uint32_t audio_bound : 6;   //6 uimsbf
	uint32_t fixed_flag : 1;    //1 bslbf
	uint32_t CSPS_flag : 1;     //1 bslbf

	uint16_t system_audio_lock_flag : 1;  // bslbf
	uint16_t system_video_lock_flag : 1;  // bslbf
	uint16_t marker_bit3 : 1;             // bslbf
	uint16_t video_bound : 5;             // uimsbf
	uint16_t packet_rate_restriction_flag : 1; //bslbf
	uint16_t reserved_bits : 7;                //bslbf
	unsigned char reserved[6];
}sh_header_t; //18
//program stream map header
typedef struct psm_header{
	unsigned char promgram_stream_map_start_code[4];

	unsigned short program_stream_map_length;

	unsigned char program_stream_map_version : 5;
	unsigned char reserved1 : 2;
	unsigned char current_next_indicator : 1;

	unsigned char marker_bit : 1;
	unsigned char reserved2 : 7;

	unsigned short program_stream_info_length;
	unsigned short elementary_stream_map_length;
	unsigned char stream_type;
	unsigned char elementary_stream_id;
	unsigned short elementary_stream_info_length;
	unsigned char CRC_32[4];
	unsigned char reserved[16];
}psm_header_t; //36
//??
typedef struct pes_header
{
	unsigned char pes_start_code_prefix[3];
	unsigned char stream_id;
	unsigned short PES_packet_length;
}pes_header_t; //6
//??
typedef struct optional_pes_header{
	unsigned char original_or_copy : 1;
	unsigned char copyright : 1;
	unsigned char data_alignment_indicator : 1;
	unsigned char PES_priority : 1;
	unsigned char PES_scrambling_control : 2;
	unsigned char fix_bit : 2;

	unsigned char PES_extension_flag : 1;
	unsigned char PES_CRC_flag : 1;
	unsigned char additional_copy_info_flag : 1;
	unsigned char DSM_trick_mode_flag : 1;
	unsigned char ES_rate_flag : 1;
	unsigned char ESCR_flag : 1;
	unsigned char PTS_DTS_flags : 2;

	unsigned char PES_header_data_length;
}optional_pes_header_t;

typedef struct pes_status_data{
	bool pesTrued;
	PSStatus pesType;
	pes_header_t *pesPtr;
}pes_status_data_t;
/*
typedef struct esFrameData{
	MediaTypeP mediaType;
	bool Iframe;
	uint32_t frame_len;
	char *frame_data;

}esFrameData_t;
*/

typedef struct stESMapInfo{
	unsigned char streamType;
	unsigned char eleStreamId;
	unsigned short elsStreamInfoLen;
}ESMapInfo;

typedef struct stPosSign{
	bool used;
	uint32_t sign;
}PosSign;

#pragma pack()

#ifndef AV_RB16
#   define AV_RB16(x)                           \
	((((const unsigned char*)(x))[0] << 8) | \
	((const unsigned char*)(x))[1])
#endif


static inline unsigned __int64 ff_parse_pes_pts(const unsigned char* buf) {
	return (unsigned __int64)(*buf & 0x0e) << 29 |
		(AV_RB16(buf + 1) >> 1) << 15 |
		AV_RB16(buf + 3) >> 1;
}

static unsigned __int64 get_pts(optional_pes_header* option)
{
	if (option->PTS_DTS_flags != 2 && option->PTS_DTS_flags != 3 && option->PTS_DTS_flags != 0)
	{
		return 0;
	}
	if ((option->PTS_DTS_flags & 2) == 2)
	{
		unsigned char* pts = (unsigned char*)option + sizeof(optional_pes_header);
		return ff_parse_pes_pts(pts);
	}
	return 0;
}

static unsigned __int64 get_dts(optional_pes_header* option)
{
	if (option->PTS_DTS_flags != 2 && option->PTS_DTS_flags != 3 && option->PTS_DTS_flags != 0)
	{
		return 0;
	}
	if ((option->PTS_DTS_flags & 3) == 3)
	{
		unsigned char* dts = (unsigned char*)option + sizeof(optional_pes_header)+5;
		return ff_parse_pes_pts(dts);
	}
	return 0;
}

bool inline is_ps_header(ps_header_t* ps)
{
	if (ps->pack_start_code[0] == 0 && ps->pack_start_code[1] == 0 && ps->pack_start_code[2] == 1 && ps->pack_start_code[3] == 0xBA)
		return true;
	return false;
}
bool inline is_sh_header(sh_header_t* sh)
{
	if (sh->system_header_start_code[0] == 0 && sh->system_header_start_code[1] == 0 && sh->system_header_start_code[2] == 1 && sh->system_header_start_code[3] == 0xBB)
		return true;
	return false;
}
bool inline is_psm_header(psm_header_t* psm)
{
	if (psm->promgram_stream_map_start_code[0] == 0 && psm->promgram_stream_map_start_code[1] == 0 && psm->promgram_stream_map_start_code[2] == 1 && psm->promgram_stream_map_start_code[3] == 0xBC)
		return true;
	return false;
}
bool inline is_pes_video_header(pes_header_t* pes)
{
	if (pes->pes_start_code_prefix[0] == 0 && pes->pes_start_code_prefix[1] == 0 && pes->pes_start_code_prefix[2] == 1 && pes->stream_id == 0xE0)
		return true;
	return false;
}
bool inline is_pes_audio_header(pes_header_t* pes)
{
	if (pes->pes_start_code_prefix[0] == 0 && pes->pes_start_code_prefix[1] == 0 && pes->pes_start_code_prefix[2] == 1 && pes->stream_id == 0xC0)
		return true;
	return false;
}

bool inline is_pes_header(pes_header_t* pes)
{
	if (pes->pes_start_code_prefix[0] == 0 && pes->pes_start_code_prefix[1] == 0 && pes->pes_start_code_prefix[2] == 1)
	{
		if (pes->stream_id == 0xC0 || pes->stream_id == 0xE0)
		{
			return true;
		}
	}
	return false;
}

PSStatus inline pes_type(pes_header_t* pes)
{
	if (NULL == pes)
		return ps_pes_none;
	if (pes->pes_start_code_prefix[0] == 0 && pes->pes_start_code_prefix[1] == 0 && pes->pes_start_code_prefix[2] == 1)
	{
		if (pes->stream_id == 0xC0)
		{
			return ps_pes_audio;
		}
		else if (pes->stream_id == 0xE0)
		{
			return ps_pes_video;
		}
		else if ((pes->stream_id == 0xBC) || (pes->stream_id == 0xBD) || (pes->stream_id == 0xBE) || (pes->stream_id == 0xBF))
		{
			return ps_pes_jump;
		}
		else if ((pes->stream_id == 0xF0) || (pes->stream_id == 0xF1) || (pes->stream_id == 0xF2) || (pes->stream_id == 0xF8))
		{
			return ps_pes_jump;
		}
	}
	return ps_padding;
}

//nal单元开头三个字节00 00 01 （Nal单元开始）
//nal单元开头四个字节00 00 00 01 （Nal单元开始）
typedef struct nal_unit_header3
{
	unsigned char start_code[3];
	unsigned char nal_type : 5;
	unsigned char nal_ref_idc : 2;
	unsigned char for_bit : 1;

}nal_unit_header3_t;

typedef struct nal_unit_header4
{
	unsigned char start_code[4];
	unsigned char nal_type : 5;
	unsigned char nal_ref_idc : 2;
	unsigned char for_bit : 1;
}nal_unit_header4_t;

NalType inline is_264_param(char *nal_str)
{
	nal_unit_header3_t *nalUnit3 = (nal_unit_header3_t *)nal_str;
	nal_unit_header4_t *nalUnit4 = (nal_unit_header4_t *)nal_str;

	if (nalUnit3->start_code[0] == 0 && nalUnit3->start_code[1] == 0 && nalUnit3->start_code[2] == 1)
	{
		if (sps_Nal == nalUnit3->nal_type)
		{
			return sps_Nal;
		}
		else if (pps_Nal == nalUnit3->nal_type)
		{
			return pps_Nal;
		}
		else if (sei_Nal == nalUnit3->nal_type)
		{
			return sei_Nal;
		}
		else if (I_Nal == nalUnit3->nal_type)
		{
			return I_Nal;
		}
		else
		{
			return other;
		}

	}
	else if (nalUnit4->start_code[0] == 0 && nalUnit4->start_code[1] == 0 && nalUnit4->start_code[2] == 0 && nalUnit4->start_code[3] == 1)
	{
		if (sps_Nal == nalUnit4->nal_type)
		{
			return sps_Nal;
		}
		else if (pps_Nal == nalUnit4->nal_type)
		{
			return pps_Nal;
		}
		else if (sei_Nal == nalUnit4->nal_type)
		{
			return sei_Nal;
		}
		else if (I_Nal == nalUnit4->nal_type)
		{
			return I_Nal;
		}
		else
		{
			return other;
		}
	}


	return unknow;
}


class HaikangPSparse:public SpliteStreamGetES
{
public:
	HaikangPSparse();
	~HaikangPSparse();

	virtual int InputBufData(char* frame_buf, int frame_len);
	

private:
	bool inPutRTPData(char* pBuffer, unsigned int sz);
	//输出PS完整的一帧
	bool getPSFrame();

	//解析PS帧，并输出Element stream 
	pes_status_data_t pes_payload();

	bool naked_payload();

private:

	HaikangPSparse(const HaikangPSparse& hkPar){}
	HaikangPSparse& operator=(const HaikangPSparse& hkPar){}
	//GetESCallB   m_getEsData;//回调函数

	//用于处理RTP的丢包和错序
	int m_nPsIndicator;//DUANG，当指示器的值达到3时，可以将完整的ps帧取出来，然后将指示器的值减一

	bool m_bFirstIFrame;                        //找到第一个I帧开始输出
	bool m_bIFrame;

	unsigned __int64 m_iIFramePts;
	unsigned __int64 m_iIFrameDts;
	unsigned int m_iTsSigned;

	//ps计数器 调试用
	//FILE *m_testFpVedio;
	//FILE *m_testFpAudio;

	//unsigned int m_iPrePSCnt;
	//unsigned int m_iCurPSCnt;
	//ps计数器 调试用
	
	char *m_dataStore;
	unsigned int m_dataLen;
	PosSign m_posSign[2];

	//用于PS帧的解析变量
	PSStatus      m_status;                     //当前状态
	char          m_pPSBuffer[MAX_PS_LENGTH];   //PS缓冲区
	unsigned int  m_nPSWrtiePos;                //PS写入位置
	unsigned int  m_nPESIndicator;              //PES指针

	char          m_pPESBuffer[MAX_PES_LENGTH]; //PES缓冲区
	unsigned int  m_nPESLength;                 //PES数据长度

	ps_header_t*  m_ps;                         //PS头
	sh_header_t*  m_sh;                         //系统头
	psm_header_t* m_psm;                        //节目流头
	pes_header_t* m_pes;                        //PES头
	pes_header_t* m_nextpes;                    //下一个pes头

	char         m_pESBuffer[MAX_ES_LENGTH];    //裸码流
	unsigned int m_nESLength;                   //裸码流长度

	unsigned char m_AudioType;                   //
	unsigned char m_VedioType;


};


#endif