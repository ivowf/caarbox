// Serial_Test.cpp : ??????????ó????????
//

#include "stdafx.h"

#include <windows.h>
#include <stdio.h>

#include <ctype.h>
#include <stdlib.h>



#include "Serial_port.h"

#include "cardRead.h"

#include "socket_client.h"

#define SMALL_SIM_PACKAGE


//#define DEBUG_MOD 1  //add by ml.20170206.

#ifdef DEBUG_MOD
    #define CARD_BOX_MDF_TIME "2018-8-18.18:18."
#else
    #define CARD_BOX_MDF_TIME "2017-2-06.17:28."
#endif  //end #ifdef DEBUG_MOD

/*2015.11.05, mod by Xili for saving file Package to local file, begin*/
#define ICCID_LEN 10
/*2015.11.05, mod by Xili for saving file Package to local file, end*/

//extern SimData simdata;

SimData *simdataHead = NULL;
SimData *simdataCurr = NULL;

SimBankIpAddrRecord *pSimBankIpAddrRecord = NULL;

ushort *pValidFlag = NULL;
ushort *pValidDataLen = NULL;
uchar *pIsWaitingHeartBeatRsp = NULL;

HANDLE hComa;

unsigned int cnt;
/*2016.04.21 mod by Xili for adding Serial Mutex, begin*/
//volatile bool isProcessComReq = false;
HANDLE  hSerialMutex;                // "Serial read/write" mutex
/*2016.04.21 mod by Xili for adding Serial Mutex, end*/

extern volatile bool isServerConnected ;
extern bool isProcessUeReq ;
extern SOCKET sockClient;

BOOL CtrlHandler( DWORD fdwCtrlType );

int setBandrate(HANDLE hcom);
void clearAllocateMemForSIMFcpAndData(SimData *simData_p);



int activeAndReadSIM(HANDLE hCom, unsigned short simNum, SimData *simdataCurr_p);

	
DWORD CALLBACK TimerThread(PVOID pvoid);
void CALLBACK TimeProc(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime);

long   __stdcall   ExceptionCallback(_EXCEPTION_POINTERS*   excp);


/*2015.11.05, mod by Xili for saving file Package to local file, begin*/
void saveSimFilesOnLocFile(unsigned char simID, SimData *simdataCurr_p, unsigned char *iccid);
BOOL readSimFilesFromLocFile(unsigned char simID, SimData *simdataCurr_p, unsigned char *iccid);
/*2015.11.05, mod by Xili for saving file Package to local file, end*/

//add by ml.20160804.
char cComPortBuf[6] = { 0 };
char *pComPort = NULL;
uchar pComPortNum = 0;
uchar pMaxSimNum = 0;
ushort pServerNetPort = 0;
uint pSimCardIdNum = 0;

char AnalysisCardBoxCfgFile(void)
{
    FILE * pFile = NULL;
    char cFileBuf[27] = { 0 };
    uchar cFileLen = 0;
    uchar i = 0, j = 0;
    uchar cSatausFlag = 0;
    char cSimNumBuf[4] = { 0 };
    char cServerPortBuf[6] = { 0 };
	char cSimCardIdBuf[9] = { 0 };
    
#ifdef DEBUG_MOD
    pFile = fopen("E:\\ServerFile\\debug\\CarBoxCfgFile.txt", "r");
#else
	pFile = fopen("CarBoxCfgFile.txt", "r");
#endif  //end #ifdef DEBUG_MOD
    
    if (NULL == pFile)
    {
        printf("????????????????...\n");
        
        return -1;
    }
    
    //COM0:9:1025:1:
    //COM99:201:65535:16777215:
    cFileLen = fread(cFileBuf, sizeof(uchar), 27, pFile);
    if ((cFileLen < 15) || (cFileLen >= 27))
    {
        printf("??????????????????...\n");
        
        fclose(pFile);
        pFile = NULL;
        
        return -1;
    }
    
    //COM0:9:1025:1:
    //COM99:201:65535:16777215:
    for (i = 0; i < cFileLen; i ++)
    {
        if (0 == cSatausFlag)  //????COM99
        {
            if (cFileBuf[i] != ':')
            {
                cComPortBuf[i] = cFileBuf[i];
                
                if (i >= 5)
                {
                    printf("?????????????????????????????1...\n");
                    
                    fclose(pFile);
                    pFile = NULL;
                    
                    return -1;
                }
            }
            else
            {
                if (4 == i)
                {
                    pComPortNum = (cComPortBuf[3] - 48);
                }
                else if (5 == i)
                {
                    pComPortNum = (cComPortBuf[3] - 48) * 10 +\
                                  (cComPortBuf[4] - 48) * 1;
                }
                
                cComPortBuf[i] = '\0';
                pComPort = &cComPortBuf[0];
                
                if (pComPortNum >= 100)
                {
                    printf("?????????????????????????????2...\n");
                    
                    fclose(pFile);
                    pFile = NULL;
                    
                    return -1;
                }
                
                cSatausFlag = 1;
                j = 0;
            }
        }
        else if (1 == cSatausFlag)  //????201
        {
            if (cFileBuf[i] != ':')
            {
                cSimNumBuf[j++] = cFileBuf[i];
                
                if (j > 3)
                {
                    printf("??????????????????SIM?????????????1...\n");
                    
                    fclose(pFile);
                    pFile = NULL;
                    
                    return -1;
                }
            }
            else
            {
                if (1 == j)
                    pMaxSimNum = cSimNumBuf[0] - 48;
                else if (2 == j)
                    pMaxSimNum = (cSimNumBuf[0] - 48) * 10 +\
                                 (cSimNumBuf[1] - 48) * 1;
                else if (3 == j)
                    pMaxSimNum = (cSimNumBuf[0] - 48) * 100 +\
                                 (cSimNumBuf[1] - 48) * 10  +\
                                 (cSimNumBuf[2] - 48) * 1;
                else
                {
                    printf("??????????????????SIM?????????????2...\n");
                    
                    fclose(pFile);
                    pFile = NULL;
                    
                    return -1;
                }
                
                if ((pMaxSimNum < 9) || (pMaxSimNum > 201) || ((pMaxSimNum % 3) != 0))
                {
                    printf("??????????????????SIM?????????????3...\n");
                    
                    fclose(pFile);
                    pFile = NULL;
                    
                    return -1;
                }
				
                cSatausFlag = 2;
                j = 0;
            }
        }
        else if (2 == cSatausFlag)  //????65535
        {
            if (cFileBuf[i] != ':')
            {
                cServerPortBuf[j++] = cFileBuf[i];
                
                if (j > 5)
                {
                    printf("??????????????????????????????????1...\n");
                    
                    fclose(pFile);
                    pFile = NULL;
                    
                    return -1;
                }
            }
            else
            {
            #if 0
                if (4 == j)
                    pServerNetPort = (cServerPortBuf[0] - 48) * 1000 +\
                                     (cServerPortBuf[1] - 48) * 100  +\
                                     (cServerPortBuf[2] - 48) * 10   +\
                                     (cServerPortBuf[3] - 48) * 1;
                else if (5 == j)
                    pServerNetPort = (cServerPortBuf[0] - 48) * 10000 +\
                                     (cServerPortBuf[1] - 48) * 1000  +\
                                     (cServerPortBuf[2] - 48) * 100   +\
                                     (cServerPortBuf[3] - 48) * 10    +\
                                     (cServerPortBuf[4] - 48) * 1;
                else
                {
                    printf("??????????????????????????????????2...\n");
                    
                    fclose(pFile);
                    pFile = NULL;
                    
                    return -1;
                }
                
                if (pServerNetPort < 1025)
                {
                    printf("??????????????????????????????????3...\n");
                    
                    fclose(pFile);
                    pFile = NULL;
                    
                    return -1;
                }
            #endif
                
                cSatausFlag = 3;
                j = 0;
            }
        }
        else if (3 == cSatausFlag)  //????16777215
        {
            if (cFileBuf[i] != ':')
            {
                cSimCardIdBuf[j++] = cFileBuf[i] - 48;
                
                if (j > 8)
                {
                    printf("??????????????????SIM??ID???????1...\n");
                    
                    fclose(pFile);
                    pFile = NULL;
                    
                    return -1;
                }
            }
            else
            {
                uchar k = 0;
                
                pSimCardIdNum = 0;
                
                for (k = 0; k < j; k++)
                {
                    pSimCardIdNum = pSimCardIdNum * 10 + cSimCardIdBuf[k];
                }
                
                if ((pSimCardIdNum < 1) || (pSimCardIdNum > 16777215))
                {
                    printf("??????????????????SIM??ID???????2...\n");
                    
                    fclose(pFile);
                    pFile = NULL;
                    
                    return -1;
                }
                
                cSatausFlag = 4;
                j = 0;
            }
        }
        else
        {
            cSatausFlag = 4;
            j = 0;
        }
    }
    
    fclose(pFile);
    pFile = NULL;
    
    return 0;
}

