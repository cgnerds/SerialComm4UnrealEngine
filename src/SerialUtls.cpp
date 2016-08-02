// Fill out your copyright notice in the Description page of Project Settings.

#include "SerialPorts.h"
#include "SerialUtls.h"
#include <process.h>

// Sets default values
ASerialUtls::ASerialUtls()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	m_hComm = INVALID_HANDLE_VALUE;
}

ASerialUtls::~ASerialUtls()
{
	ClosePort();
}


// Called when the game starts or when spawned
void ASerialUtls::BeginPlay()
{
	Super::BeginPlay();

	InitPort(3, CBR_9600, 'N', 8U, 1U, EV_RXCHAR);
}

// Called every frame
void ASerialUtls::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	unsigned char outString[] = "F88F0280808000002000";

	WriteData(outString, 21);
}


bool ASerialUtls::InitPort(UINT portNo, UINT baud, char parity,
	UINT databits, UINT stopsbits, DWORD dwCommEvents)
{

	/** 临时变量,将制定参数转化为字符串形式,以构造DCB结构 */
	char szDCBparam[50];
	sprintf_s(szDCBparam, "baud=%d parity=%c data=%d stop=%d", baud, parity, databits, stopsbits);

	//** DEBUG
	FString tempString(szDCBparam);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 5.f, FColor::Red, tempString);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("baud=%d parity=%c data=%d stop=%d"), baud, parity, databits, stopsbits);
	}

	/** 打开指定串口,该函数内部已经有临界区保护,上面请不要加保护 */
	if (!openPort(portNo))
	{
		return false;
	}

	/** 是否有错误发生 */
	BOOL bIsSuccess = TRUE;

	/** 设置串口的超时时间,均设为0,表示不使用超时限制 */
	COMMTIMEOUTS  CommTimeouts;
	CommTimeouts.ReadIntervalTimeout = 0;
	CommTimeouts.ReadTotalTimeoutMultiplier = 0;
	CommTimeouts.ReadTotalTimeoutConstant = 0;
	CommTimeouts.WriteTotalTimeoutMultiplier = 0;
	CommTimeouts.WriteTotalTimeoutConstant = 0;
	if (bIsSuccess)
	{
		bIsSuccess = SetCommTimeouts(m_hComm, &CommTimeouts);
	}

	DCB  dcb;
	if (bIsSuccess)
	{
		// 将ANSI字符串转换为UNICODE字符串
		DWORD dwNum = MultiByteToWideChar(CP_ACP, 0, szDCBparam, -1, NULL, 0);
		wchar_t *pwText = new wchar_t[dwNum];
		if (!MultiByteToWideChar(CP_ACP, 0, szDCBparam, -1, pwText, dwNum))
		{
			bIsSuccess = TRUE;
		}

		/** 获取当前串口配置参数,并且构造串口DCB参数 */
		bIsSuccess = GetCommState(m_hComm, &dcb) && BuildCommDCB(pwText, &dcb);
		/** 开启RTS flow控制 */
		dcb.fRtsControl = RTS_CONTROL_ENABLE;

		/** 释放内存空间 */
		delete[] pwText;
	}

	if (bIsSuccess)
	{
		/** 使用DCB参数配置串口状态 */
		bIsSuccess = SetCommState(m_hComm, &dcb);
	}

	/**  清空串口缓冲区 */
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	return bIsSuccess == TRUE;
}

bool ASerialUtls::openPort(UINT portNo)
{
	/** 把串口的编号转换为设备名 */
	char szPort[50];
	sprintf_s(szPort, "COM%d", portNo);

	/** 打开指定的串口 */
	m_hComm = CreateFileA(szPort,		                /** 设备名,COM1,COM2等 */
		GENERIC_READ | GENERIC_WRITE,  /** 访问模式,可同时读写 */
		0,                             /** 共享模式,0表示不共享 */
		NULL,							/** 安全性设置,一般使用NULL */
		OPEN_EXISTING,					/** 该参数表示设备必须存在,否则创建失败 */
		0,
		0);

	/** 如果打开失败，释放资源并返回 */
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	return true;
}

void ASerialUtls::ClosePort()
{
	/** 如果有串口被打开，关闭它 */
	if (m_hComm != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hComm);
		m_hComm = INVALID_HANDLE_VALUE;
	}
}

bool ASerialUtls::WriteData(unsigned char* pData, unsigned int length)
{
	BOOL   bResult = TRUE;
	DWORD  BytesToSend = 0;

	if (m_hComm == INVALID_HANDLE_VALUE)
	{

		FString msg("failure！");

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(2, 5.f, FColor::Red, msg);
		}

		return false;
	}

	/** 向缓冲区写入指定量的数据 */
	bResult = WriteFile(m_hComm, pData, length, &BytesToSend, NULL);
	if (!bResult)
	{
		DWORD dwError = GetLastError();
		/** 清空串口缓冲区 */
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		//** LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("pData=%s length=%d"), pData, length);


	return true;
}