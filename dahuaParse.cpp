
#include "dahuaParse.h"

//设置大华的解析结构体

DahuaParse::DahuaParse()
{
	m_menUnit = new Unit_t;
	m_menUnit->buf = new char[MAX_BUF_SIZE];
	memset(m_menUnit->buf, 0, MAX_BUF_SIZE);
	m_menUnit->cur_len = 0;

	m_iSufFrameLen = 0;
	m_iPreFrameLen = 0;
	m_bComplete = false;
	m_bFirstIFrame = false;

	m_iVedioFstCnt = 0;
	m_iVedioCurCnt = 0;

	m_ivFrameRate = 0;
	m_ivFrameRateInc = 0;

	m_pIFrameContent = NULL;
	m_iIFrameLen = 0;

}

DahuaParse::~DahuaParse()
{
	if (m_menUnit->buf != NULL){
		delete[] m_menUnit->buf;
		m_menUnit->buf = NULL;
	}
	if (m_menUnit != NULL){
		delete m_menUnit;
		m_menUnit = NULL;
	}
	if (m_pIFrameContent){
		delete[] m_pIFrameContent;
		m_pIFrameContent = NULL;
	}


}
int DahuaParse::InputBufData(char* frame_buf, int frame_len)
{
	//需要有返回类型，需要根据接口设置
	//int res = 0;

	//首先判断缓冲的类型，音频，视频
	//目前音频是G711A
	if ((frame_buf[0] == 0x44) && (frame_buf[1] == 0x48) && (frame_buf[2] == 0x41) && (frame_buf[3] == 0x56)&&(0xF0 == (unsigned char)frame_buf[4]))
	{

		m_menUnit->cur_len = frame_len - (DAHUA_AUDIO_HEADSIZE + DAHUA_AUDIO_TAILSIZE);
		memcpy(m_menUnit->buf, frame_buf + DAHUA_AUDIO_HEADSIZE, m_menUnit->cur_len);
		
		m_iPreFrameLen = (((unsigned char)frame_buf[15] << 24)) | (((unsigned char)frame_buf[14] << 16)) | (((unsigned char)frame_buf[13] << 8)) | ((unsigned char)frame_buf[12]);

		if ((frame_buf[frame_len - 8] == 0x64) && (frame_buf[frame_len - 7] == 0x68) && (frame_buf[frame_len - 6] == 0x61) && (frame_buf[frame_len - 5] == 0x76))
		{
			m_iSufFrameLen = (((unsigned char)frame_buf[frame_len - 1] << 24)) | (((unsigned char)frame_buf[frame_len - 2] << 16)) | (((unsigned char)frame_buf[frame_len - 3] << 8)) | (unsigned char(frame_buf[frame_len - 4]));
		}

		if ((m_iSufFrameLen == m_iPreFrameLen) && (m_iSufFrameLen == (unsigned int)frame_len))
		{
			m_bComplete = true;
		}

		if (m_bComplete)
		{

			SpliteStreamESInfo mInfo;
			mInfo.mType = SpliteStreamGetES_MediaType_AUDIOTYPE;
			mInfo.fType = SpliteStreamGetES_FrameType_NONEF;
			getESCB(m_menUnit->buf, m_menUnit->cur_len, mInfo, 0, getp);

			m_iSufFrameLen = 0;
			m_iPreFrameLen = 0;
			m_bComplete = false;

			//清理存储相关
			memset(m_menUnit->buf, 0, MAX_BUF_SIZE);
			m_menUnit->cur_len = 0;

			return 0;
		}
		else
		{
		
			m_iSufFrameLen = 0;
			m_iPreFrameLen = 0;
			m_bComplete = false;

			//清理存储相关
			memset(m_menUnit->buf, 0, MAX_BUF_SIZE);
			m_menUnit->cur_len = 0;

			fprintf(stderr, "the audio frame is incomplete,we cant cope with it!\n");

			return 0;
		}
	
	}

	else if ((frame_buf[0] == 0x44) && (frame_buf[1] == 0x48) && (frame_buf[2] == 0x41) && (frame_buf[3] == 0x56) && (0xF1 == (unsigned char)frame_buf[4]))
	{//大华私有参数，未知
#if DEBUG
		printf("the frame buf is give up,and go on to use function InputBufData\n");
#endif
		return 0;
	}

	else if ((frame_buf[0] == 0x44) && (frame_buf[1] == 0x48) && (frame_buf[2] == 0x41) && (frame_buf[3] == 0x56) && ((0xFD == (unsigned char)frame_buf[4]) || (0xFC == (unsigned char)frame_buf[4])))
	{

		if (m_menUnit->cur_len != 0)
		{
			fprintf(stderr, "last vedio frame incomplete, we select to drop it!\n");
			memset(m_menUnit->buf, 0, MAX_BUF_SIZE);
			m_menUnit->cur_len = 0;
		}

		m_iPreFrameLen = (((unsigned char)frame_buf[15] << 24)) | (((unsigned char)frame_buf[14] << 16)) | (((unsigned char)frame_buf[13] << 8)) | ((unsigned char)frame_buf[12]);

		if ((frame_buf[frame_len - 8] == 0x64) && (frame_buf[frame_len - 7] == 0x68) && (frame_buf[frame_len - 6] == 0x61) && (frame_buf[frame_len - 5] == 0x76))
		{
			m_iSufFrameLen = (((unsigned char)frame_buf[frame_len - 1] << 24)) | (((unsigned char)frame_buf[frame_len - 2] << 16)) | (((unsigned char)frame_buf[frame_len - 3] << 8)) | (unsigned char(frame_buf[frame_len - 4]));
		}
		//记录下大华的帧计数值，便于产生pts
		m_iVedioCurCnt = (((unsigned char)frame_buf[11] << 24)) | (((unsigned char)frame_buf[10] << 16)) | (((unsigned char)frame_buf[9] << 8)) | ((unsigned char)frame_buf[8]);

		if (m_iVedioFstCnt == 0)
		{
			//记录下第一个视频帧的计数
			m_iVedioFstCnt = (((unsigned char)frame_buf[11] << 24)) | (((unsigned char)frame_buf[10] << 16)) | (((unsigned char)frame_buf[9] << 8)) | ((unsigned char)frame_buf[8]);
			//获取帧率，设置pts的增量
			m_ivFrameRate = (unsigned char)frame_buf[31];
			m_ivFrameRateInc = unsigned int(VEDIO_CLOCK / (unsigned char)frame_buf[31]);
			//printf("the m_ivFrameRateInc = %d\n", m_ivFrameRateInc);
		}

		if ((m_iSufFrameLen == m_iPreFrameLen) && (m_iSufFrameLen == (unsigned int)frame_len))
		{
			m_bComplete = true;
		}

		if (m_bComplete)
		{
			if (0xFC == (unsigned char)frame_buf[4])
			{
				m_menUnit->cur_len = frame_len - (DAHUA_VIDEO_PB_HEADSIZE + DAHUA_VIDEO_PB_TAILSIZE);
				memcpy(m_menUnit->buf, frame_buf + DAHUA_VIDEO_PB_HEADSIZE, m_menUnit->cur_len);
			}
			else if (0xFD == (unsigned char)frame_buf[4])
			{
				m_menUnit->cur_len = frame_len - (DAHUA_VIDEO_I_HEADSIZE + DAHUA_VIDEO_PB_TAILSIZE);
				memcpy(m_menUnit->buf, frame_buf + DAHUA_VIDEO_I_HEADSIZE, m_menUnit->cur_len);
			}


			SpliteStreamESInfo mInfo;
			mInfo.mType = SpliteStreamGetES_MediaType_VEDIOTYPE;
			if (0xFC == (unsigned char)frame_buf[4])
				mInfo.fType = SpliteStreamGetES_FrameType_PBFRAME;
			else if (0xFD == (unsigned char)frame_buf[4])
				mInfo.fType = SpliteStreamGetES_FrameType_IFRAME;

			//调用回调输出数据
			if (mInfo.fType == SpliteStreamGetES_FrameType_IFRAME)
			{
				m_bFirstIFrame = true;
			}

			if (m_bFirstIFrame == true)
			{
				//产生pts，产生的规则是，根据帧率和帧数
				//容易产生的问题是m_iVedioCurCnt如果发生溢出，使m_iVedioCurCnt为0，容易发生问题。
				unsigned __int64 lTS = (m_iVedioCurCnt - m_iVedioFstCnt)*(unsigned __int64)m_ivFrameRateInc;
				//保存下第一个I帧
				if (m_iIFrameLen == 0 && !m_pIFrameContent)
				{
					m_iIFrameLen = m_menUnit->cur_len;
					m_pIFrameContent = new char[m_iIFrameLen];

					memcpy(m_pIFrameContent, m_menUnit->buf, m_iIFrameLen);
				}

				getESCB(m_menUnit->buf, m_menUnit->cur_len, mInfo, lTS, getp);
			}
			
			//清理存储相关
			m_iSufFrameLen = 0;
			m_iPreFrameLen = 0;
			m_bComplete = false;

			memset(m_menUnit->buf, 0, MAX_BUF_SIZE);
			m_menUnit->cur_len = 0;

			return 0;
		}
		else
		{
			//视频缓冲只有开头标志，没有结尾标志，这是sps、pps、I帧缓冲的第一段缓冲
			if (0xFD == (unsigned char)frame_buf[4])
			{
				m_menUnit->cur_len = frame_len - DAHUA_VIDEO_I_HEADSIZE;
				memcpy(m_menUnit->buf, frame_buf + DAHUA_VIDEO_I_HEADSIZE, m_menUnit->cur_len); //modify
			}
			else if (0xFC == (unsigned char)frame_buf[4])
			{
				m_menUnit->cur_len = frame_len - DAHUA_VIDEO_PB_HEADSIZE;
				memcpy(m_menUnit->buf, frame_buf + DAHUA_VIDEO_PB_HEADSIZE, m_menUnit->cur_len); //modify
			}	
		}
		return 0;
	}

	else if ((frame_buf[frame_len - 8] == 0x64) && (frame_buf[frame_len - 7] == 0x68) && (frame_buf[frame_len - 6] == 0x61) && (frame_buf[frame_len - 5] == 0x76))
	{
		//在视频缓冲加到I帧缓冲上
		memcpy(m_menUnit->buf + m_menUnit->cur_len, frame_buf, frame_len - DAHUA_VIDEO_I_TAILSIZE);
		m_menUnit->cur_len += (frame_len - DAHUA_VIDEO_I_TAILSIZE);

		m_iSufFrameLen = (((unsigned char)frame_buf[frame_len - 1] << 24)) | (((unsigned char)frame_buf[frame_len - 2] << 16)) | (((unsigned char)frame_buf[frame_len - 3] << 8)) | (unsigned char(frame_buf[frame_len - 4]));


		if (0x67 == (unsigned char)m_menUnit->buf[4])
		{
			if ((m_iSufFrameLen == m_iPreFrameLen) && (m_iSufFrameLen == (m_menUnit->cur_len + DAHUA_VIDEO_I_HEADSIZE + DAHUA_VIDEO_I_TAILSIZE)))
			{
				m_bComplete = true;
			}
		}
		else
		{
			if ((m_iSufFrameLen == m_iPreFrameLen) && (m_iSufFrameLen == (m_menUnit->cur_len + DAHUA_VIDEO_PB_HEADSIZE + DAHUA_VIDEO_I_TAILSIZE)))
			{
				m_bComplete = true;
			}
		}


		if (m_bComplete)
		{
			//组成完整的I帧，可以进行输出
			SpliteStreamESInfo mInfo;
			if (0x67 == (unsigned char)m_menUnit->buf[4])
			{
				mInfo.fType = SpliteStreamGetES_FrameType_IFRAME;
			}
			else 
			{
				mInfo.fType = SpliteStreamGetES_FrameType_PBFRAME;
			}
			mInfo.mType = SpliteStreamGetES_MediaType_VEDIOTYPE;

			//调用回调输出数据
			//调用回调输出数据
			if (mInfo.fType == SpliteStreamGetES_FrameType_IFRAME)
			{
				m_bFirstIFrame = true;
			}

			if (m_bFirstIFrame == true)
			{
				//产生pts，产生的规则是，根据帧率和帧数
				//容易产生的问题是m_iVedioCurCnt如果发生溢出，使m_iVedioCurCnt为0，容易发生问题。
				unsigned __int64 lTS = (m_iVedioCurCnt - m_iVedioFstCnt)*(unsigned __int64)m_ivFrameRateInc;
				//保存下第一个I帧
				if (m_iIFrameLen == 0 && !m_pIFrameContent)
				{
					m_iIFrameLen = m_menUnit->cur_len;
					m_pIFrameContent = new char[m_iIFrameLen];

					memcpy(m_pIFrameContent, m_menUnit->buf, m_iIFrameLen);
				}
				getESCB(m_menUnit->buf, m_menUnit->cur_len, mInfo, lTS, getp);
			}
			//清理存储相关
			m_iSufFrameLen = 0;
			m_iPreFrameLen = 0;
			m_bComplete = false;

			memset(m_menUnit->buf, 0, MAX_BUF_SIZE);
			m_menUnit->cur_len = 0;
			
			return 0;
		}
		else
		{
			fprintf(stderr,"the frame is incomplete, some date lost!\n");
			//清理存储相关
			m_iSufFrameLen = 0;
			m_iPreFrameLen = 0;
			m_bComplete = false;

			memset(m_menUnit->buf, 0, MAX_BUF_SIZE);
			m_menUnit->cur_len = 0;
			return 0;
		}
	
	}

	else
	{
		//视频帧头丢了
		if (!m_menUnit->buf || m_menUnit->cur_len <= 0)
		{
			fprintf(stderr,"the frame is incomplete without frame header!\n");
			//清理存储相关
			m_iSufFrameLen = 0;
			m_iPreFrameLen = 0;
			m_bComplete = false;
			memset(m_menUnit->buf, 0, MAX_BUF_SIZE);
			m_menUnit->cur_len = 0;

			return 0;
		}
		else
		{
			//视频缓冲没有开头结尾
			//在缓冲后追加数据
			memcpy(m_menUnit->buf + m_menUnit->cur_len, frame_buf, frame_len);
			m_menUnit->cur_len += frame_len;
			return 0;
		}
		
	}

}

