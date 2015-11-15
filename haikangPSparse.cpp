/*
*
*/

#include "haikangPSparse.h"

HaikangPSparse::HaikangPSparse()
{
	m_bFirstIFrame = false;
	m_bIFrame = false;
	m_nPsIndicator = 0;

	//m_getEsData = getESCB;
	
	m_dataStore = new char[MAX_PS_LENGTH];
	memset(m_dataStore,0,sizeof(m_dataStore));
	m_dataLen = 0;
	for (int i = 0; i < 2; i++){
		m_posSign[i].used = false;
		m_posSign[i].sign = 0;
	}

	m_ps = NULL;
	m_sh = NULL;
	m_psm = NULL;
	m_pes = NULL;

	m_status = ps_padding;
	m_nESLength = m_nPESIndicator = m_nPSWrtiePos = m_nPESLength = 0;

	m_AudioType = -1;                   
	m_VedioType = -1;

	//fopen_s(&m_testFpVedio, "pts-dts-Vedio.dat", "wb");
	//fopen_s(&m_testFpAudio, "pts-dts-Audio.dat", "wb");

	//m_iPrePSCnt = 0;
	//m_iCurPSCnt = 0;

}

HaikangPSparse::~HaikangPSparse()
{
	//�ͷ���Դ
	if (m_dataStore){
		delete[] m_dataStore;
	}

	for (int i = 0; i < 2; i++){
		m_posSign[i].used = false;
		m_posSign[i].sign = 0;
	}
	m_ps = NULL;                         
	m_sh = NULL;                         
	m_psm = NULL;                        
	m_pes = NULL;  

	//fclose(m_testFpVedio);
	//fclose(m_testFpAudio);

	//m_iPrePSCnt = 0;
	//m_iCurPSCnt = 0;
}


int HaikangPSparse::InputBufData(char* frame_buf, int frame_len)
{
	int ret = 0;
	if (inPutRTPData(frame_buf, (unsigned int)frame_len))
		return ret;
	else
		return -1;
}

//������������ݣ��ҵ�������ps������
bool HaikangPSparse::inPutRTPData(char* pBuffer, unsigned int sz)
{
	bool achieved = false;

	if (pBuffer == NULL || sz == 0){
		return false;
	}


	//����00 00 01 BA ��ǣ�����λ�ü�¼�� m_1stSign m_2ndSign �����У�m_2ndSign��Ϊ�㣬�������ps��
	char *data = pBuffer;
	unsigned int length = sz;
	ps_header_t *tempPs = NULL;
	unsigned int mPackIndicator = 0;
	//�������������ܱȽϺķ�ʱ��,
	//��ʱֻ����ǰ����֮һ��λ��
	for (; mPackIndicator<length/3; mPackIndicator++)
	{

		tempPs = (ps_header_t*)(data + mPackIndicator);

		if (is_ps_header(tempPs))
		{
			//����ҵ����λ
			m_nPsIndicator++;
			for (int i = 0; i < 2; i++){
				if (m_posSign[i].used == false)
				{
					m_posSign[i].used = true;
					m_posSign[i].sign = m_dataLen + mPackIndicator;
					break;
				}
			}
			break;
		}
	}

	//�����ݸ��Ƶ�������
	memcpy(m_dataStore + m_dataLen, pBuffer, sz);
	m_dataLen += sz;

	data = NULL;
	tempPs = NULL;

	if (2 == m_nPsIndicator)//�Ѿ���¼����00 00 01 BA �������һ��PS֡
	{
		//::printf("ready to output a complete PS frame\n");

		achieved = getPSFrame();
		return achieved;
	}
	else
	{

		achieved = true;
		return achieved;
	}

}