//add by ml.20170206.
char AnalysisServerCfgList(void)
{
    FILE * pFile = NULL;
    char cFileBuf[185] = { 0 };
    uchar cFileLen = 0;
    uchar i = 0, j = 0, k = 0;
    uchar cSatausFlag = 0;

    uchar index = 0;
    uchar cIpAddrBuf[16] = { 0 };
    uchar IPAddrValue[4] = { 0 };
    
    uchar cPortBuf[6] = { 0 };
    ushort PortValue = 0;
    
    uchar connectNum = 0;
    ushort connectTIme = 0;
    
    memset(pSimBankIpAddrRecord, 0, sizeof(SimBankIpAddrRecord));
    
#ifdef DEBUG_MOD
    pFile = fopen("E:\\ServerFile\\debug\\ServerConfigList.txt", "r");
#else
    pFile = fopen("ServerConfigList.txt", "r");
#endif  //end #ifdef DEBUG_MOD
    
    if (NULL == pFile)
    {
        printf("???????????б?????????...\n");
        
        return -1;
    }
    
    // 0:1.1.1.1:1025:3:5:  ---19
    // 5:255.255.255.255:65535:9:999:  ---30
    cFileLen = fread(cFileBuf, sizeof(uchar), 185, pFile);
    if ((cFileLen < (2 * 19 + 1)) ||\
        (cFileLen > 185))
    {
        printf("???????????б???????????...\n");
        
        fclose(pFile);
        pFile = NULL;
        
        return -1;
    }
    
    // 0:47.90.89.43:9734:5:10:
    // 1:47.90.57.160:9734:5:10:
    // 2:116.62.43.63:9734:5:10:
    // 3:47.89.40.236:9734:5:10:
    // 5:58.96.188.197:9988:5:120:
    for (i = 0; i < cFileLen; i ++)
    {
        if (0 == cSatausFlag)  //0-4??5
        {
            if (cFileBuf[i] != ':')
            {
                index = (cFileBuf[i] - 48);
                
                if (index > 5)
                {
                    printf("???????????б?????????????????????1...\n");
                    
                    fclose(pFile);
                    pFile = NULL;
                    
                    return -1;
                }
            }
            else
            {
                cSatausFlag = 1;
                j = 0;
            }
        }
        else if (1 == cSatausFlag)  //????58.96.188.197
        {
            if (cFileBuf[i] != ':')
            {
                cIpAddrBuf[j++] = cFileBuf[i];
                
                if (j > 15)
                {
                    printf("???????????б???????????IP??????????1...\n");
                    
                    fclose(pFile);
                    pFile = NULL;
                    
                    return -1;
                }
            }
            else
            {
                uchar ipcnt = 0;
                cIpAddrBuf[j] = cFileBuf[i];
                
                for (k = 0; k < j; k++)
                {
                    if (cIpAddrBuf[k] != '.')
                    {
                        IPAddrValue[ipcnt] = IPAddrValue[ipcnt] * 10 +\
                                             cIpAddrBuf[k] - 48;
                    }
                    else
                    {
                        ipcnt ++;
                    }
                }
                
                if (ipcnt < 3)
                {
                    printf("???????????б???????????IP??????????2...\n");
                    
                    fclose(pFile);
                    pFile = NULL;
                    
                    return -1;
                }
                
                cSatausFlag = 2;
                j = 0;
            }
        }
        else if (2 == cSatausFlag)  //????1025-65535
        {
            if (cFileBuf[i] != ':')
            {
                PortValue = PortValue * 10 + cFileBuf[i] - 48;
            }
            else
            {
                if (PortValue < 1025)
                {
                    printf("???????????б???????????Port???????...\n");
                    
                    fclose(pFile);
                    pFile = NULL;
                    
                    return -1;
                }
                
                cSatausFlag = 3;
                j = 0;
            }
        }
        else if (3 == cSatausFlag)  //????3-9
        {
            if (cFileBuf[i] != ':')
            {
                connectNum = cFileBuf[i] - 48;
            }
            else
            {
                if ((connectNum < 3) || (connectNum > 9))
                {
                    printf("???????????б??????????????????????????...\n");
                    
                    fclose(pFile);
                    pFile = NULL;
                    
                    return -1;
                }
                
                cSatausFlag = 4;
                j = 0;
            }
        }
        else if (4 == cSatausFlag)  //????5-999
        {
            if (cFileBuf[i] != ':')
            {
                connectTIme = connectTIme * 10 + cFileBuf[i] - 48;
            }
            else
            {
                if ((connectTIme < 5) || (connectTIme > 999))
                {
                    printf("???????????б????????????????????????????...\n");
                    
                    fclose(pFile);
                    pFile = NULL;
                    
                    return -1;
                }
                
                if (index < 5)
                {
                    if (0 == pSimBankIpAddrRecord->loadBalancingRecord[index].fieldValidFlag)
                    {
                        pSimBankIpAddrRecord->loadBalancingRecord[index].fieldValidFlag = 1;
                        memcpy(pSimBankIpAddrRecord->loadBalancingRecord[index].ipAddr, IPAddrValue, 4);
                        pSimBankIpAddrRecord->loadBalancingRecord[index].portAddr = PortValue;
                        pSimBankIpAddrRecord->loadBalancingRecord[index].connectNum = connectNum;
                        pSimBankIpAddrRecord->loadBalancingRecord[index].overTimes = connectTIme;
                        
                        pSimBankIpAddrRecord->serverIpValidFlag |= (1 << index);
                    }
                    else
                    {
                        printf("???????????б???????????????????1...\n");
                        
                        fclose(pFile);
                        pFile = NULL;
                        
                        return -1;
                    }
                }
                else if (5 == index)
                {
                    if (0 == pSimBankIpAddrRecord->defaultIpPortRecord.fieldValidFlag)
                    {
                        pSimBankIpAddrRecord->defaultIpPortRecord.fieldValidFlag = 1;
                        memcpy(pSimBankIpAddrRecord->defaultIpPortRecord.ipAddr, IPAddrValue, 4);
                        pSimBankIpAddrRecord->defaultIpPortRecord.portAddr = PortValue;
                        pSimBankIpAddrRecord->defaultIpPortRecord.connectNum = connectNum;
                        pSimBankIpAddrRecord->defaultIpPortRecord.overTimes = connectTIme;
                        
                        pSimBankIpAddrRecord->serverIpValidFlag |= (1 << index);
                    }
                    else
                    {
                        printf("???????????б???????????????????2...\n");
                        
                        fclose(pFile);
                        pFile = NULL;
                        
                        return -1;
                    }
                }
                else
                {
                    ;
                }
                
                cSatausFlag = 5;
                j = 0;
            }
        }
        else
        {
            cSatausFlag = 0;
            j = 0;
			memset(IPAddrValue, 0, 4);
			PortValue = 0;
            connectNum = 0;
			connectTIme = 0;
        }
    }
    
    if ((0 == (pSimBankIpAddrRecord->serverIpValidFlag & 0x1F)) ||\
         (0 == (pSimBankIpAddrRecord->serverIpValidFlag & 0x20)))
    {
        printf("???????????б?????????????????????????????IP?????????????IP 1...\n");
        
        fclose(pFile);
        pFile = NULL;
        
        return -1;
    }
    
    printf("???????????????б??????\n");
    for (i = 0; i <= 5; i++)
    {
        if ((i < 5) && (1 == pSimBankIpAddrRecord->loadBalancingRecord[i].fieldValidFlag))
        {
            printf("  ????????????%d????????\n", i);
            printf("    IP???       = %d.%d.%d.%d.\n", pSimBankIpAddrRecord->loadBalancingRecord[i].ipAddr[0],
                                                        pSimBankIpAddrRecord->loadBalancingRecord[i].ipAddr[1],
                                                        pSimBankIpAddrRecord->loadBalancingRecord[i].ipAddr[2],
                                                        pSimBankIpAddrRecord->loadBalancingRecord[i].ipAddr[3]);
            printf("    ????       = %d.\n", pSimBankIpAddrRecord->loadBalancingRecord[i].portAddr);
            printf("    ????????     = %d.\n", pSimBankIpAddrRecord->loadBalancingRecord[i].connectNum);
            printf("    ?????????? = %d.\n", pSimBankIpAddrRecord->loadBalancingRecord[i].overTimes);
        }
        else if ((5 == i) && (1 == pSimBankIpAddrRecord->defaultIpPortRecord.fieldValidFlag))
        {
            printf("  ??????????÷?????????????\n");
            printf("    IP???       = %d.%d.%d.%d.\n", pSimBankIpAddrRecord->defaultIpPortRecord.ipAddr[0],
                                                        pSimBankIpAddrRecord->defaultIpPortRecord.ipAddr[1],
                                                        pSimBankIpAddrRecord->defaultIpPortRecord.ipAddr[2],
                                                        pSimBankIpAddrRecord->defaultIpPortRecord.ipAddr[3]);
            printf("    ????       = %d.\n", pSimBankIpAddrRecord->defaultIpPortRecord.portAddr);
            printf("    ????????     = %d.\n", pSimBankIpAddrRecord->defaultIpPortRecord.connectNum);
            printf("    ?????????? = %d.\n\n", pSimBankIpAddrRecord->defaultIpPortRecord.overTimes);
        }
        else
        {
            ;
        }
    }
    
    fclose(pFile);
    pFile = NULL;
    
    return 0;
}

int pLenCase = 0;
void EexceptionalCasesRecord(uchar uErrorType)
{
    FILE *pEexceptionalFile = NULL;
    char RecordBuf[256] = { 0 };
    uchar i = 0;
    
    //pEexceptionalFile = fopen("E:\\ServerFile\\test\\CarBoxEexceptionalFile.log", "a+");
	pEexceptionalFile = fopen("CarBoxEexceptionalFile.log", "a+");
    if (NULL == pEexceptionalFile)
    {
        printf("?????????????????????...\n\n");
        
        return;
    }
    
    SYSTEMTIME sys_cur;
	GetLocalTime(&sys_cur);
	pLenCase = fprintf(pEexceptionalFile, "CurrentTime: %4d/%02d/%02d %02d:%02d:%02d.%03d ---> ", sys_cur.wYear, sys_cur.wMonth, 
	                   sys_cur.wDay, sys_cur.wHour, sys_cur.wMinute, sys_cur.wSecond, sys_cur.wMilliseconds);
	
	switch (uErrorType)
	{
	    case RS485_RECV_PACKET_ERROE:
	        pLenCase = fprintf(pEexceptionalFile, "RS-485????????????...\n");
	        
	        break;
	    case NETWORK_RECV_DATA_EQUAL_0:
	        pLenCase = fprintf(pEexceptionalFile, "?????????????0...\n");
	        
	        break;
	    case NETWORK_RECV_DATA_LESS_0:
	        pLenCase = fprintf(pEexceptionalFile, "????????????С??0...\n");
	        
	        break;
	    case NETWORK_LINK_RECONNECTION:
	        pLenCase = fprintf(pEexceptionalFile, "??????·????????...\n");
	        
	        break;
	    case NETWORK_HEART_BEAT_NO_RECV:
	        pLenCase = fprintf(pEexceptionalFile, "??????????δ??????...\n");
	        
	        break;
	    default :
	        pLenCase = fprintf(pEexceptionalFile, "EexceptionalCasesRecord--->?????????...\n");
	        
	        break;
	}
	
	//fwrite(RecordBuf, pLenCase, 1, pEexceptionalFile);
	
	fclose(pEexceptionalFile);
	
	return;
}

