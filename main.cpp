/*
*author:achilles_xu
*读取混合流文件，输出为音视频的原始帧
*修改
*/

#include <stdio.h>
#include <stdlib.h>
#include "SpliteStreamGetES.h"
#include "dahuaParse.h"
#include "haikangPSparse.h"

#pragma comment(lib,"ws2_32.lib")

#include "vld.h"
#pragma comment(lib,"vld.lib")


#define DDeBUG 1
#define CAMERA_SELECT 0    //0 for dahua, 1 for haikang
#define MAXBUF (1*1024*1024)
//回调将音视频的rtp缓冲分别写文件
FILE *fpv = NULL;
FILE *fpa = NULL;
FILE *fpTS = NULL;

void RTPReCall(char *buffer, int size, SpliteStreamESInfo Info,unsigned __int64 Pts, void *p);

int main(int argc,char **argv)
{
	printf("Hello achilles_xu\n");
	int cnt = 0;

	fopen_s(&fpTS, "H264-TS", "ab");
	fopen_s(&fpv, "H264-ES", "ab");
	fopen_s(&fpa, "Audio-ES", "ab");

	int BufSize = 0;
	char *Buff = new char[MAXBUF];

	FILE *fpt;

	char fileName[50] = { 0 };

	SpliteStreamGetES *getES = NULL;

	
#if (!CAMERA_SELECT)
	getES = new DahuaParse();
	getES->setCallBack(RTPReCall,NULL);
	for(int i=0;i<3000;i++){
#else
	getES = new HaikangPSparse();
	getES->setCallBack(RTPReCall, NULL);
	for (int i = 0; i<10000; i++){
#endif
		BufSize = 0;
		memset(Buff,0,MAXBUF);
		if (!CAMERA_SELECT)
			sprintf_s(fileName, sizeof(fileName), "dahua-15/InputSourceData_%d", i);
		else
			//sprintf_s(fileName, sizeof(fileName), "STREAM/stream_%d.H264", i);
			sprintf_s(fileName, sizeof(fileName), "haikang_move/InputPsData_%d", i);	
			//sprintf_s(fileName, sizeof(fileName), "haikang/stream_%d.H264", i);
		//strcat(fileName,".H264")
		
		//printf("the file name is %s\n",fileName);
		if(fopen_s(&fpt,fileName,"rb")!=0){
			//printf("the data file %s open failed\n",fileName);
			continue;
		}
		fseek(fpt,0,SEEK_END);
		BufSize = ftell(fpt);
		fseek(fpt,0,SEEK_SET);
		fread(Buff,1,BufSize,fpt);
		
		getES->InputBufData(Buff,BufSize);

		fclose(fpt);
		cnt++;

	}
	std::string sdpStr = "";
	((DahuaParse *)getES)->getSDPStr(sdpStr);

	printf("sdp:\n");
	printf("%s\n", sdpStr.c_str());

	delete getES;
	getES = NULL;
	delete[] Buff;
	Buff = NULL;
	fclose(fpv);
	fclose(fpa);
	fclose(fpTS);
	printf("Bye achilles_xu\n");

	return 0;
}
//将返回的数据写到一个文件中便于测试

void RTPReCall(char *buffer, int size, SpliteStreamESInfo Info, unsigned __int64 Pts, void *p)
{
	if (SpliteStreamGetES_MediaType_VEDIOTYPE == Info.mType)
	{
		fwrite(buffer, 1, size, fpv);
		fprintf(fpTS, "PTS = %I64d\n", Pts);
	}
	else
	{
		fwrite(buffer, 1, size, fpa);
	}
		

	//printf("RTPReCall\n");
}