bool HaikangPSparse::getPSFrame()
{
	bool achieved = false;

	uint32_t tPSFrmLen = 0;//��ʱ��¼m_nPSFrmLen����
	uint16_t tPSCompleted = 0;//��ʱ��¼������ps���Ƿ��������������Ļ���Ҫ����
	m_nPSWrtiePos = 0;

	//ȡ��������ps������
	memcpy(m_pPSBuffer, m_dataStore + m_posSign[0].sign, m_posSign[1].sign - m_posSign[0].sign);
	tPSFrmLen = m_posSign[1].sign - m_posSign[0].sign;
	//���´洢
	memmove(m_dataStore, m_dataStore + m_posSign[1].sign, m_dataLen - m_posSign[1].sign);
	m_dataLen = m_dataLen - m_posSign[1].sign;


	//����״̬���
	m_nPsIndicator--;
	m_posSign[0].sign = 0;

	m_posSign[1].sign = 0;
	m_posSign[1].used = false;


	m_nPSWrtiePos = tPSFrmLen;
	//::printf("PS frame length  is: %d\n", m_nPSWrtiePos);
	//������һ����ps֡�Ľ�������
	//����naked_payload
	achieved = naked_payload();
	return achieved;

}

pes_status_data_t HaikangPSparse::pes_payload()
{
	unsigned int stuff_len = 0;
	//��Ϊ������ps������ͷ�ֽڿ϶���00 00 01 BA
	if (m_status == ps_padding)
	{
		for (; m_nPESIndicator<m_nPSWrtiePos; m_nPESIndicator++)
		{
			m_ps = (ps_header_t*)(m_pPSBuffer + m_nPESIndicator);
			if (is_ps_header(m_ps))
			{
				stuff_len = (unsigned int)m_ps->pack_stuffing_length;
				//m_iPrePSCnt = m_iCurPSCnt;
				//m_iCurPSCnt = ntohl(m_ps->ps_cnt);
				//m_iPrePSCnt++;

				//fprintf(m_testFpAudio, "the ps cnt is %d\n", m_iCurPSCnt);
				//fprintf(m_testFpAudio, "standard ps cnt is %d\n", m_iPrePSCnt);
				//if (m_iCurPSCnt != m_iPrePSCnt)
					//fprintf(m_testFpAudio, "standard ps cnt is %d,and the ps cnt is %d\n", m_iPrePSCnt, m_iCurPSCnt);
				m_status = ps_ps;
				break;
			}
		}
	}

	if (m_status == ps_ps)
	{
		m_nPESIndicator = 14 + stuff_len;
		for (; m_nPESIndicator<m_nPSWrtiePos; m_nPESIndicator++)
		{
			m_sh = (sh_header_t*)(m_pPSBuffer + m_nPESIndicator);
			m_psm = (psm_header_t*)(m_pPSBuffer + m_nPESIndicator);
			m_pes = (pes_header_t*)(m_pPSBuffer + m_nPESIndicator);
			if (is_sh_header(m_sh))
			{
				m_status = ps_sh;
				break;
			}
			else if (is_psm_header(m_psm))
			{
				m_status = ps_psm;

				unsigned mediaNumCnt = 0;
				unsigned short ioneESInfoLen = 0;
				char *esMapLenPtr = (char *)(m_psm) + 10 + ntohs(m_psm->program_stream_info_length);
				char *itempPtr = esMapLenPtr+2;
				unsigned short esMapLen = ntohs(*(unsigned short *)esMapLenPtr);

				ESMapInfo *iesMapInfo = NULL;
					
				do{
					iesMapInfo = (ESMapInfo *)(itempPtr);
					if (0xE0==iesMapInfo->eleStreamId)
					{
						m_VedioType = iesMapInfo->streamType;
					}
					else if (0xC0 == iesMapInfo->eleStreamId)
					{
						m_AudioType = iesMapInfo->streamType;
					}
					ioneESInfoLen = ntohs(iesMapInfo->elsStreamInfoLen);
					
					itempPtr += (4 + ioneESInfoLen);
					esMapLen -= (4 + ioneESInfoLen);

				} while (esMapLen);
				
				break;
			}
			else if (is_pes_header(m_pes))
			{
				m_status = ps_pes;
				break;
			}
		}
	}

	if (m_status == ps_sh)
	{
		for (; m_nPESIndicator<m_nPSWrtiePos; m_nPESIndicator++)
		{
			m_psm = (psm_header_t*)(m_pPSBuffer + m_nPESIndicator);
			m_pes = (pes_header_t*)(m_pPSBuffer + m_nPESIndicator);
			if (is_psm_header(m_psm))
			{
				m_status = ps_psm;//���s_sh״̬
				break;
			}
			if (is_pes_header(m_pes))
			{
				m_status = ps_pes;
				break;
			}
		}
	}
	if (m_status == ps_psm)
	{
		m_nPESIndicator += 4 + ntohs(m_psm->program_stream_map_length);
		for (; m_nPESIndicator<m_nPSWrtiePos; m_nPESIndicator++)
		{
			m_pes = (pes_header_t*)(m_pPSBuffer + m_nPESIndicator);
			if (is_pes_header(m_pes))
			{
				m_status = ps_pes;
				break;
			}
		}
	}
	if (m_status == ps_pes)
	{
		//Ѱ����һ��pes

		unsigned short PES_packet_length = ntohs(m_pes->PES_packet_length);
		//���滹��pes��Ҫ����
		if ((m_nPESIndicator + PES_packet_length + sizeof(pes_header_t)) < m_nPSWrtiePos)
		{
			char* next = (m_pPSBuffer + m_nPESIndicator + sizeof(pes_header_t)+PES_packet_length);
			
			m_nPESLength = PES_packet_length + sizeof(pes_header_t);
			memcpy(m_pPESBuffer, m_pes, m_nPESLength);
			PSStatus tempPSSt = pes_type(m_pes);
			if ((ps_pes_video == tempPSSt) || (ps_pes_audio == tempPSSt) || (ps_pes_jump == tempPSSt))
			{

				int remain = m_nPSWrtiePos - (next - m_pPSBuffer);
				memcpy(m_pPSBuffer, next, remain);
				m_nPSWrtiePos = remain; m_nPESIndicator = 0;
				m_pes = (pes_header_t*)m_pPSBuffer;
				m_nextpes = m_pes;

				pes_status_data_t pesStatusTemp;

				pesStatusTemp.pesTrued = true;
				pesStatusTemp.pesType = tempPSSt;
				pesStatusTemp.pesPtr = (pes_header_t*)m_pPESBuffer;

				return pesStatusTemp;
			}

		}
		else if ((m_nPESIndicator + PES_packet_length + sizeof(pes_header_t)) == m_nPSWrtiePos)
		{

			m_nPESLength = PES_packet_length + sizeof(pes_header_t);
			memcpy(m_pPESBuffer, m_pes, m_nPESLength);
			//����m_nPESIndicator��m_nPSWrtiePos״̬
			m_nPESIndicator = m_nPSWrtiePos = 0;
			m_status = ps_padding;
			m_nextpes = NULL;

			pes_status_data_t pesStatusTemp;

			pesStatusTemp.pesTrued = true;
			pesStatusTemp.pesType = ps_pes_last;
			pesStatusTemp.pesPtr = (pes_header_t*)m_pPESBuffer;

			return pesStatusTemp;

		}
		else
		{
			m_nPESIndicator = m_nPSWrtiePos = 0;
			m_status = ps_padding;
			printf("ERROR�� never go here unless it lost some data, the ps packet should be discarded\n");
		}
	}

	pes_status_data_t pesStatusTemp;

	pesStatusTemp.pesTrued = false;
	pesStatusTemp.pesType = ps_pes_lost;
	pesStatusTemp.pesPtr = NULL;

	return pesStatusTemp;

}