void main()
{
    char ret = 0;
    
    ret = AnalysisCardBoxCfgFile();
    if (ret < 0)
    {
        printf("???????????????????...\n");
        
        return ;
    }
    else
    {
        printf("???????????????:\n");
        printf("  ???????   =  %s.\n", pComPort);
        printf("  ???SIM????  =  %d.\n", pMaxSimNum);
        //printf("  ?????????? =  %d.\n", pServerNetPort);
        printf("  SIM??ID      =  %d.\n", pSimCardIdNum);
        printf("  ???????汾 =  %s.\n\n", CARD_BOX_MDF_TIME);
    }
    
    pSimBankIpAddrRecord = (SimBankIpAddrRecord *)malloc(sizeof(SimBankIpAddrRecord));
    if (NULL == pSimBankIpAddrRecord)
    {
        printf("malloc pSimBankIpAddrRecord Error.\n");
        
        return ;
    }
    
    pSimBankIpAddrRecord->serverIpValidFlag = 0;
    pSimBankIpAddrRecord->netCurrentConnectFlag = 0;
    pSimBankIpAddrRecord->netNextConnectFlag = 0;
    
    ret = AnalysisServerCfgList();
    if (ret < 0)
    {
        printf("???????????????б????????...\n");
        
        return ;
    }
    
       HANDLE hCom;
	   SetUnhandledExceptionFilter(ExceptionCallback);	   
       /*Test pack and unpack function*/
	    //hCom = Open_driver_T("COM1");/*open com1-com9*/
	    //hCom = Open_driver_T("COM3");  //TEST

        if (pComPortNum <= 9)
    	    hCom = Open_driver_T(pComPort);  //mdf by ml.20160804.
    	else  //add by ml.20160810.
    	{
        	char sComPortBufUp10[13] = "\\\\.\\COM";
            
            sComPortBufUp10[7] = cComPortBuf[3];
            sComPortBufUp10[8] = cComPortBuf[4];
            sComPortBufUp10[9] = '\0';
            
            hCom = Open_driver_T(sComPortBufUp10);
        }
        
		//hCom = Open_driver_T("\\\\.\\COM10");/*open >= com10*/
		if(!hCom)
	    {
			  printf("??????????\n");
			  return ;
	    }
	    else
	    {
		   printf("?????????!\n");
	   	}
		//setBandrate(hCom);
		hComa = hCom;
		if( SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE ) )
		 {
		  printf( "\nThe Control Handler is installed.\n" );
		  printf( "\n -- Now try pressing Ctrl+C or Ctrl+Break, or" );
		  printf( "\n    try logging off or closing the console...\n" );
		  printf( "\n(...waiting in a loop for events...)\n\n" );
		 
		}
		 else
  			printf( "\nERROR: Could not set control handler");

        //add by ml.20160906.
        pValidFlag = (ushort *)malloc(sizeof(ushort));
        pValidDataLen = (ushort *)malloc(sizeof(ushort));
        //add by ml.20161119.
        pIsWaitingHeartBeatRsp = (uchar *)malloc(sizeof(uchar));
        
        if ((NULL == pValidFlag) || (NULL == pValidDataLen) || (NULL == pIsWaitingHeartBeatRsp))
        {
            printf("malloc pValidFlag or pValidDataLen or pIsWaitingHeartBeatRsp Error.\n");
            
            return;
        }
        
        *pValidFlag = 0;
        *pValidDataLen = 0;
        *pIsWaitingHeartBeatRsp = 0;
        
		/*RESET and ATR check*/
		//Led1 on, led2 off
		//setLedStatus(hCom, 0x01);

		/*Test BUS time delay, 55ms~60ms*/
		#if 0
		BYTE ResetCmd3V[9] = {0xaa,0xbb,0x05,0x00,0x00,0x00,0x04,0x01,0x05};
		BYTE RecvData[267] = {0};
		//unsigned char ATR[100] = {0};
		//unsigned char atrLen = 0;
		//bool checkresult = false;
		unsigned int realReceiveLeg = 0;
		unsigned char num = 0;

		while(num < 200)
		{
		
			if(hCom != NULL)
			{
			  if(Send_driver_T(hCom, ResetCmd3V, 0x09))
			  {
					 printf("??λ???? ???????!\n");
			  }
			  else
			  {
					 printf("??λ???? ??????!\n");
			  }
			 // Sleep(100);
		
			  if(Receive_driver_T(hCom, RecvData, &realReceiveLeg))
			  {
					 printf("ATR???? ???????!\n");
			  }
			  else
			  {
					 printf("ATR???? ??????!\n");
			  }
			}
			num++;
		}
		#endif
		//Sleep(100);
		//#if 0
		/*Set and read SIM files*/
		/*2016.04.21 mod by Xili for adding Serial Mutex, begin*/
	    // Create the mutexes and reset thread count.
        hSerialMutex = CreateMutex( NULL, FALSE, NULL );  // Cleared 	   
		//isProcessComReq=false;
		/*2016.04.21 mod by Xili for adding Serial Mutex, end*/
		//for(unsigned char simNum = 1; simNum <= ALLSIMNUM; simNum++ )
		for(unsigned char simNum = 1; simNum <= pMaxSimNum; simNum++ )  //mdf by ml.20160804.
		{
			SimData *simdata_tmp = (SimData *)malloc(sizeof(SimData));  //
			if(simdata_tmp != NULL)
			{
				printf("Simdata mem allocate OK \n");
				memset(simdata_tmp,0,sizeof(SimData));
				simdata_tmp->isProcessNetcmd = false;
				simdata_tmp->isProcessNetcmd = false;
				simdata_tmp->next_SIM = NULL; 
				simdata_tmp->simId = simNum;
				/*2016.06.04, mod by Xili for reducing the same usimStatus reported repeat,begin*/
				simdata_tmp->pre_usimStatus = USIM_STATUS;
				/*2016.06.04, mod by Xili for reducing the same usimStatus reported repeat,end*/
				if(simNum == 1)
				{
					simdataHead = simdata_tmp;
				}
				else /*if simNum is not 1, simdataCurr shall not be null*/
				{
					simdataCurr->next_SIM = simdata_tmp;
				}
				simdataCurr = simdata_tmp;
				simdataCurr->isUsim = TRUE;
				simdataCurr->hCom = hCom;
				//getCurrentSingleUsimStatus(hComa, simdataCurr->simId, &(simdataCurr->usimStatus));
				//if(simdataCurr->usimStatus!= NO_USIM)
				{
					activeAndReadSIM(hCom, simNum, simdataCurr);
				}
			}
		}
		//SIM initialised OK
		//Led1 off, Led2 on
		//setLedStatus(hCom, 0x02);
		//#endif
		/*Add the socket server here*/
		HANDLE hThreadSoc, hThreadtimer;
		DWORD hThreadId, dwThreadId;
		
		HANDLE hThreadUdp;
		DWORD uThreadId;
		
		printf("\n/********************Creat thread for the Udp socket process*******************/\n");
		hThreadUdp = CreateThread(NULL, 0,(LPTHREAD_START_ROUTINE)UdpSocketThread, 0, 0, &uThreadId);
   		printf("\n/********************Udp socket process Created*******************/\n");
   		
   		Sleep(1000);
   		
		printf("\n/********************Creat thread for the socket process*******************/\n");
		hThreadSoc = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)socketClient,0,0,&hThreadId);
		//socketClient();
		printf("\n/********************Socket process Created*******************/\n");
        
		Sleep(1000);
		
		printf("\n/********************Creat thread for the Timer process*******************/\n");
		hThreadtimer = CreateThread(NULL, 0,(LPTHREAD_START_ROUTINE)TimerThread, 0, 0, &dwThreadId);   
   		printf("\n/********************Timer process Created*******************/\n");
   		
		unsigned char c;
		scanf_s("%c",&c,1);
		printf("%c, %d",c, c);
		if(c > 0)
		{
			if(hCom != NULL)
			{
				Close_driver(hCom);
			}
		}
		/*2016.04.21 mod by Xili for adding Serial Mutex, begin*/
        CloseHandle( hSerialMutex );
		/*2016.04.21 mod by Xili for adding Serial Mutex, end*/
}

/* for single sim status process
void CALLBACK TimeProc(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime)  
{  
	
    printf("TimeProc thread count = %d, simNum is %d \n", cnt, (cnt%ALLSIMNUM+1));
	SimData *simData_p = NULL;
	simData_p = simdataHead;
	UsimStatus usimStatus_before;
	//to check the SIM status
	if(isProcessUeReq == false)
	{
		while((simData_p != NULL))
		{
			if(simData_p->simId == (cnt%ALLSIMNUM+1))
			{
				usimStatus_before = simData_p->usimStatus;
				getCurrentSingleUsimStatus(hComa, simData_p->simId, &(simData_p->usimStatus));
				if((usimStatus_before == NO_USIM)&&(simData_p->usimStatus == USIM_OFF))
				{
					//there is a new sim inserted
					printf("TimeProc %d SIM slot inserted a new SIM\n", simData_p->simId);
					simdataCurr = simData_p;
					simdataCurr->isUsim = TRUE;
					activeAndReadSIM(hComa, simData_p->simId);
				}
				break;
			}
			
			simData_p = simData_p->next_SIM;
		}
		if(cnt%ALLSIMNUM == 0)
		{
			printf("TimeProc a round of usimStatus updated, report to server\n");
			sendUsimstatusInd();
		}
		cnt++;
	}
	else
	{
		printf("TimeProc %d SIM slot inserted a new SIM\n", simData_p->simId);
	}
	
	
	
}  
*/
void CALLBACK TimeProc(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime)  
{  
	
    
	SimData *simData_p = NULL;
	UsimStatus	   beforeusimStatus;
	
	//UsimStatus usimStatus_before;
	bool simfound = false;
	//printf("\nTimeProc thread count = %d, simNum is %d \n", cnt, (cnt%ALLSIMNUM+1));
	printf("\nTimeProc thread count = %d, simNum is %d \n", cnt, (cnt%pMaxSimNum+1));  //mdf by ml.20160804.
	//to check the SIM status
	if(isProcessUeReq == true)
	{
		//printf("TimeProc %d time out, %d SIM socket thread is processing\n", cnt,(cnt%ALLSIMNUM+1));
		printf("TimeProc %d time out, %d SIM socket thread is processing\n", cnt,(cnt%pMaxSimNum+1));  //mdf by ml.20160804.
	}
	//if(isProcessUeReq == false)
	{
		simData_p = simdataHead;
		while((simData_p != NULL))
		{
		//	printf("\nTimeProc thread simData_p->simId is %d \n", simData_p->simId);
			//if(simData_p->simId == (unsigned char)(cnt%ALLSIMNUM+1))
			if(simData_p->simId == (unsigned char)(cnt%pMaxSimNum+1))  //mdf by ml.20160804.
			{
				if(simData_p->isProcessNetcmd == false)
				{
					simfound = true;
					/*2016.06.04, mod by Xili for reducing the same usimStatus reported repeat, begin*/
					beforeusimStatus = simData_p->usimStatus;
					printf("\n SIM%d pre status: %d\n", simData_p->simId,beforeusimStatus);
					/*2016.06.04, mod by Xili for reducing the same usimStatus reported repeat, end*/
				}
				else
				{
					printf("\n %d found but it is process NET cmd!!!", simData_p->simId);
				}
				break;
			}
			
			simData_p = simData_p->next_SIM;
		}
		
		if(simfound == true)
		{
			simData_p->isProcessStatuscmd = true;
			
			getCurrentUsimStatus(hComa, simData_p->simId, &(simData_p->usimStatus));
			if(/*(usimStatus_before == NO_USIM)&&*/(simData_p->usimStatus == USIM_OFF))
			{
				//there is a new sim inserted
				printf("\nTimeProc %d SIM slot inserted a new SIM!!!!", simData_p->simId);
				printf("\nSet simData_tmp->pre_usimStatus as USIM_STATUS for new SIM\n");
				simData_p->pre_usimStatus = USIM_STATUS;
				simData_p->isUsim = TRUE;
				clearAllocateMemForSIMFcpAndData(simData_p);
				activeAndReadSIM(hComa, simData_p->simId, simData_p);
				if(simData_p->next_SIM != NULL)
				{
					printf("\nTimeProc Test SIM, next SIM is not null ");
				}
				else
				{
					printf("\nTimeProc Test SIM, next SIM is null ");
				}
			}
			else if(simData_p->usimStatus == NO_USIM)
			{
				/*print the FCP and data each read file and delete the allocate memory*/
				clearAllocateMemForSIMFcpAndData(simData_p);				
				
			}
			//if((cnt+1)%ALLSIMNUM == 0)
			if((cnt+1)%pMaxSimNum == 0)  //mdf by ml.20160804.
			{
				printf("\nTimeProc a round of usimStatus updated, report to server");
				//sendUsimstatusInd();
				if(isServerConnected == TRUE)
					sendUsimInfoUpdateInd();
				else
					printf("\nThe connect to server is down!!!!");
			}
			cnt++;

			simData_p->isProcessStatuscmd = FALSE;
		}
		
	}
	//printf("\nTimeProc: isProcessComReq:%d\n", isProcessComReq);
	
	
	
}  
DWORD CALLBACK TimerThread(PVOID pvoid)  
{  
    MSG msg;  
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);   
    SetTimer(NULL, 10, 1000, TimeProc);  //1s
    while(GetMessage(&msg, NULL, 0, 0))  
    {  
        if(msg.message == WM_TIMER)  
        {  
            TranslateMessage(&msg);    // ???????  
            DispatchMessage(&msg);     // ??????  
        }  
    }  
    KillTimer(NULL, 10);  
    return 0;  
}  

 
 void clearAllocateMemForSIMFcpAndData(SimData *simData_p)
 {
 	printf("\n/********************clearAllocateMemForSIMFcpAndData  delete the allocate memory, Begin********************/\n");
	for(unsigned char filenum = 0; filenum < simData_p->simFilesNum; filenum++)
	{
		printf("\n\n/*****%dst FILEID=%02X%02X, FILETYPE=%d, recordLen= %d, recordNum =%d, dataLen=%d*****/\n", 
				(filenum+1),
				simData_p->simfile[filenum].fileId[0],
				simData_p->simfile[filenum].fileId[1],
				simData_p->simfile[filenum].fileType,
				simData_p->simfile[filenum].recordLen,
				simData_p->simfile[filenum].recordNum,
				simData_p->simfile[filenum].dataLen);
		
		if(simData_p->simfile[filenum].fcpLen > 0)
		{
			printf("-----Clear FCP data :\n");
			delete simData_p->simfile[filenum].fcp;
			simData_p->simfile[filenum].fcp = NULL;
		}
		else
		{
			printf("\n-----NO File FCP: The File not exist:\n");
		}
		
		if(simData_p->simfile[filenum].dataLen > 0)
		{
			printf("\n-----Clear File data Mem:\n");
			delete simData_p->simfile[filenum].data;
			simData_p->simfile[filenum].data = NULL;
		}
		else
		{
			printf("\n-----NO File data: MF DF or File not exist\n");
		}
		
		
	}

	simData_p->simFilesNum = 0;		
	printf("\n/********************clearAllocateMemForSIMFcpAndData delete the allocate memory, End********************/\n");
 }