//typedef bool Boolean;
//#define False false
//#define True true

static const char base64Char[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char* strDup(char const* str)
{
	if (str == NULL)
		return NULL;
	size_t len = strlen(str) + 1;
	char* copy = new char[len];
	if (copy != NULL) {
		memcpy(copy, str, len);
	}
	return copy;
}

char* strDupSize(char const* str, size_t& resultBufSize)
{
	if (str == NULL) {
		resultBufSize = 0;
		return NULL;
	}
	resultBufSize = strlen(str) + 1;
	char* copy = new char[resultBufSize];
	return copy;
}

char* strDupSize(char const* str)
{
	size_t dummy;
	return strDupSize(str, dummy);
}


static char base64DecodeTable[256];

static void initBase64DecodeTable()
{
	int i;
	for (i = 0; i < 256; ++i) base64DecodeTable[i] = (char)0x80;
	// default value: invalid

	for (i = 'A'; i <= 'Z'; ++i) base64DecodeTable[i] = 0 + (i - 'A');
	for (i = 'a'; i <= 'z'; ++i) base64DecodeTable[i] = 26 + (i - 'a');
	for (i = '0'; i <= '9'; ++i) base64DecodeTable[i] = 52 + (i - '0');
	base64DecodeTable[(unsigned char)'+'] = 62;
	base64DecodeTable[(unsigned char)'/'] = 63;
	base64DecodeTable[(unsigned char)'='] = 0;
}


unsigned char* base64Decode(char const* in, unsigned int inSize, unsigned int& resultSize, bool trimTrailingZeros)
{
	static bool haveInitializedBase64DecodeTable = false;
	if (!haveInitializedBase64DecodeTable)
	{
		initBase64DecodeTable();
		haveInitializedBase64DecodeTable = true;
	}

	unsigned char* out = (unsigned char*)strDupSize(in); // ensures we have enough space
	int k = 0;
	int paddingCount = 0;
	int const jMax = inSize - 3;
	// in case "inSize" is not a multiple of 4 (although it should be)
	for (int j = 0; j < jMax; j += 4)
	{
		char inTmp[4], outTmp[4];
		for (int i = 0; i < 4; ++i)
		{
			inTmp[i] = in[i + j];
			if (inTmp[i] == '=') ++paddingCount;
			outTmp[i] = base64DecodeTable[(unsigned char)inTmp[i]];
			if ((outTmp[i] & 0x80) != 0)
				outTmp[i] = 0; // this happens only if there was an invalid character; pretend that it was 'A'
		}

		out[k++] = (outTmp[0] << 2) | (outTmp[1] >> 4);
		out[k++] = (outTmp[1] << 4) | (outTmp[2] >> 2);
		out[k++] = (outTmp[2] << 6) | outTmp[3];
	}

	if (trimTrailingZeros)
	{
		while (paddingCount > 0 && k > 0 && out[k - 1] == '\0')
		{
			--k;
			--paddingCount;
		}
	}
	resultSize = k;
	unsigned char* result = new unsigned char[resultSize];
	memmove(result, out, resultSize);
	delete[] out;

	return result;
}

unsigned char* base64Decode(char const* in, unsigned int& resultSize, bool trimTrailingZeros)
{
	if (in == NULL) return NULL; // sanity check
	return base64Decode(in, strlen(in), resultSize, trimTrailingZeros);
}

char* base64Encode(char const* origSigned, unsigned origLength)
{
	unsigned char const* orig = (unsigned char const*)origSigned; // in case any input bytes have the MSB set
	if (orig == NULL)
		return NULL;

	unsigned const numOrig24BitValues = origLength / 3;
	bool havePadding = origLength > numOrig24BitValues * 3;
	bool havePadding2 = origLength == numOrig24BitValues * 3 + 2;
	unsigned const numResultBytes = 4 * (numOrig24BitValues + havePadding);
	char* result = new char[numResultBytes + 1]; // allow for trailing '\0'

	// Map each full group of 3 input bytes into 4 output base-64 characters:
	unsigned i;
	for (i = 0; i < numOrig24BitValues; ++i)
	{
		result[4 * i + 0] = base64Char[(orig[3 * i] >> 2) & 0x3F];
		result[4 * i + 1] = base64Char[(((orig[3 * i] & 0x3) << 4) | (orig[3 * i + 1] >> 4)) & 0x3F];
		result[4 * i + 2] = base64Char[((orig[3 * i + 1] << 2) | (orig[3 * i + 2] >> 6)) & 0x3F];
		result[4 * i + 3] = base64Char[orig[3 * i + 2] & 0x3F];
	}

	// Now, take padding into account.  (Note: i == numOrig24BitValues)
	if (havePadding)
	{
		result[4 * i + 0] = base64Char[(orig[3 * i] >> 2) & 0x3F];
		if (havePadding2)
		{
			result[4 * i + 1] = base64Char[(((orig[3 * i] & 0x3) << 4) | (orig[3 * i + 1] >> 4)) & 0x3F];
			result[4 * i + 2] = base64Char[(orig[3 * i + 1] << 2) & 0x3F];
		}
		else
		{
			result[4 * i + 1] = base64Char[((orig[3 * i] & 0x3) << 4) & 0x3F];
			result[4 * i + 2] = '=';
		}
		result[4 * i + 3] = '=';
	}

	result[numResultBytes] = '\0';
	return result;
}


#define fHNumber 264
#define SPS_MAX_SIZE 1000

#if defined(_DEBUG)
#define DEBUG_PRINT(x) do {printf(#x); printf("=    %d\n",x);} while (0)
#else
#define DEBUG_PRINT(x) do {} while (0)
#endif

// Note: the "x=x;" statement is intended to eliminate "unused variable" compiler warning messages
#define DEBUG_STR(x) do {} while (0)
#define DEBUG_TAB do {} while (0)

class BitVector {
public:
	BitVector(unsigned char* baseBytePtr,
		unsigned baseBitOffset,
		unsigned totNumBits);

	void setup(unsigned char* baseBytePtr,
		unsigned baseBitOffset,
		unsigned totNumBits);

	void putBits(unsigned from, unsigned numBits); // "numBits" <= 32
	void put1Bit(unsigned bit);

	unsigned getBits(unsigned numBits); // "numBits" <= 32
	unsigned get1Bit();
	bool get1BitBoolean() { return get1Bit() != 0; }

	void skipBits(unsigned numBits);

	unsigned curBitIndex() const { return fCurBitIndex; }
	unsigned totNumBits() const { return fTotNumBits; }
	unsigned numBitsRemaining() const { return fTotNumBits - fCurBitIndex; }

	unsigned get_expGolomb();
	// Returns the value of the next bits, assuming that they were encoded using an exponential-Golomb code of order 0

private:
	unsigned char* fBaseBytePtr;
	unsigned fBaseBitOffset;
	unsigned fTotNumBits;
	unsigned fCurBitIndex;
};

BitVector::BitVector(unsigned char* baseBytePtr,
	unsigned baseBitOffset,
	unsigned totNumBits) {
	setup(baseBytePtr, baseBitOffset, totNumBits);
}

void BitVector::setup(unsigned char* baseBytePtr,
	unsigned baseBitOffset,
	unsigned totNumBits) {
	fBaseBytePtr = baseBytePtr;
	fBaseBitOffset = baseBitOffset;
	fTotNumBits = totNumBits;
	fCurBitIndex = 0;
}

static unsigned char const singleBitMask[8]
= { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

#define MAX_LENGTH 32

void shiftBits(unsigned char* toBasePtr, unsigned toBitOffset,
	unsigned char const* fromBasePtr, unsigned fromBitOffset,
	unsigned numBits) {
	if (numBits == 0) return;

	/* Note that from and to may overlap, if from>to */
	unsigned char const* fromBytePtr = fromBasePtr + fromBitOffset / 8;
	unsigned fromBitRem = fromBitOffset % 8;
	unsigned char* toBytePtr = toBasePtr + toBitOffset / 8;
	unsigned toBitRem = toBitOffset % 8;

	while (numBits-- > 0) {
		unsigned char fromBitMask = singleBitMask[fromBitRem];
		unsigned char fromBit = (*fromBytePtr)&fromBitMask;
		unsigned char toBitMask = singleBitMask[toBitRem];

		if (fromBit != 0) {
			*toBytePtr |= toBitMask;
		}
		else {
			*toBytePtr &= ~toBitMask;
		}

		if (++fromBitRem == 8) {
			++fromBytePtr;
			fromBitRem = 0;
		}
		if (++toBitRem == 8) {
			++toBytePtr;
			toBitRem = 0;
		}
	}
}

void BitVector::putBits(unsigned from, unsigned numBits) {
	if (numBits == 0) return;

	unsigned char tmpBuf[4];
	unsigned overflowingBits = 0;

	if (numBits > MAX_LENGTH) {
		numBits = MAX_LENGTH;
	}

	if (numBits > fTotNumBits - fCurBitIndex) {
		overflowingBits = numBits - (fTotNumBits - fCurBitIndex);
	}

	tmpBuf[0] = (unsigned char)(from >> 24);
	tmpBuf[1] = (unsigned char)(from >> 16);
	tmpBuf[2] = (unsigned char)(from >> 8);
	tmpBuf[3] = (unsigned char)from;

	shiftBits(fBaseBytePtr, fBaseBitOffset + fCurBitIndex, /* to */
		tmpBuf, MAX_LENGTH - numBits, /* from */
		numBits - overflowingBits /* num bits */);
	fCurBitIndex += numBits - overflowingBits;
}

void BitVector::put1Bit(unsigned bit) {
	// The following is equivalent to "putBits(..., 1)", except faster:
	if (fCurBitIndex >= fTotNumBits) { /* overflow */
		return;
	}
	else {
		unsigned totBitOffset = fBaseBitOffset + fCurBitIndex++;
		unsigned char mask = singleBitMask[totBitOffset % 8];
		if (bit) {
			fBaseBytePtr[totBitOffset / 8] |= mask;
		}
		else {
			fBaseBytePtr[totBitOffset / 8] &= ~mask;
		}
	}
}

unsigned BitVector::getBits(unsigned numBits) {
	if (numBits == 0) return 0;

	unsigned char tmpBuf[4] = { 0 };
	unsigned overflowingBits = 0;

	if (numBits > MAX_LENGTH) {
		numBits = MAX_LENGTH;
	}

	if (numBits > fTotNumBits - fCurBitIndex) {
		overflowingBits = numBits - (fTotNumBits - fCurBitIndex);
	}

	shiftBits(tmpBuf, 0, /* to */
		fBaseBytePtr, fBaseBitOffset + fCurBitIndex, /* from */
		numBits - overflowingBits /* num bits */);
	fCurBitIndex += numBits - overflowingBits;

	if (numBits == MAX_LENGTH){
		printf("tmpBuf  =  %d\n", tmpBuf[0]);
		printf("tmpBuf  =  %d\n", tmpBuf[1]);
		printf("tmpBuf  =  %d\n", tmpBuf[2]);
		printf("tmpBuf  =  %d\n", tmpBuf[3]);
	}
	unsigned result
		= (tmpBuf[0] << 24) | (tmpBuf[1] << 16) | (tmpBuf[2] << 8) | tmpBuf[3];
	//printf("result =                  %d\n",result);
	result >>= (MAX_LENGTH - numBits); // move into low-order part of word
	result &= (0xFFFFFFFF << overflowingBits); // so any overflow bits are 0
	return result;
}

unsigned BitVector::get1Bit() {
	// The following is equivalent to "getBits(1)", except faster:

	if (fCurBitIndex >= fTotNumBits) { /* overflow */
		return 0;
	}
	else {
		unsigned totBitOffset = fBaseBitOffset + fCurBitIndex++;
		unsigned char curFromByte = fBaseBytePtr[totBitOffset / 8];
		unsigned result = (curFromByte >> (7 - (totBitOffset % 8))) & 0x01;
		return result;
	}
}

void BitVector::skipBits(unsigned numBits) {
	if (numBits > fTotNumBits - fCurBitIndex) { /* overflow */
		fCurBitIndex = fTotNumBits;
	}
	else {
		fCurBitIndex += numBits;
	}
}

unsigned BitVector::get_expGolomb() {
	unsigned numLeadingZeroBits = 0;
	unsigned codeStart = 1;

	while (get1Bit() == 0 && fCurBitIndex < fTotNumBits) {
		++numLeadingZeroBits;
		codeStart *= 2;
	}

	return codeStart - 1 + getBits(numLeadingZeroBits);
}

unsigned removeH264or5EmulationBytes(unsigned char* to, unsigned toMaxSize,
	unsigned char* from, unsigned fromSize) {
	unsigned toSize = 0;
	unsigned i = 0;
	while (i < fromSize && toSize + 1 < toMaxSize) {
		if (i + 2 < fromSize && from[i] == 0 && from[i + 1] == 0 && from[i + 2] == 3) {
			to[toSize] = to[toSize + 1] = 0;
			toSize += 2;
			i += 3;
		}
		else {
			to[toSize] = from[i];
			toSize += 1;
			i += 1;
		}
	}

	return toSize;
}

void removeEmulationBytes(unsigned char* nalUnitCopy, unsigned maxSize, unsigned& nalUnitCopySize, unsigned char* spsNal, unsigned spsNalSize) {
	unsigned char* nalUnitOrig = spsNal;
	unsigned const numBytesInNALunit = spsNalSize;
	nalUnitCopySize
		= removeH264or5EmulationBytes(nalUnitCopy, maxSize, nalUnitOrig, numBytesInNALunit);
}


typedef struct stSpsPara
{
	unsigned int nWidth;
	unsigned int nHeight;
	unsigned int num_units_in_tick;
	unsigned int time_scale;
	float nFrameRate;
}stSpsPara;

void analyze_vui_parameters(BitVector& bv,
	unsigned& num_units_in_tick, unsigned& time_scale) {
	bool aspect_ratio_info_present_flag = bv.get1BitBoolean();
	DEBUG_PRINT(aspect_ratio_info_present_flag);
	if (aspect_ratio_info_present_flag) {
		DEBUG_TAB;
		unsigned aspect_ratio_idc = bv.getBits(8);
		DEBUG_PRINT(aspect_ratio_idc);
		if (aspect_ratio_idc == 255/*Extended_SAR*/) {
			bv.skipBits(32); // sar_width; sar_height
		}
	}
	bool overscan_info_present_flag = bv.get1BitBoolean();
	DEBUG_PRINT(overscan_info_present_flag);
	if (overscan_info_present_flag) {
		bv.skipBits(1); // overscan_appropriate_flag
	}
	bool video_signal_type_present_flag = bv.get1BitBoolean();
	DEBUG_PRINT(video_signal_type_present_flag);
	if (video_signal_type_present_flag) {
		DEBUG_TAB;
		bv.skipBits(4); // video_format; video_full_range_flag
		bool colour_description_present_flag = bv.get1BitBoolean();
		DEBUG_PRINT(colour_description_present_flag);
		if (colour_description_present_flag) {
			bv.skipBits(24); // colour_primaries; transfer_characteristics; matrix_coefficients
		}
	}
	bool chroma_loc_info_present_flag = bv.get1BitBoolean();
	DEBUG_PRINT(chroma_loc_info_present_flag);
	if (chroma_loc_info_present_flag) {
		(void)bv.get_expGolomb(); // chroma_sample_loc_type_top_field
		(void)bv.get_expGolomb(); // chroma_sample_loc_type_bottom_field
	}
	if (fHNumber == 265) {
		bv.skipBits(3); // neutral_chroma_indication_flag, field_seq_flag, frame_field_info_present_flag
		bool default_display_window_flag = bv.get1BitBoolean();
		DEBUG_PRINT(default_display_window_flag);
		if (default_display_window_flag) {
			(void)bv.get_expGolomb(); // def_disp_win_left_offset
			(void)bv.get_expGolomb(); // def_disp_win_right_offset
			(void)bv.get_expGolomb(); // def_disp_win_top_offset
			(void)bv.get_expGolomb(); // def_disp_win_bottom_offset
		}
	}
	bool timing_info_present_flag = bv.get1BitBoolean();
	DEBUG_PRINT(timing_info_present_flag);
	if (timing_info_present_flag) {
		DEBUG_TAB;
		num_units_in_tick = bv.getBits(32);
		DEBUG_PRINT(num_units_in_tick);
		time_scale = bv.getBits(32);
		DEBUG_PRINT(time_scale);
		if (fHNumber == 264) {
			bool fixed_frame_rate_flag = bv.get1BitBoolean();
			DEBUG_PRINT(fixed_frame_rate_flag);
		}
		else { // 265
			bool vui_poc_proportional_to_timing_flag = bv.get1BitBoolean();
			DEBUG_PRINT(vui_poc_proportional_to_timing_flag);
			if (vui_poc_proportional_to_timing_flag) {
				unsigned vui_num_ticks_poc_diff_one_minus1 = bv.get_expGolomb();
				DEBUG_PRINT(vui_num_ticks_poc_diff_one_minus1);
			}
		}
	}
}

int getParaFromSPS(unsigned char *SPSbuf, unsigned int SPSsize, bool parseAll, stSpsPara &SpsPara)
{
	int res = 0;

	if (!parseAll)
		return res;

	unsigned num_units_in_tick;
	unsigned time_scale;
	num_units_in_tick = time_scale = 0; // default values

	unsigned char sps[SPS_MAX_SIZE] = { 0 };
	unsigned spsSize;
	removeEmulationBytes(sps, sizeof sps, spsSize, SPSbuf, SPSsize);

	BitVector bv(sps, 0, 8 * spsSize);

	bv.skipBits(8); // forbidden_zero_bit; nal_ref_idc; nal_unit_type
	unsigned profile_idc = bv.getBits(8);
	DEBUG_PRINT(profile_idc);
	unsigned constraint_setN_flag = bv.getBits(8); // also "reserved_zero_2bits" at end
	DEBUG_PRINT(constraint_setN_flag);
	unsigned level_idc = bv.getBits(8);
	DEBUG_PRINT(level_idc);
	unsigned seq_parameter_set_id = bv.get_expGolomb();
	DEBUG_PRINT(seq_parameter_set_id);
	if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 244 || profile_idc == 44 || profile_idc == 83 || profile_idc == 86 || profile_idc == 118 || profile_idc == 128) {
		DEBUG_TAB;
		unsigned chroma_format_idc = bv.get_expGolomb();
		DEBUG_PRINT(chroma_format_idc);
		if (chroma_format_idc == 3) {
			DEBUG_TAB;
			bool separate_colour_plane_flag = bv.get1BitBoolean();
			DEBUG_PRINT(separate_colour_plane_flag);
		}
		(void)bv.get_expGolomb(); // bit_depth_luma_minus8
		(void)bv.get_expGolomb(); // bit_depth_chroma_minus8
		bv.skipBits(1); // qpprime_y_zero_transform_bypass_flag
		bool seq_scaling_matrix_present_flag = bv.get1BitBoolean();
		//DEBUG_PRINT(seq_scaling_matrix_present_flag);
		if (seq_scaling_matrix_present_flag) {
			for (int i = 0; i < ((chroma_format_idc != 3) ? 8 : 12); ++i) {
				DEBUG_TAB;
				DEBUG_PRINT(i);
				bool seq_scaling_list_present_flag = bv.get1BitBoolean();
				//DEBUG_PRINT(seq_scaling_list_present_flag);
				if (seq_scaling_list_present_flag) {
					DEBUG_TAB;
					unsigned sizeOfScalingList = i < 6 ? 16 : 64;
					unsigned lastScale = 8;
					unsigned nextScale = 8;
					for (unsigned j = 0; j < sizeOfScalingList; ++j) {
						DEBUG_TAB;
						//DEBUG_PRINT(j);
						//DEBUG_PRINT(nextScale);
						if (nextScale != 0) {
							DEBUG_TAB;
							unsigned delta_scale = bv.get_expGolomb();
							//DEBUG_PRINT(delta_scale);
							nextScale = (lastScale + delta_scale + 256) % 256;
						}
						lastScale = (nextScale == 0) ? lastScale : nextScale;
						//DEBUG_PRINT(lastScale);
					}
				}
			}
		}
	}
	unsigned log2_max_frame_num_minus4 = bv.get_expGolomb();
	DEBUG_PRINT(log2_max_frame_num_minus4);
	unsigned pic_order_cnt_type = bv.get_expGolomb();
	DEBUG_PRINT(pic_order_cnt_type);
	if (pic_order_cnt_type == 0) {
		DEBUG_TAB;
		unsigned log2_max_pic_order_cnt_lsb_minus4 = bv.get_expGolomb();
		DEBUG_PRINT(log2_max_pic_order_cnt_lsb_minus4);
	}
	else if (pic_order_cnt_type == 1) {
		DEBUG_TAB;
		bv.skipBits(1); // delta_pic_order_always_zero_flag
		(void)bv.get_expGolomb(); // offset_for_non_ref_pic
		(void)bv.get_expGolomb(); // offset_for_top_to_bottom_field
		unsigned num_ref_frames_in_pic_order_cnt_cycle = bv.get_expGolomb();
		DEBUG_PRINT(num_ref_frames_in_pic_order_cnt_cycle);
		for (unsigned i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; ++i) {
			(void)bv.get_expGolomb(); // offset_for_ref_frame[i]
		}
	}
	unsigned max_num_ref_frames = bv.get_expGolomb();
	DEBUG_PRINT(max_num_ref_frames);
	bool gaps_in_frame_num_value_allowed_flag = bv.get1BitBoolean();
	DEBUG_PRINT(gaps_in_frame_num_value_allowed_flag);
	unsigned pic_width_in_mbs_minus1 = bv.get_expGolomb();
	DEBUG_PRINT(pic_width_in_mbs_minus1);
	unsigned pic_height_in_map_units_minus1 = bv.get_expGolomb();
	DEBUG_PRINT(pic_height_in_map_units_minus1);
	bool frame_mbs_only_flag = bv.get1BitBoolean();
	DEBUG_PRINT(frame_mbs_only_flag);
	if (!frame_mbs_only_flag) {
		bv.skipBits(1); // mb_adaptive_frame_field_flag
	}
	bv.skipBits(1); // direct_8x8_inference_flag
	bool frame_cropping_flag = bv.get1BitBoolean();
	DEBUG_PRINT(frame_cropping_flag);

	if (frame_cropping_flag) {
		(void)bv.get_expGolomb(); // frame_crop_left_offset
		(void)bv.get_expGolomb(); // frame_crop_right_offset
		(void)bv.get_expGolomb(); // frame_crop_top_offset
		(void)bv.get_expGolomb(); // frame_crop_bottom_offset
	}
	bool vui_parameters_present_flag = bv.get1BitBoolean();
	DEBUG_PRINT(vui_parameters_present_flag);
	if (vui_parameters_present_flag) {
		DEBUG_TAB;
		analyze_vui_parameters(bv, num_units_in_tick, time_scale);
	}

	SpsPara.nWidth = (pic_width_in_mbs_minus1 + 1) * 16;
	SpsPara.nHeight = (pic_height_in_map_units_minus1 + 1) * 16;

	SpsPara.num_units_in_tick = num_units_in_tick;
	SpsPara.time_scale = time_scale;
	
	if (SpsPara.num_units_in_tick == 0)
	{
		SpsPara.nFrameRate = 0.0;
	}
	else
	{
		SpsPara.nFrameRate = time_scale / float(2*num_units_in_tick);
	}

	return res;
}


int FindSpsPps(char *pbuf, int len, int &StartCodePos1, int &StartCodePos2, int &StartCodePos3)
{
	int pos = 0;
	StartCodePos1 = 0;
	StartCodePos2 = 0;
	StartCodePos3 = 0;
	bool ppsNextStartCode = false;
	for (pos = 0; pos<len;)
	{
		if (pbuf[pos] == 0x00 && pbuf[pos + 1] == 0x00 && pbuf[pos + 2] == 0x00 && pbuf[pos + 3] == 0x01)
		{
			if (((pbuf[pos + 4]) & 0x1F) == 0x07)//sps
			{
				pos += 4;
				StartCodePos1 = pos;
			}
			else if (((pbuf[pos + 4]) & 0x1F) == 0x08)//pps
			{
				pos += 4;
				StartCodePos2 = pos;
				ppsNextStartCode = true;
			}
			else
			{
				if (ppsNextStartCode)
				{
					pos += 4;
					StartCodePos3 = pos;
					break;
				}
				else
					pos += 4;
			}
		}
		else
		{
			pos += 1;
		}
	}
	if (StartCodePos1 != 0 && StartCodePos2 != 0 && StartCodePos3 != 0)
		return 0;
	else
		return -1;
}


int DahuaParse::getSDPStr(std::string &sdpStr, bool withAudio)
{
	int res = 0;
	int StartCodePos1 = 0, StartCodePos2 = 0, StartCodePos3 = 0;

	char *SpsStr = NULL;
	char *PpsStr = NULL;

	char *encodedSps = NULL;
	char *encodedPps = NULL;

	int SpsLen = 0;
	int PpsLen = 0;

	char arrProfileLevelI[100] = { 0 };
	char proLeId[4] = { 0 };

	bool parseAll = true;

	std::string spropParameterSets = "";
	std::string profileLevelId = "";

	stSpsPara *SpsPara = NULL;
	SpsPara = (stSpsPara *)malloc(sizeof(stSpsPara));
	memset(SpsPara, 0, sizeof(stSpsPara));
	do{

		res = FindSpsPps(m_pIFrameContent, m_iIFrameLen, StartCodePos1, StartCodePos2, StartCodePos3);
		if (res != 0)
		{
			res = -1;
			break;
		}

		SpsLen = StartCodePos2 - StartCodePos1 - 4;
		PpsLen = StartCodePos3 - StartCodePos2 - 4;
		SpsStr = new char[StartCodePos2 - StartCodePos1 - 4 + 1];
		PpsStr = new char[StartCodePos3 - StartCodePos2 - 4 + 1];
		memset(SpsStr, 0, StartCodePos2 - StartCodePos1 - 4);
		memset(PpsStr, 0, StartCodePos3 - StartCodePos2 - 4);
		memcpy(SpsStr, m_pIFrameContent + StartCodePos1, StartCodePos2 - StartCodePos1 - 4);
		memcpy(PpsStr, m_pIFrameContent + StartCodePos2, StartCodePos3 - StartCodePos2 - 4);

		//printf("the first four char 0X%02x,0X%02x,0X%02x,0X%02x,", SpsStr[0], SpsStr[1], SpsStr[2], SpsStr[3]);
		//get the profileLevelId paramters

		memcpy(proLeId + 1, SpsStr + 1, 3);

		sprintf_s(arrProfileLevelI, 100, "%06X", htonl(*(int *)proLeId));

		res = getParaFromSPS((unsigned char*)SpsStr, (unsigned int)SpsLen, parseAll, *SpsPara);
		if (res != 0)
		{
			res = -1;
			break;
		}

		encodedSps = base64Encode(SpsStr, SpsLen);
		if (!encodedSps)
		{
			res = -1;
			break;
		}
		encodedPps = base64Encode(PpsStr, PpsLen);
		if (!encodedPps)
		{
			res = -1;
			break;
		}
		//all param about h264 is ready
		spropParameterSets = encodedSps;
		spropParameterSets += ",";
		spropParameterSets += encodedPps;

		profileLevelId = arrProfileLevelI;

		if (spropParameterSets.length()<4)
		{
			fprintf(stderr, "the spspps string length too short!\n");
			res = -1;
			break;
		}
		if (profileLevelId.length()>6)
		{
			fprintf(stderr, "the profile-level-id string length too long!\n");
			res = -1;
			break;
		}

		char frameRate[100] = { 0 };
		char cliprect[100] = { 0 };

		if (SpsPara->num_units_in_tick == 0)
		{
			sprintf_s(frameRate, 100, "%.6f", (float)m_ivFrameRate);
		}
		else
		{
			sprintf_s(frameRate, 100, "%.6f", SpsPara->nFrameRate);
		}
		
		sprintf_s(cliprect, 100, "0,0,%u,%u", SpsPara->nHeight, SpsPara->nWidth);



		sdpStr = "v=0\r\n";
		sdpStr += "o=TEMOBI 579093000 18 IN IP4 0.0.0.0\r\n";
		sdpStr += "s=default\r\n";
		sdpStr += "c=IN IP4 0.0.0.0\r\n";
		sdpStr += "t=0 0\r\n";
		sdpStr += "a=control:*\r\n";

		sdpStr += "m=video 0 RTP/AVP 96\r\n";
		sdpStr += "a=control:trackID=0\r\n";
		sdpStr += "a=framerate:";
		sdpStr += frameRate;
		sdpStr += "\r\n";
		sdpStr += "a=rtpmap:96 H264/90000\r\n";
		sdpStr += "a=fmtp:96 packetization-mode=1;";
		sdpStr += "profile-level-id=";
		sdpStr += profileLevelId;
		sdpStr += ";sprop-parameter-sets=";
		sdpStr += spropParameterSets;
		sdpStr += "\r\n";
		sdpStr += "a=cliprect:";
		sdpStr += cliprect;
		sdpStr += "\r\n";

		if (withAudio)
		{
			sdpStr += "m=audio 0 RTP/AVP 8\r\n";
			sdpStr += "a=control:trackID=1\r\n";
			sdpStr += "a=rtpmap:8 PCMA/8000\r\n";
		}



	} while (0);

	//we should clear all resource 
	if (SpsStr)
		delete[] SpsStr;
	if (PpsStr)
		delete[] PpsStr;
	if (encodedSps)
		delete[] encodedSps;
	if (encodedPps)
		delete[] encodedPps;

	return res;
}