bool HaikangPSparse::naked_payload()
{
	bool achieved = false;
	do
	{

		pes_status_data_t t = pes_payload();
		if (!t.pesTrued)
		{
			break;
		}
		PSStatus status = t.pesType;

		pes_header_t* pes = t.pesPtr;
		optional_pes_header* option = (optional_pes_header*)((char*)pes + sizeof(pes_header_t));
		if (option->PTS_DTS_flags != 2 && option->PTS_DTS_flags != 3 && option->PTS_DTS_flags != 0)
		{
			::fprintf(stderr, "PTS_DTS_flags is 01 which is invalid PTS_DTS_flags\n");
			break;
		}
		unsigned int pts__signed = (unsigned int)option->PTS_DTS_flags;
		unsigned __int64 pts = get_pts(option);
		unsigned __int64 dts = get_dts(option);

		unsigned char stream_id = pes->stream_id;
		unsigned short PES_packet_length = ntohs(pes->PES_packet_length);
		char* pESBuffer = ((char*)option + sizeof(optional_pes_header)+option->PES_header_data_length);
		int nESLength = PES_packet_length - (sizeof(optional_pes_header)+option->PES_header_data_length);
		//delete the sei information
		//if ((stream_id == 0xE0) && ((sei_Nal == is_264_param(pESBuffer))))
		//	continue;

		memcpy(m_pESBuffer + m_nESLength, pESBuffer, nESLength);
		m_nESLength += nESLength;

		// sps��pps��sei��I֡��Ϊһ����Ԫ���
		if ((stream_id == 0xE0) && ((sps_Nal == is_264_param(pESBuffer))))
		{
			m_bFirstIFrame = true;
			m_iTsSigned = pts__signed;
			m_iIFramePts = pts;
			m_iIFrameDts = dts;
			achieved = true;
		}
		else if ((stream_id == 0xE0) && (pps_Nal == is_264_param(pESBuffer)))
		{
			achieved = true;
		}
		/*
		else if ((stream_id == 0xE0) && (sei_Nal == is_264_param(pESBuffer)))
		{
			achieved = true;
		}*/
		else if (stream_id == 0xE0 && status == pes_type(m_nextpes))
		{
			if (I_Nal == is_264_param(pESBuffer))
			{
				m_bIFrame = true;
			}
			else
			{
				if (!m_bIFrame)
				{
					if (0 == m_iTsSigned && 0 == m_iIFramePts && 0 == m_iIFrameDts)
					{
						m_iTsSigned = pts__signed;
						m_iIFramePts = pts;
						m_iIFrameDts = dts;
					}
				}

			}
			achieved = true;
		}
		else if (stream_id == 0xE0 && status != pes_type(m_nextpes))
		{
			//�˴����ûص������ݷ���
			SpliteStreamESInfo mInfo;
			mInfo.mType = SpliteStreamGetES_MediaType_VEDIOTYPE;
			if (m_bIFrame)
				mInfo.fType = SpliteStreamGetES_FrameType_IFRAME;
			else
				mInfo.fType = SpliteStreamGetES_FrameType_PBFRAME;

			if (0 == m_iTsSigned && 0 == m_iIFramePts && 0 == m_iIFrameDts)
			{
				m_iTsSigned = pts__signed;
				m_iIFramePts = pts;
				m_iIFrameDts = dts;
			}
			//�ص������������

			if (m_bFirstIFrame)
			{
				if (m_bIFrame)
				{
					getESCB(m_pESBuffer, m_nESLength, mInfo, m_iIFramePts, getp);
					//fprintf(m_testFpVedio, "vedioI PTS_DTS_flags = %d\tpts = %I64u\tdts = %I64u\n", m_iTsSigned, m_iIFramePts, m_iIFrameDts);
				}
				else
				{
					getESCB(m_pESBuffer, m_nESLength, mInfo, m_iIFramePts, getp);
					//fprintf(m_testFpVedio, "vedio PTS_DTS_flags = %d\tpts = %I64u\tdts = %I64u\n", m_iTsSigned, m_iIFramePts, m_iIFrameDts);
				}


			}
			m_iTsSigned = 0;
			m_iIFramePts = 0;
			m_iIFrameDts = 0;
			m_bIFrame = false;
			m_nESLength = 0;
		}
		else if (stream_id == 0xC0)
		{
			//�˴����ûص������ݷ���
			SpliteStreamESInfo mInfo;
			mInfo.mType = SpliteStreamGetES_MediaType_AUDIOTYPE;
			mInfo.fType = SpliteStreamGetES_FrameType_NONEF;

			//�ص������������
			if (m_bFirstIFrame)
			{
				getESCB(m_pESBuffer, m_nESLength, mInfo, pts, getp);
				//fprintf(m_testFpAudio, "audio PTS_DTS_flags = %d\tpts = %I64u\tdts = %I64u\n", pts__signed, pts, dts);
			}
			m_nESLength = 0;

			achieved = true;
		}
		else
		{
			//fprintf(stderr, "the pes packet is some private data without parsing\n");
			m_nESLength = 0;
		}

		//����Ǵ�ps�������һ��pes�������˳�ѭ��
		if (ps_pes_last == status)
		{
			break;
		}


	} while (true);

	return achieved;

}