int activeAndReadSIM(HANDLE hCom, unsigned short simNum, SimData *simdataCurr_p)
{

	/*2015.11.05, mod by Xili for saving file Package to local file, begin*/
	UsimStatus lastUsimStatus;
    if(hCom != NULL)
    {
	    getCurrentUsimStatus(hCom, simNum, &lastUsimStatus);
		printf("\nSave last Usimstatus: %d\n", lastUsimStatus);
		if(lastUsimStatus == NO_USIM)
		{
			return 1;
		}
    }
	/*2015.11.05, mod by Xili for saving file Package to local file, end*/
	
	if((hCom != NULL)&&(resetAndAtrCheck3V(hCom,simdataCurr_p) == TRUE))
	{

			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM, next SIM is null ");
			}

			printf("\n/********************Test SIM or USIM********************/\n");
			unsigned char *testfileId = new unsigned char[2];
			testfileId[0]= 0x3F;
			testfileId[1]= 0x00;
			printf("\n******If support 00*****\n");
			//processSelectCmd(hCom,testfileId,TRUE,simdataCurr_p);
			//mdf by 20160918.
			if (FALSE == processSelectCmd(hCom, testfileId, TRUE, simdataCurr_p))
			{
				printf("\n 00 MF read fail, set the SIM as 2G SIM");
				/*
				simdataCurr_p->usimStatus = USIM_INITING;
				 setCurrentUsimStatus(simdataCurr_p->hCom, simdataCurr_p->simId, simdataCurr_p->usimStatus);
				 */
				simdataCurr_p->isUsim = FALSE;
				if(FALSE == processSelectCmd(hCom,testfileId,TRUE,simdataCurr_p))
				{
					printf("\n A0 USIM MF read also fail, set the USIM as invalid ");
					return 1;
				}
				
			}
			else
			{
				simdataCurr_p->isUsim=testIsUsimCard(simdataCurr_p->sw);
			}
			
			if(simdataCurr_p->isUsim == true)
			{
				printf("\n******supported 00, check if EFdir exist********\n");
				testfileId[0]= 0x2F;
				testfileId[1]= 0x00;
				processSelectCmd(hCom,testfileId,TRUE,simdataCurr_p);
				if(simdataCurr_p->simstatus==SIM_FILE_NOT_FOUND)
					simdataCurr_p->isUsim = false;
			}

			printf("\n******After test, simdataCurr_p->isUsim = %d*****\n",simdataCurr_p->isUsim);
			if(simdataCurr_p->isUsim == false)
			{
				printf("\n******simdataCurr_p->isUsim=false, Rest the card********\n");
				resetAndAtrCheck3V(hCom,simdataCurr_p);
			}


			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM1, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM1, next SIM is null ");
			}
			//simdataCurr_p->hCom = hCom;

			simdataCurr_p->simFilesNum = 0;
			simdataCurr_p->currentDF = 0x3F00;
			unsigned char *fileId = new unsigned char[2];
			FileType currentFileType = FILETYPENUM;
			unsigned short recordLen = 0;
			unsigned char recordNum = 0;
			unsigned short fcpLen = 0;
			SimFile	 currenSimFile;
			unsigned char iccid[ICCID_LEN];/*2015.11.05, mod by Xili for saving file Package to local file*/
			//select MF
			printf("\n/********************Select MF********************/\n");
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			fileId[0]= 0x3F;
			fileId[1]= 0x00;
			currenSimFile.currentDF[0]=0x00;
			currenSimFile.currentDF[1]=0x00;
			processSelectCmd(hCom,fileId,TRUE,simdataCurr_p);
//			simdataCurr_p->testIsUsimCard(simdataCurr_p->sw);
			fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
			if( fcpLen >0 )
			{
				/*simdataCurr_p->responseApdu contains the FCP and sw*/
				saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				currenSimFile.fileType = MF;
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;			
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFiccid********************/\n");
			//select EFiccid
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			currenSimFile.currentDF[0]=0x3F;
			currenSimFile.currentDF[1]=0x00;
			fileId[0]= 0x2F;
			fileId[1]= 0xE2;
			processSelectCmd(hCom,fileId,TRUE,simdataCurr_p);
			fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
			if( fcpLen >0 )
			{
				/*simdataCurr_p->responseApdu contains the FCP and sw*/
				saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
							
			}
			printf("\n/********************Read EFiccid********************/\n");
			ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			/*2015.11.05, mod by Xili for saving file Package to local file, begin*/
			memcpy(iccid, simdataCurr_p->simfile[simdataCurr_p->simFilesNum].data, simdataCurr_p->simfile[simdataCurr_p->simFilesNum].dataLen);
			for(unsigned int i = 0; i< ICCID_LEN; i++)
			{
				printf(" %02X", iccid[i]);
			}
			/*2015.11.05, mod by Xili for saving file Package to local file, end*/
			simdataCurr_p->simFilesNum++;
			printf("\nThis is simFilesNum = %d \n", simdataCurr_p->simFilesNum);			
				
			printf("\n/********************Select EFpl********************/\n");
			//select EFpl
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			currenSimFile.currentDF[0]=0x3F;
			currenSimFile.currentDF[1]=0x00;
			fileId[0]= 0x2F;
			fileId[1]= 0x05;
			processSelectCmd(hCom,fileId,TRUE,simdataCurr_p);
			fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
			if( fcpLen >0 )
			{
				/*simdataCurr_p->responseApdu contains the FCP and sw*/
				saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
			}
			printf("\n/********************Read EFpl********************/\n");
			ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFarr********************/\n");
			//select EFarr
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			currenSimFile.currentDF[0]=0x3F;
			currenSimFile.currentDF[1]=0x00;
			fileId[0]= 0x2F;
			fileId[1]= 0x06;
			processSelectCmd(hCom,fileId,TRUE,simdataCurr_p);
			fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
			if( fcpLen >0 )
			{
				/*simdataCurr_p->responseApdu contains the FCP and sw*/
				saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
			}
			printf("\n/********************Read EFarr********************/\n");
			ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
			
			if(simdataCurr_p->isUsim==true)
			{
				printf("\n/********************Select EFdir********************/\n");
				//select EFdir
				memset(&currenSimFile, 0x0, sizeof(SimFile));
				currenSimFile.currentDF[0]=0x3F;
				currenSimFile.currentDF[1]=0x00;
				fileId[0]= 0x2F;
				fileId[1]= 0x00;
				processSelectCmd(hCom,fileId,TRUE,simdataCurr_p);
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				
							//read EFdir
					printf("\n/********************Read EFdir********************/\n");
					ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
					simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
					simdataCurr_p->simFilesNum++;
					printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

					printf("\n/********************Read AID and Active USIM Application********************/\n");
					unsigned char AID[16]={0};
					unsigned char AIDLoc = 0;
					unsigned char AIDLen = 0;
					for(unsigned char i = 1; i <= recordNum; i++)
					{
						if(ProcessReadCmd(hCom,currentFileType, recordLen, i, NULL, simdataCurr_p)== TRUE)
						{
							printf("\n/********************Select AID********************/\n");
							/*61 1E 4F 10 A0 00 00 03 43 10 02 FF 86 FF 03 89 FF FF FF FF 
									50 0A 74 69 61 6E 79 75 63 73 69 6D FF FF FF FF FF FF FF
									FF FF FF FF FF FF FF FF FF 90 00
		 						*/
							AIDLoc = 0;
							AIDLen = 0;
							printf("\nsimdataCurr_p->responseApdu[0]=%d\n",simdataCurr_p->responseApdu[0]);
							if(simdataCurr_p->responseApdu[0] == (unsigned char)APPLICATIONTEMPLATE)
							{
								for(AIDLoc = 1; AIDLoc< recordLen; AIDLoc++)
								{
									if(simdataCurr_p->responseApdu[AIDLoc] == (unsigned char)APPLICATIONID)
									{
										printf("\n/Found a valid AID/\n");
										AIDLen = simdataCurr_p->responseApdu[AIDLoc+1];
										memcpy(AID,&simdataCurr_p->responseApdu[AIDLoc+2], sizeof(unsigned char)*AIDLen);
										break;
									}
										
								}
							}
							/*
							3GPP USIM 		'A000000087' '1002' See annex F for further coding details TS 131 102 [11]
							3GPP USIM toolkit 	'A000000087' '1003' See annex G for further coding details TS 131 111 [12]
							3GPP ISIM 		'A000000087' '1004' See
							3GPP2 CSIM 		'A000000343' '1002' see annex F for further coding details
							*/
							
							if((AIDLen != 0)&&
							(AID[4] == 0x87)
							&&(AID[5] == 0x10)
							&&(AID[6] == 0x02))
							{
								printf("\nThis is the USIM application/\n");
								processSelectAIDCmd(hCom, AID, AIDLen,simdataCurr_p);
								/*Save the FCP of select ADF*/
								memset(&currenSimFile, 0x0, sizeof(SimFile));
								currenSimFile.currentDF[0]=0x3F;
							    currenSimFile.currentDF[1]=0x00;
								fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
								if( fcpLen >0 )
								{
									/*simdataCurr->responseApdu contains the FCP and sw*/
									saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
									currenSimFile.fileType = ADF;
								}
								simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
								simdataCurr_p->simFilesNum++;
								printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

								if(simdataCurr_p->pinStatus == USIM_APP_PIN1)
								{
									printf("\nThis USIM is PIN enabled and used, break here!!!!\n");
									simdataCurr_p->usimStatus = USIM_BLOCK;
									return 0;
								}
								
								
							}
						}
					}

#if 0					
					if(ProcessReadCmd(hCom,currentFileType, recordLen, 1, NULL, simdataCurr_p)== TRUE)
					{
						printf("\n/********************Select AID********************/\n");
						unsigned char *AID = new unsigned char[16];
						unsigned int i;
						if(simdataCurr_p->responseApdu[0] == APPLICATIONTEMPLATE)
						{
							for(i = 1; i < recordLen; i++)
							{
								if(simdataCurr_p->responseApdu[i] == APPLICATIONID)
									break;
							}
						}
						memcpy(AID,&simdataCurr_p->responseApdu[i+2], sizeof(unsigned char)*simdataCurr_p->responseApdu[i+1]);
						processSelectAIDCmd(hCom, AID, simdataCurr_p->responseApdu[i+1],simdataCurr_p);
						/*Save the FCP of select ADF*/
						memset(&currenSimFile, 0x0, sizeof(SimFile));
						currenSimFile.currentDF[0]=0x3F;
					    currenSimFile.currentDF[1]=0x00;
						fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
						if( fcpLen >0 )
						{
							/*simdataCurr_p->responseApdu contains the FCP and sw*/
							saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
							currenSimFile.fileType = ADF;
						}
						simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
						simdataCurr_p->simFilesNum++;
						printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);				
					}
					if(simdataCurr_p->pinStatus == USIM_APP_PIN1)
					{
						printf("\nThis USIM is PIN enabled and used, break here!!!!\n");
						simdataCurr_p->usimStatus = USIM_BLOCK;
						return 0;
					}
#endif			
			}
			else
			{
				printf("\n/********************Select DFgsm********************/\n");
				//select DFgsm
				memset(&currenSimFile, 0x0, sizeof(SimFile));
				currenSimFile.currentDF[0]=0x3F;
				currenSimFile.currentDF[1]=0x00;
				fileId[0]= 0x7F;
				fileId[1]= 0x20;
				processSelectCmd(hCom,fileId,TRUE,simdataCurr_p);
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
				simdataCurr_p->simFilesNum++;			
				printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
			}

			/*2015.11.05, mod by Xili for saving file Package to local file, begin*/
			if(readSimFilesFromLocFile(simNum, simdataCurr_p, &iccid[0]) == TRUE)
			{
				if(lastUsimStatus == USIM_ACTIVE)
				{
					simdataCurr_p->usimStatus = USIM_ACTIVE;
				}
				else if(lastUsimStatus == USIM_BLOCK)
				{
					simdataCurr_p->usimStatus = USIM_BLOCK;
				}
                else if(lastUsimStatus == USIM_WAIT_RESET)  //add by 20161025.
                {
                    simdataCurr_p->usimStatus = USIM_WAIT_RESET;
                }
				else
				{
        		    simdataCurr_p->usimStatus = USIM_ON;
				}
                
				printf("\nRecovery SIM files from local file successfully, Usimstatus is %d", simdataCurr_p->usimStatus);
				setCurrentUsimStatus(hCom, simdataCurr_p->simId, simdataCurr_p->usimStatus);
                
				return 1;
			}			
			
			/*2015.11.05, mod by Xili for saving file Package to local file, end*/
			
			printf("\n/********************************Files under DFusim read Begin**************************/\n");

			printf("\n/********************Select EFecc********************/\n");
			//select EFecc
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			
			fileId[0]= 0x6F;
			fileId[1]= 0xB7;
			processSelectCmd(hCom,fileId,TRUE,simdataCurr_p);
			fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
			if( fcpLen >0 )
			{
				/*simdataCurr_p->responseApdu contains the FCP and sw*/
				saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
			}
			printf("\n/********************Read EFiccid********************/\n");
			ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFli********************/\n");
			//select EFli
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x05;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p)== TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFli********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			/*read as the structure of the ADFusim, the sequence may need to be modified as the UE needed*/
			printf("\n/********************Select EFarr********************/\n");
			//select EFarr
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x06;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p)== TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFarr********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
			
			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM2, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM2, next SIM is null ");
			}

			
			printf("\n/********************Select EFimsi********************/\n");
			//select EFimsi
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x07;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFimsi********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
#if 0
				//GETHPLMN from the IMSI value
				unsigned char mcc[3];
				unsigned char mnc[3];
				mcc[0] = (currenSimFile.data[1]&0xF0)>>4;
				mcc[1] = (currenSimFile.data[2]&0x0F);
				mcc[2] = (currenSimFile.data[2]&0xF0)>>4;
				
				simdataCurr_p->hplmn.MCC = mcc[0]*256+mcc[1]*16+mcc[2];
				/*assume the mnc is 2 bits*/
				mnc[0] = 0;
				mnc[1] = (currenSimFile.data[3]&0x0F);
				mnc[2] = (currenSimFile.data[3]&0xF0)>>4;
				simdataCurr_p->hplmn.MNC= mnc[0]*256+mnc[1]*16+mnc[2];
				printf("HPLMN is MCC =%d, MNC =%d\n", simdataCurr_p->hplmn.MCC , simdataCurr_p->hplmn.MNC);
				
#endif				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			/*after IMSI read OK, we can check if this is the same sim under the same SIM loc*/
#if 0
			{
				char fileName[10];
				unsigned int allFileLen =0;
				sprintf(fileName, "%d.txt", simdataCurr_p->simId);
				readFile(fileName,simdataCurr_p->allFileData,32);
				/**/
				allFileLen =((unsigned int)simdataCurr_p->allFileData[1]<<24|
								(unsigned int)simdataCurr_p->allFileData[2]<<16|
								(unsigned int)simdataCurr_p->allFileData[3]<<8|
								(unsigned int)simdataCurr_p->allFileData[4])+1;
				readFile(fileName,simdataCurr_p->allFileData,allFileLen);

				DecodeAllFileData(simdataCurr_p->simFilesNum, simdataCurr_p,allFileLen);
			}
#endif			
			printf("\n/********************Select EFkeys(USIM)/EFKc(SIM)********************/\n");
			//select EFkeys(USIM)/EFKc(SIM)
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
				fileId[0]= 0x6F;
			    fileId[1]= 0x08;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
				fileId[0]= 0x6F;
			    fileId[1]= 0x20;
			}
			
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFkeys(USIM)/EFKc(SIM)********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
		

			
			printf("\n/********************Select EFkesPS********************/\n");
			//select EFkeysps
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x09;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFkeysps********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM3, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM3, next SIM is null ");
			}

			printf("\n/********************Select EFdck********************/\n");
#ifndef SMALL_SIM_PACKAGE			
			//select EFdck  S36
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x2C;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFdck********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
#endif			
			printf("\n/********************Select EFhpplmn********************/\n");
			//select EFhpplmn
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x31;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFhpplmn********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
			
			printf("\n/********************Select EFcnl********************/\n");
			//select EFcnl
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x32;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFicnl********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
#ifndef SMALL_SIM_PACKAGE			
			printf("\n/********************Select EFacmmax********************/\n");
			//select EFacmmax S13
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x37;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFacmax********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
#endif			

			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM4, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM4, next SIM is null ");
			}

			printf("\n/********************Select EFust********************/\n");
			//select EFust
			fileId[0]= 0x6F;
			fileId[1]= 0x38;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFust*******************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);

				/*********************************Disable phonebook****************/
				/*Service n??1:	Local Phone Book*/
				/*Service n??8:	Outgoing Call Information (OCI and OCT)*/
				/*Service n??9:	Incoming Call Information (ICI and ICT)*/
				/*Service n??10:	Short Message Storage (SMS)*/
				/*Service n??11:	Short Message Status Reports (SMSR)*/
				/*Service n??12:	Short Message Service Parameters (SMSP)*/
				/*Service n??13:	Advice of Charge (AoC)*/
				/*Service n??36:	Depersonalisation Control Keys*/
				/*Service n??49:	Call Forwarding Indication Status*/
				/*Service n??52	Multimedia Messaging Service (MMS)*/
				/*Service n??56	Network's indication of alerting in the MS (NIA)*/
				printf("\n\n--EFust: ");
				for(unsigned char i = 0; i<currenSimFile.dataLen; i++)
				{
					printf(" %02X", currenSimFile.data[i]);
				}
				unsigned char serLoc = 0;
				serLoc = 1;
				currenSimFile.data[(serLoc-1)/8] &= ~(0x01<<(serLoc-1)%8); //server 1 PB
				/*
				for(serLoc = 8;	serLoc<currenSimFile.dataLen*8;serLoc++) 
				{
					if(serLoc<=13||(serLoc == 36)||(serLoc == 49)||(serLoc == 52)||(serLoc == 56))
						currenSimFile.data[(serLoc-1)/8] &= ~(0x01<<(serLoc-1)%8); //server 8 OCI and OCT
				}
				*/
				printf("\nEFust--: ");
				for(unsigned char i = 0; i<currenSimFile.dataLen; i++)
				{
					printf(" %02X", currenSimFile.data[i]);
				}
				
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM5, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM5, next SIM is null ");
			}
#ifndef SMALL_SIM_PACKAGE			
			printf("\n/********************Select EFacm********************/\n");
			//select EFacm  S13
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x39;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFacm********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
#endif
			printf("\n/********************Select EFfdn********************/\n");
			//select EFfdn
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x3B;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFfdn********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
//#endif
#ifndef SMALL_SIM_PACKAGE

			printf("\n/********************Select EFsms********************/\n");
			//select EFsms S10
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x3C;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFsms********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
#endif
//#if 0
			printf("\n/********************Select EFgid1********************/\n");
			//select EFgid1
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x3E;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFgid1********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			
			printf("\n/********************Select EFgid2********************/\n");
			//select EFgid2
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x3F;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFgid2********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
//#endif			
			printf("\n/********************Select EFmsisdn********************/\n");
			//select EFmsisdn
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x40;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFmsisdn********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
#ifndef SMALL_SIM_PACKAGE
			printf("\n/********************Select EFpuct********************/\n");
			//select EFpuct  S13
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x41;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFpuct********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
#endif 

#ifndef SMALL_SIM_PACKAGE

			printf("\n/********************Select EFsmsp********************/\n");
			//select EFsmsp S12
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x42;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFsmsp********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFsmss********************/\n");
			//select EFsmss
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x43;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFsms********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
#endif
			printf("\n/********************Select EFcbmi********************/\n");
			//select EFcbmi
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x43;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFcbmi********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFspn********************/\n");
			//select EFspn
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x46;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFspn********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

#ifndef SMALL_SIM_PACKAGE
			printf("\n/********************Select EFsmsr********************/\n");
			//select EFsmsr S11
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x47;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFsmsr********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
#endif			
			printf("\n/********************Select EFcbmid********************/\n");
			//select EFcbmid
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x48;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFcbmid********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFsdn********************/\n");
			//select EFsdn
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x49;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFsdn********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFext2********************/\n");
			//select EFext2
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x4B;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFext2********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				

			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFext3********************/\n");
			//select EFext3
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x4C;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFext3********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFbdn********************/\n");
			//select EFbdn
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x4D;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}				
				printf("\n/********************Read EFbdn********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFext5********************/\n");
			//select EFext5
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x4E;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFext5********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
			

			printf("\n/********************Select EFccp2********************/\n");
			//select EFccp2
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x4F;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFccp2********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFcbmir********************/\n");
			//select EFcbmir
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x50;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFcbmir********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

            /*2016.04.13, mod by Xili for adding read EFKcGPRS/EFLOCIGPRS, begin*/
			
			if(simdataCurr_p->isUsim != TRUE)
			{	
				printf("\n/********************Select EFKcGPRS********************/\n");
				//select EFKcGPRS
				memset(&currenSimFile, 0x0, sizeof(SimFile));
				
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
				fileId[0]= 0x6F;
				fileId[1]= 0x52;
				if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
				{
					fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
					if( fcpLen >0 )
					{
						/*simdataCurr_p->responseApdu contains the FCP and sw*/
						saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
					}
					printf("\n/********************Read EFKcGPRS********************/\n");
					ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
					
				}
				else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
				{
					currenSimFile.fileId[0]= fileId[0];
					currenSimFile.fileId[1]= fileId[1];
				}
				simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
				simdataCurr_p->simFilesNum++;
				printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
				

				printf("\n/********************Select EFLOCIGPRS********************/\n");
				//select EFLOCIGPRS
				memset(&currenSimFile, 0x0, sizeof(SimFile));
				
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
				fileId[0]= 0x6F;
				fileId[1]= 0x53;
				if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
				{
					fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
					if( fcpLen >0 )
					{
						/*simdataCurr_p->responseApdu contains the FCP and sw*/
						saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
					}
					printf("\n/********************Read EFLOCIGPRS********************/\n");
					ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
					
				}
				else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
				{
					currenSimFile.fileId[0]= fileId[0];
					currenSimFile.fileId[1]= fileId[1];
				}
				simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
				simdataCurr_p->simFilesNum++;
				printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
				
			}
            /*2016.04.13, mod by Xili for adding read EFKcGPRS/EFLOCIGPRS, end*/
			
			printf("\n/********************Select EFext4********************/\n");
			//select EFext4
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x55;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFext4********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM6, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM6, next SIM is null ");
			}

			printf("\n/********************Select EFest********************/\n");
			//select EFest
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x56;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFest********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFacl********************/\n");
			//select EFacl
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x57;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFacl********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFcmi********************/\n");
			//select EFcmi
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x58;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFcmi********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFstart-hfn********************/\n");
			//select EFstart-hfn
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x5B;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFstarf-hfn********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM7, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM7, next SIM is null ");
			}
			
			printf("\n/********************Select EFthreshold********************/\n");
			//select EFthreshold
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x5C;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFthreshold********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
//#endif
			printf("\n/********************Select EFplmnwact********************/\n");
			//select EFplmnwact
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x60;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFplmnwact********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
                if (recordLen <= 255)  //mdf by ml.20161114.
                {
                    simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
                    simdataCurr_p->simFilesNum++;
                    printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
                }
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}

			printf("\n/********************Select EFoplmnwact********************/\n");
			//select EFoplmnwact
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x61;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen > 0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFoplmnwact********************/\n");
				printf("recordLen = %d \n", recordLen);
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				if (recordLen <= 255)  //mdf by ml.20160824.
				{
					simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
					simdataCurr_p->simFilesNum++;
					printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
				}
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			
			
			printf("\n/********************Select EFhplmnwact********************/\n");
			//select EFhplmnwact
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x62;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFhplmnwact********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFpsloci********************/\n");
			//select EFpsloci
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x73;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFpuct********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM8, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM8, next SIM is null ");
			}

			printf("\n/********************Select EFacc********************/\n");
			//select EFacc
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x78;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFacc********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM81, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM82, next SIM is null ");
			}

			printf("\n/********************Select EFfplmn********************/\n");
			//select EFfplmn
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x7B;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFfplmn********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM83, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM83, next SIM is null ");
			}

			printf("\n/********************Select EFloci********************/\n");
			//select EFloci
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x7E;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFloci********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				if(currenSimFile.dataLen== 11) //EFloci with 11 Bytes
					currenSimFile.data[10] = 0x01;  //NOT UPDATED
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM84, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM84, next SIM is null ");
			}
			
#ifndef SMALL_SIM_PACKAGE
			printf("\n/********************Select EFici********************/\n");
			//select EFici S9
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x80;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFici********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFoci********************/\n");
			//select EFoci S8
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x81;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFoci********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			

			printf("\n/********************Select EFict********************/\n");
			//select EFict
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0x82;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFoct********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
#endif

		if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM85, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM85, next SIM is null ");
			}
			printf("\n/********************Select EFad********************/\n");
			//select EFad
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xAD;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFad********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

		if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM86, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM86, next SIM is null ");
			}
			printf("\n/********************Select EFvgcs********************/\n");
			//select EFvgcs
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xB1;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFvgcs********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM87, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM87, next SIM is null ");
			}

			printf("\n/********************Select EFvgcss********************/\n");
			//select EFvgcss
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xB2;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFcgcss********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM88, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM88, next SIM is null ");
			}
			
			printf("\n/********************Select EFvbs********************/\n");
			//select EFvbs
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xB3;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFvbs********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM89, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM89, next SIM is null ");
			}

			printf("\n/********************Select EFvbss********************/\n");
			//select EFvbss
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xB4;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFvbss********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM10, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM10, next SIM is null ");
			}
			printf("\n/********************Select EFemlpp********************/\n");
			//select EFemlpp
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xB5;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFemlpp********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFaaem********************/\n");
			//select EFaaem
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xB6;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFaaem********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
//#endif
			printf("\n/********************Select EFecc********************/\n");
			//select EFecc
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xB7;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFecc********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
			
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
//#if 0
			printf("\n/********************Select EFhiddenkey********************/\n");
			//select EFhidenkey
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xC3;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFhidenkey********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
//#endif
			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM11, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM11, next SIM is null ");
			}
			printf("\n/********************Select EFnetpar********************/\n");
			//select EFnetpar
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xC4;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFnetpar********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFpnn********************/\n");
			//select EFpnn
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xC5;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFpnn********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFopl********************/\n");
			//select EFopl
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xC6;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFopl********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM12, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM12, next SIM is null ");
			}
//#if 0
			printf("\n/********************Select EFmbdn********************/\n");
			//select EFmbdn
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xC7;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFmbdn********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFext6********************/\n");
			//select EFext6
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xC8;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFext6********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFmbi********************/\n");
			//select EFmbi
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xC9;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFmbi********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFmwis********************/\n");
			//select EFmwis
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xCA;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFmwis********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
#ifndef SMALL_SIM_PACKAGE
			printf("\n/********************Select EFcfis********************/\n");
			//select EFcfis S49
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xCB;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFcfis********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFext7********************/\n");
			//select EFext7
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xCC;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFext7********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
#endif
			printf("\n/********************Select EFspdi********************/\n");
			//select EFspdi
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xCD;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFspdi********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM13, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM13, next SIM is null ");
			}
#ifndef SMALL_SIM_PACKAGE
			printf("\n/********************Select EFmmsn********************/\n");
			//select EFmmsn S52
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xCE;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFmmsn********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFext8********************/\n");
			//select EFext8
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xCF;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFext8********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}

			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFmmsicp********************/\n");
			//select EFecc
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xD0;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFmmsicp********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFmmsup********************/\n");
			//select EFmmsup
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xD1;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFmmsup********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFmmsucp********************/\n");
			//select EFmmsucp
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xD2;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFmmsucp********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
#endif

#ifndef SMALL_SIM_PACKAGE
			printf("\n/********************Select EFnia********************/\n");
			//select EFnia S56
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xD3;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFnia********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
#endif
			printf("\n/********************Select EFvgcsca********************/\n");
			//select EFvgcsca
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xD4;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFvgcsca********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFvbsca********************/\n");
			//select EFvbsca
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xD5;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFvbsca********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFgbap********************/\n");
			//select EFgbap
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xD6;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFgbap********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM14, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM14, next SIM is null ");
			}

			printf("\n/********************Select EFmsk********************/\n");
			//select EFmsk
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xD7;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFmsk********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFmuk********************/\n");
			//select EFmuk
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xD8;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFmuk********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
//#endif
			printf("\n/********************Select EFehplmn********************/\n");
			//select EFehplmn
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xD9;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFehplmn********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
//#if 0
			printf("\n/********************Select EFgbanl********************/\n");
			//select EFgbanl
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xDA;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFgbanl********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFehplmnpi********************/\n");
			//select EFehplmnpi
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xDB;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFehplmnpi********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFlrplmnsi********************/\n");
			//select EFlrplmnsi
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xDC;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFlrplmnsi********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM15, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM15, next SIM is null ");
			}

			printf("\n/********************Select EFnafkca********************/\n");
			//select EFnafkca
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xDD;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFnafkca********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFspni********************/\n");
			//select EFspni
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xDE;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFspni********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFpnni********************/\n");
			//select EFpnni
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xDF;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFpnni********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFncp-ip********************/\n");
			//select EFncp-ip
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xE2;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFncp-ip********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
//#endif
			printf("\n/********************Select EFepsloci********************/\n");
			//select EFepsloci
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xE3;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFepsloci********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				if(currenSimFile.dataLen== 18) //EFepsloci with 18 Bytes
					currenSimFile.data[17] = 0x01;  //NOT UPDATED
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFepsnsc********************/\n");
			//select EFepsnsc
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xE4;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFepsnsc********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}

			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
//#if 0
			printf("\n/********************Select EFufc********************/\n");
			//select EFufc
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xE6;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFufc********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);

			printf("\n/********************Select EFuicciari********************/\n");
			//select EFuicciari
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xE7;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFuicciari********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
			
//#endif

			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM16, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM16, next SIM is null ");
			}
			printf("\n/********************Select EFnasconfig********************/\n");
			//select EFnasconfig
			memset(&currenSimFile, 0x0, sizeof(SimFile));
			if(simdataCurr_p->isUsim == TRUE)
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0xFF;
			}
			else
			{
				currenSimFile.currentDF[0]=0x7F;
				currenSimFile.currentDF[1]=0x20;
			}
			fileId[0]= 0x6F;
			fileId[1]= 0xE8;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				fcpLen = processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				if( fcpLen >0 )
				{
					/*simdataCurr_p->responseApdu contains the FCP and sw*/
					saveFcp(simdataCurr_p->responseApdu,&currenSimFile,fcpLen, currentFileType, recordLen, recordNum,simdataCurr_p);	
				}
				printf("\n/********************Read EFnasconfig********************/\n");
				ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, &currenSimFile, simdataCurr_p);
				
			}
			else if(simdataCurr_p->simstatus == SIM_FILE_NOT_FOUND)
			{
				currenSimFile.fileId[0]= fileId[0];
				currenSimFile.fileId[1]= fileId[1];
			}
			simdataCurr_p->simfile[simdataCurr_p->simFilesNum] = currenSimFile;
			simdataCurr_p->simFilesNum++;
			printf("This is simFilesNum = %d \n", simdataCurr_p->simFilesNum);
			

			printf("/********************************Files under DFusim read done**************************/\n");
			
			printf("\n/********************************DFs under DFusim read, begin**************************/\n");
	
			printf("\n/********************************DFphonebook, ingore**************************/\n");

			
			printf("\n/********************************DFgsm-access, Begin**************************/\n");
#if 0
			//select MF
			printf("\n/********************Select MF********************/\n");
			fileId[0]= 0x3F;
			fileId[1]= 0x00;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
			}
			//select DFgsm-access
			printf("\n/********************Select DFgsm-access********************/\n");
			fileId[0]= 0x5F;
			fileId[1]= 0x3B;
			if((processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)&&
				(processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum)))
			{
				printf("\n/********************Read files under DFgsm-access********************/\n");
			}
			else
			{
				printf("\n/********************DFgsm-access does not exist********************/\n");
			}
			printf("\n/********************************DFgsm-access, End**************************/\n");
			

			printf("\n/********************************DFmexe, Begin**************************/\n");
			//select MF
			printf("\n/********************Select MF********************/\n");
			fileId[0]= 0x3F;
			fileId[1]= 0x00;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
			
				//select DFmexe
				printf("\n/********************Select DFmexe********************/\n");
				fileId[0]= 0x5F;
				fileId[1]= 0x3C;
				if((processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)&&
					(processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum)))
				{
					printf("\n/********************Read files under DFmexe********************/\n");
				}
				else
				{
					printf("\n/********************DFmexe does not exist********************/\n");
				}
			}
			printf("\n/********************************DFmexe, End**************************/\n");

			

			printf("\n/********************************DFsolsa, Begin**************************/\n");
			//select MF
			printf("\n/********************Select MF********************/\n");
			fileId[0]= 0x3F;
			fileId[1]= 0x00;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
			
				//select DFsolsa
				printf("\n/********************Select DFsolsa********************/\n");
				fileId[0]= 0x5F;
				fileId[1]= 0x70;
				if((processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)&&
					(processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum)))
				{
					printf("\n/********************Read files under DFmexe********************/\n");
				}
				else
				{
					printf("\n/********************DFsolsa does not exist********************/\n");
				}
				
			}
			printf("\n/********************************DFsolsa, End**************************/\n");

			printf("\n/********************************DFhnb, Begin**************************/\n");
			//select MF
			printf("\n/********************Select MF********************/\n");
			fileId[0]= 0x3F;
			fileId[1]= 0x00;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				//select DFhnb
				printf("\n/********************Select DFhnb********************/\n");
				fileId[0]= 0x5F;
				fileId[1]= 0x50;
				if((processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)&&
					(processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum)))
				{
					printf("\n/********************Read files under DFhnb********************/\n");
				}
				else
				{
					printf("\n/********************DFhnb does not exist********************/\n");
				}
			}
			printf("\n/********************************DFhnb, End**************************/\n");


			printf("\n/********************************Back to ADFusim, which is the active application, Begin**************************/\n");
			//select MF
			printf("\n/********************Select MF********************/\n");
			fileId[0]= 0x3F;
			fileId[1]= 0x00;
			if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
			{
				processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
				//select DFhnb
				printf("\n/********************Select ADFusim********************/\n");
				fileId[0]= 0x7F;
				fileId[1]= 0xFF;
				if((processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)&&
					(processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum)))
				{
					printf("/********************Back to ADF usim application********************/\n");
					printf("/********************Select EFimsi for test ********************/\n");
					//select EFimsi
					fileId[0]= 0x6F;
					fileId[1]= 0x07;
					if(processSelectCmd(hCom,fileId,TRUE,simdataCurr_p) == TRUE)
					{
						processFcp(simdataCurr_p->responseApdu, &currentFileType,&recordLen,&recordNum,simdataCurr_p);
						printf("/********************Read EFimsi********************/\n");
						ProcessReadCmd(hCom,currentFileType, recordLen, recordNum, NULL);

						printf("/********************Back to ADF usim application Success!!********************/\n");
					}
				}
				else
				{
					printf("/********************Select ADFusim fail********************/\n");
				}
			}
			printf("\n/********************************Back to ADFusim, which is the active application, End**************************/\n");

#endif

#if 0
			
			/*print the FCP and data each read file and delete the allocate memory*/
			printf("\n/********************print the FCP and data each read file and delete the allocate memory, Begin********************/\n");
			for(unsigned char filenum = 0; filenum < simdataCurr_p->simFilesNum; filenum++)
			{
				printf("\n\n/*****%dst FILEID=%02X%02X, FILETYPE=%d, recordLen= %d, recordNum =%d, dataLen=%d*****/\n", 
						(filenum+1),
						simdataCurr_p->simfile[filenum].fileId[0],
						simdataCurr_p->simfile[filenum].fileId[1],
						simdataCurr_p->simfile[filenum].fileType,
						simdataCurr_p->simfile[filenum].recordLen,
						simdataCurr_p->simfile[filenum].recordNum,
						simdataCurr_p->simfile[filenum].dataLen);
				
				if(simdataCurr_p->simfile[filenum].fcpLen > 0)
				{
					printf("-----FCP data:\n");
					for(unsigned int fcpNum = 0; fcpNum < simdataCurr_p->simfile[filenum].fcpLen; fcpNum++)
					{
						printf(" %02X", simdataCurr_p->simfile[filenum].fcp[fcpNum]);
					}
					//delete simdataCurr_p->simfile[filenum].fcp;
					//simdataCurr_p->simfile[filenum].fcp = NULL;
				}
				else
				{
					printf("\n-----NO File FCP: The File not exist:\n");
				}
				
				if(simdataCurr_p->simfile[filenum].dataLen > 0)
				{
					printf("\n-----File data:\n");
					for(unsigned int dataNum = 0; dataNum < simdataCurr_p->simfile[filenum].dataLen; dataNum++)
					{
						printf(" %02X", simdataCurr_p->simfile[filenum].data[dataNum]);
						if(simdataCurr_p->simfile[filenum].fileType == LINEARFILE)
						{
							if((dataNum+1)%simdataCurr_p->simfile[filenum].recordLen ==0)
							{	
								printf("\n");
							}
						}
					}
					//delete simdataCurr_p->simfile[filenum].data;
					//simdataCurr_p->simfile[filenum].data = NULL;
				}
				else
				{
					printf("\n-----NO File data: MF DF or File not exist\n");
				}
				
				
			}
			printf("\n/********************print the FCP and data each read file and delete the allocate memory, End********************/\n");
#endif
			printf("\n/********************Test Auth Command, Begin********************/\n");
			//testAuthCmd(hCom);
			printf("\n/********************Test Auth Command, End  ********************/\n");
			if(simdataCurr_p->next_SIM != NULL)
			{
				printf("\nactiveAndReadSIM17, next SIM is not null ");
			}
			else
			{
				printf("\nactiveAndReadSIM17, next SIM is null ");
			}
			simdataCurr_p->usimStatus = USIM_ON;
			setCurrentUsimStatus(hCom, simdataCurr_p->simId, simdataCurr_p->usimStatus);

			/*2015.11.05, mod by Xili for saving file Package to local file, begin*/
			saveSimFilesOnLocFile(simNum, simdataCurr_p, iccid);
			/*2015.11.05, mod by Xili for saving file Package to local file, end*/

			
		}
	
	return 1;
}

 int setBandrate(HANDLE hcom)
 {

	if(hcom!= NULL)
	{
		bool bandRateSetSucc = false;
		//for(unsigned char simNum = 1; simNum <= ALLSIMNUM/3; simNum++ )
		for(unsigned char simNum = 1; simNum <= pMaxSimNum/3; simNum++ )  //mdy by ml.20160804.
		{
			//00 4800 01 9600 02 14400 03 19200 04 38400
			BYTE setBandRate[10] = {0xaa,0xbb,0x06,0x00,0x00,0x00,0x01,0x01,0x04,0x51}; //set the first MCU 14400
			BYTE RecvData[267] = {0};
			unsigned char setBandRateRsp[10] = {0};
			unsigned char atrLen = 0;
			bool checkresult = false;
			unsigned int realReceiveLeg = 0;
			setBandRate[5] |= simNum;
			//setBandRate[8] |= bandRate;
		  	if(Send_driver_T(hcom, setBandRate, 0x0A))
			{
				 printf("setBandrate send fail\n");
			}
		  	else
			{
				 printf("setBandrate send succ\n");
			}
		 // Sleep(100);

		  	if(Receive_driver_T(hcom, RecvData, &realReceiveLeg))
			{
				 printf("setBandrate recv fail\n");
			}
		 	else
		  	{
				 printf("setBandrate recv succ\n");		
				
				 bandRateSetSucc = true;
		  	}
		  //Sleep(100);
		
		}
		 //??????????
		 if(bandRateSetSucc == true)
		 {
		 	 DCB dcb = {0};
		   if (!GetCommState(hcom,&dcb))
		   {
				   printf("GetCommState fail\n");
				   return 1;
		   }

		   dcb.DCBlength = sizeof(dcb);
		   
		   if (!BuildCommDCB("9600,n,8,1",&dcb))/*(!BuildCommDCB("9600,n,8,1",&dcb))*/
	//????????????????????У???????????λ????λ
		   {
				   printf("BuileCOmmDCB fail\n");
				   CloseHandle(hcom);
				   return 1;
		   }

		   if(SetCommState(hcom,&dcb))
		   {
				  printf("SetCommState OK!\n");
				  return 0;
		   }
		 }
		  
		
	 }
	return 0;
}

 BOOL CtrlHandler( DWORD fdwCtrlType )
{
 switch( fdwCtrlType )
 {
  // Handle the CTRL-C signal.
 case CTRL_C_EVENT:
  printf( "Ctrl-C event\n\n" );
  //Led1 on, led2 off
  //setLedStatus(hComa, 0x01);
  return( TRUE );
  // CTRL-CLOSE: confirm that the user wants to exit.
 case CTRL_CLOSE_EVENT:
 
  printf( "Ctrl-Close event\n\n" );
  return( TRUE );
  // Pass other signals to the next handler.
 case CTRL_BREAK_EVENT:

  printf( "Ctrl-Break event\n\n" );
  return FALSE;
 case CTRL_LOGOFF_EVENT:
 
  printf( "Ctrl-Logoff event\n\n" );
  return FALSE;
 case CTRL_SHUTDOWN_EVENT:
   printf( "Ctrl-Shutdown event\n\n" );
  return FALSE;
 default:
  return FALSE;
 }
}

long	__stdcall	ExceptionCallback(_EXCEPTION_POINTERS*	excp)	
 {	 
   
 printf("Error	 address   %x/n",excp->ExceptionRecord->ExceptionAddress);	 
 printf("CPU   register:/n");	
 printf("eax   %x	ebx   %x   ecx	 %x   edx	%x/n",excp->ContextRecord->Eax,   
 excp->ContextRecord->Ebx,excp->ContextRecord->Ecx,   
 excp->ContextRecord->Edx);   
 return   EXCEPTION_EXECUTE_HANDLER;   
 }


/*2015.11.05, mod by Xili for saving file Package to local file, begin*/
#if 0
unsigned char CharToHex(unsigned char bHex)
{
if((bHex>=0)&&(bHex<=9))
bHex += 0x30;
else if((bHex>=10)&&(bHex<=15))//??д???
bHex += 0x37;
else bHex = 0xff;
return bHex;
}
#endif
void setSimId2FileName(unsigned char simID, char *fname)
{
	
	if((simID >= 0) && (simID <= 9))
	{
		fname[3] = 0x30;
		fname[4] = 0x30;
		fname[5] = simID + 0x30;
	}	
	else if(simID <= 19)
	{
		fname[3] = 0x30;
		fname[4] = 1 + 0x30;
		fname[5] = (simID - 10) + 0x30;
	}
	else if(simID <= 29)
	{
		fname[3] = 0x30;
		fname[4] = 2 + 0x30;
		fname[5] = (simID - 20) + 0x30;
	}
	else if(simID <= 39)
	{
		fname[3] = 0x30;
		fname[4] = 3 + 0x30;
		fname[5] = (simID - 30) + 0x30;
	}
	else if(simID <= 49)
	{
		fname[3] = 0x30;
		fname[4] = 4 + 0x30;
		fname[5] = (simID - 40) + 0x30;
	}
	else if(simID <= 59)
	{
		fname[3] = 0x30;
		fname[4] = 5 + 0x30;
		fname[5] = (simID - 50) + 0x30;
	}
	else if(simID <= 69)
	{
		fname[3] = 0x30;
		fname[4] = 6 + 0x30;
		fname[5] = (simID - 60) + 0x30;
	}
	else if(simID <= 79)
	{
		fname[3] = 0x30;
		fname[4] = 7 + 0x30;
		fname[5] = (simID - 70) + 0x30;
	}
	else if(simID <= 89)
	{
		fname[3] = 0x30;
		fname[4] = 8 + 0x30;
		fname[5] = (simID - 80) + 0x30;
	}
	else if(simID <= 99)
	{
		fname[3] = 0x30;
		fname[4] = 9 + 0x30;
		fname[5] = (simID - 90) + 0x30;
	}
	else if(simID <= 109)
	{
		fname[3] = 1+0x30;
		fname[4] = 0x30;
		fname[5] = (simID - 100) + 0x30;
	}
	else if(simID <= 119)
	{
		fname[3] = 1+0x30;
		fname[4] = 1+0x30;
		fname[5] = (simID - 110) + 0x30;
	}
	else if(simID <= 129)
	{
		fname[3] = 1+0x30;
		fname[4] = 2+0x30;
		fname[5] = (simID - 120) + 0x30;
	}
	else if(simID <= 139)
	{
		fname[3] = 1+0x30;
		fname[4] = 3+0x30;
		fname[5] = (simID - 130) + 0x30;
	}
	else if(simID <= 149)
	{
		fname[3] = 1+0x30;
		fname[4] = 4+0x30;
		fname[5] = (simID - 140) + 0x30;
	}
	else if(simID <= 159)
	{
		fname[3] = 1+0x30;
		fname[4] = 5+0x30;
		fname[5] = (simID - 150) + 0x30;
	}
	else if(simID <= 169)
	{
		fname[3] = 1+0x30;
		fname[4] = 6+0x30;
		fname[5] = (simID - 160) + 0x30;
	}
	else if(simID <= 179)
	{
		fname[3] = 1+0x30;
		fname[4] = 7+0x30;
		fname[5] = (simID - 170) + 0x30;
	}
	else if(simID <= 189)
	{
		fname[3] = 1+0x30;
		fname[4] = 8+0x30;
		fname[5] = (simID - 180) + 0x30;
	}
	else
	{
		printf("\nSIM ID is not exit or TBD!!!");
	}
	
}


void saveSimFilesOnLocFile(unsigned char simID, SimData *simdataCurr_p, unsigned char *iccid)
{
	
    unsigned int fileStrLen = 0;
	unsigned char buffer[MAXSENDNUM];
	//unsigned char simid;
	unsigned int filePackage_loc = 0;
	unsigned int buffer_loc = 0;
	FILE * ssfp = NULL;		
	char fname[] = "SIMxyz.txt";
	setSimId2FileName(simID, fname);				
	if((ssfp=fopen(fname,"w+"))==NULL)		  /*Open the file and clean the file*/
	{
	    printf("\nsim%d saved file is not exit", simID);	    
	}
    else    
    {
	    /*Need open the file with binary format to avoid get unexpected data*/
		fclose (ssfp);		
		if((ssfp=fopen(fname,"wb"))==NULL)	
		{
			printf("\nsim%d saved file open failed\n", simID);
			return ;
		}
	    printf("\nStart to saved file for sim%d\n", simID);
		
	    /*keep for the buffer length, 4 bytes*/
	    buffer[filePackage_loc++] = 0;
 		buffer[filePackage_loc++] = 0;
 		buffer[filePackage_loc++] = 0;
 		buffer[filePackage_loc++] = 0;

		/*Save ICCID to identify the SIM card*/
		for(unsigned int i = 0; i< ICCID_LEN; i++)
		{
			buffer[filePackage_loc++] = iccid[i];
			printf(" ICCID:%02X", iccid[i]);
		}

		/*save simFilesNum*/
        buffer[filePackage_loc++] = simdataCurr_p->simFilesNum;
		printf("\n simFilesNum:%d", simdataCurr_p->simFilesNum);
		
		/*Save SIM files*/
        for(unsigned char fileNum = 0; fileNum < SIMFILENUM; fileNum++)
        {
			
			printf("\n file%d exit, file ID is 0x%02x %02x ", fileNum, simdataCurr_p->simfile[fileNum].fileId[0], simdataCurr_p->simfile[fileNum].fileId[1]);
			printf("\n file%d begin, filePackage_loc is %d", fileNum, filePackage_loc );
            
			/*fileStrlen 4 bytes*/
			/*Caculate: DF/2bytes fileId/2 Ftype/1 rLen/2 rNum/1 FcpLen/2 + fcp[]/FcpLen datalen/4 data[]/datalen*/
			fileStrLen = 2 +  2 +  1 +  2 + 1 + 2 +simdataCurr_p->simfile[fileNum].fcpLen + 4 + simdataCurr_p->simfile[fileNum].dataLen;
			printf("\n file%d StrLen is %02x", fileNum, fileStrLen);			
			buffer[filePackage_loc++] = (fileStrLen&0xFF000000)>>24;
 			buffer[filePackage_loc++] = (fileStrLen&0x00FF0000)>>16;
 			buffer[filePackage_loc++] = (fileStrLen&0x0000FF00)>>8;
 			buffer[filePackage_loc++] = (fileStrLen&0x000000FF);
            
			/*currentDF*/
			buffer[filePackage_loc++] = simdataCurr_p->simfile[fileNum].currentDF[0];
			buffer[filePackage_loc++] = simdataCurr_p->simfile[fileNum].currentDF[1];

			/*fileId*/	
			buffer[filePackage_loc++] = simdataCurr_p->simfile[fileNum].fileId[0];
			buffer[filePackage_loc++] = simdataCurr_p->simfile[fileNum].fileId[1];

			/*fileType */
			buffer[filePackage_loc++] = simdataCurr_p->simfile[fileNum].fileType;	

			/*recordLen[2]*/
			buffer[filePackage_loc++] = (simdataCurr_p->simfile[fileNum].recordLen&0xFF00)>>8;
			buffer[filePackage_loc++] = (simdataCurr_p->simfile[fileNum].recordLen&0x00FF);

			/*recordNum*/
			/*
			buffer[filePackage_loc++] = (simdataCurr_p->simfile[fileNum].recordNum&0xFF00)>>8;
			buffer[filePackage_loc++] = (simdataCurr_p->simfile[fileNum].recordNum&0x00FF);
			*/
			buffer[filePackage_loc++] = simdataCurr_p->simfile[fileNum].recordNum;

			/*FCP FcpLen[2] */
			buffer[filePackage_loc++] = (simdataCurr_p->simfile[fileNum].fcpLen&0x0000FF00)>>8;
			buffer[filePackage_loc++] = (simdataCurr_p->simfile[fileNum].fcpLen&0x000000FF);
			/*fcp[] */
			if(simdataCurr_p->simfile[fileNum].fcpLen > 0)
			{
				memcpy(&buffer[filePackage_loc], simdataCurr_p->simfile[fileNum].fcp, simdataCurr_p->simfile[fileNum].fcpLen);
				filePackage_loc +=  simdataCurr_p->simfile[fileNum].fcpLen;
			}			

			/*DATA dataLen[2]*/
			buffer[filePackage_loc++] = (simdataCurr_p->simfile[fileNum].dataLen&0xFF000000)>>24;
			buffer[filePackage_loc++] = (simdataCurr_p->simfile[fileNum].dataLen&0x00FF0000)>>16;
			buffer[filePackage_loc++] = (simdataCurr_p->simfile[fileNum].dataLen&0x0000FF00)>>8;
			buffer[filePackage_loc++] = (simdataCurr_p->simfile[fileNum].dataLen&0x000000FF);					
			/*data[]*/
			if(simdataCurr_p->simfile[fileNum].dataLen > 0)
			{
				memcpy(&buffer[filePackage_loc], simdataCurr_p->simfile[fileNum].data, simdataCurr_p->simfile[fileNum].dataLen);
				filePackage_loc +=  simdataCurr_p->simfile[fileNum].dataLen;
			}
			
			printf("\n @file%d Ending, filePackage_loc is %d, fileStrLen is %d\n", fileNum, filePackage_loc, fileStrLen);					
			
	    }
		
        filePackage_loc++;/*Record the buffer data length*/
		buffer[buffer_loc++] = (filePackage_loc&0xFF000000)>>24;
 		buffer[buffer_loc++] = (filePackage_loc&0x00FF0000)>>16;
 		buffer[buffer_loc++] = (filePackage_loc&0x0000FF00)>>8;
 		buffer[buffer_loc++] = (filePackage_loc&0x000000FF);

		printf("\nsim%d saved file parameters:", simID);
		for(unsigned int j = 0; j< filePackage_loc; j++)
		{
			printf(" %02X", (unsigned char)buffer[j]);
		}

	    if(fwrite (buffer , sizeof(unsigned char), filePackage_loc, ssfp ) == 0)
	    {
	        printf("\nsim%d saved file write failed", simID);
	    }
		
        fclose (ssfp);
	
    
	}	 				

}

BOOL readSimFilesFromLocFile(unsigned char simID, SimData *simdataCurr_p, unsigned char *iccid)
{
    unsigned int bufferLenTmp;
	unsigned int bufferLen;
	unsigned int fileStrLen;	
	unsigned char temp[4];
	unsigned char iccidLoc[ICCID_LEN];
	unsigned char buffer[MAXSENDNUM];
	//unsigned char bufferTmp[800];
	unsigned int filePackage_loc = 0;
	FILE * ssfp = NULL;		
	char fname[] = "SIMxyz.txt";
	setSimId2FileName(simID, fname);	
	memset(buffer, 0, MAXSENDNUM);
	if((ssfp=fopen(fname,"rb"))==NULL)		  /*??????????*/
	{
	    printf("\nsim%d saved file is not exit", simID);
		return FALSE;
	    
	}
	else
	{
		printf("\nsim%d saved file exit, start read files: \n", simID);
        
		/*Read the buffer length, 4 bytes*/
		for(unsigned int i = 0; i< 4; i++)
		{
			fread(&temp[i], sizeof(unsigned char), 1,ssfp);
		}
		bufferLen = (temp[0]<<24 | temp[1]<<16 | temp[2]<<8 | temp[3]);
		printf("\n bufferLen: %02x\n", bufferLen);

		fseek(ssfp, 0, SEEK_SET);
		/*Read remian data, and the length is bufferLen*/
		
		bufferLenTmp = fread(buffer, sizeof(unsigned char), bufferLen,ssfp);
		if(bufferLenTmp != bufferLen)
		{
			printf("\n Read file failed, bufferLenTmp:0x%02x, bufferLen:0x%02x\n", bufferLenTmp, bufferLen);
			return FALSE;
		}       
		
        fclose (ssfp);

		printf("\nsim%d read file parameters:\n", simID);
		for(unsigned int i = 0; i< bufferLen; i++)
		{
			
			printf(" %02X", buffer[i]);
		}
		
		filePackage_loc +=4;
		/*ICCID*/
        printf("\n compare the ICCID between local saved file and SIM file\n");
		for(unsigned int i = 0; i< ICCID_LEN; i++)
		{
			iccidLoc[i] = buffer[filePackage_loc++];
			printf(" iccidLoc:%02x, iccid: %02x\n", iccidLoc[i], iccid[i]);
		}
		
		if(memcmp(iccid, iccidLoc, ICCID_LEN) != 0)
		{
			return FALSE;
		}

		/*simFilesNum*/
		simdataCurr_p->simFilesNum = buffer[filePackage_loc++];
		printf("\nsim%d file number:%02x\n", simID, simdataCurr_p->simFilesNum);		
		
		for(unsigned char fileNum = 0; fileNum < SIMFILENUM; fileNum++)
		{
			printf("\n file%d begin, filePackage_loc is %02x", fileNum, filePackage_loc );
    
  			fileStrLen = ((buffer[filePackage_loc]<<24) | (buffer[filePackage_loc+1]<<16) | (buffer[filePackage_loc+2]<<8) | buffer[filePackage_loc+3]);
			filePackage_loc +=4;
			printf("\n file%d strlength: %02x", fileNum, fileStrLen);
			
			if(filePackage_loc <= bufferLen)
			{
				/*currentDF*/
				simdataCurr_p->simfile[fileNum].currentDF[0] = buffer[filePackage_loc++];
				simdataCurr_p->simfile[fileNum].currentDF[1] = buffer[filePackage_loc++];

				/*fileId*/				
				simdataCurr_p->simfile[fileNum].fileId[0] = buffer[filePackage_loc++];
				simdataCurr_p->simfile[fileNum].fileId[1] = buffer[filePackage_loc++];
				printf("\n SIM file%d and file ID is 0x%02x %02x, file num:%d ", fileNum, simdataCurr_p->simfile[fileNum].fileId[0], simdataCurr_p->simfile[fileNum].fileId[1], fileNum);

				/*fileType */
				simdataCurr_p->simfile[fileNum].fileType = (FileType)buffer[filePackage_loc++];	

				/*recordLen[2]*/
				simdataCurr_p->simfile[fileNum].recordLen = ((buffer[filePackage_loc]<<8) | buffer[filePackage_loc+1]);	
				filePackage_loc++;
				filePackage_loc++;

				/*recordNum*/
				simdataCurr_p->simfile[fileNum].recordNum = buffer[filePackage_loc++];			

				/*FCP FcpLen[2] */
				simdataCurr_p->simfile[fileNum].fcpLen = ((buffer[filePackage_loc]<<8) | buffer[filePackage_loc+1]);
				printf("\nsimdataCurr_p->simfile[%d].fcpLen:%02x ", fileNum, simdataCurr_p->simfile[fileNum].fcpLen);
				filePackage_loc++;
				filePackage_loc++;
				
				/*fcp[] */
				if(simdataCurr_p->simfile[fileNum].fcpLen != 0)
				{
				    printf("\n filePackage_loc is %02x\n", filePackage_loc);
					simdataCurr_p->simfile[fileNum].fcp = new unsigned char[simdataCurr_p->simfile[fileNum].fcpLen];
					memcpy(simdataCurr_p->simfile[fileNum].fcp, &buffer[filePackage_loc], simdataCurr_p->simfile[fileNum].fcpLen);
					filePackage_loc +=  simdataCurr_p->simfile[fileNum].fcpLen;
					printf("\n filePackage_loc is %02x\n", filePackage_loc);
				}
				else
				{					
					printf("\n read SIMFile from local file, FCP get fail for SIM file 0x%02x %02x ", simdataCurr_p->simfile[fileNum].fileId[0], simdataCurr_p->simfile[fileNum].fileId[1]);
				}
				

				/*DATA dataLen[2]*/
				simdataCurr_p->simfile[fileNum].dataLen = ((buffer[filePackage_loc]<<24) | 
				                                           (buffer[filePackage_loc+1]<<16) | 
				                                           (buffer[filePackage_loc+2]<<8) | 
				                                           buffer[filePackage_loc+3]);
				printf("\n buffer[%d]:%02x\n", filePackage_loc, buffer[filePackage_loc]);
				printf("\n buffer[%d]:%02x\n", filePackage_loc+1, buffer[filePackage_loc+1]);
				printf("\n buffer[%d]:%02x\n", filePackage_loc+2, buffer[filePackage_loc+2]);
				printf("\n buffer[%d]:%02x\n", filePackage_loc+3, buffer[filePackage_loc+3]);
				filePackage_loc +=4;
				printf("\nsimdataCurr_p->simfile[%d].dataLen:%02x ", fileNum, simdataCurr_p->simfile[fileNum].dataLen);					
				/*data[]*/
				if(simdataCurr_p->simfile[fileNum].dataLen != 0)
				{
					simdataCurr_p->simfile[fileNum].data = new unsigned char[simdataCurr_p->simfile[fileNum].dataLen];
					memcpy(simdataCurr_p->simfile[fileNum].data, &buffer[filePackage_loc], simdataCurr_p->simfile[fileNum].dataLen);
					filePackage_loc +=  simdataCurr_p->simfile[fileNum].dataLen;
				}
				else
			    {
				    printf("\n read SIMFile from local file, DATA get fail for SIM file 0x%02x %02x ", simdataCurr_p->simfile[fileNum].fileId[0], simdataCurr_p->simfile[fileNum].fileId[1]);
			    }
			}		
			printf("\n @file%d Ending, filePackage_loc is %02x\n", fileNum, filePackage_loc);
				

		 }
        /*check if filePackage_loc equal to buffer length, so need filePackage_loc++*/
		filePackage_loc++;
		if(filePackage_loc == bufferLen)
		{
			return TRUE;
		}
	}
}
/*2015.11.05, mod by Xili for saving file Package to local file, end*/

 

