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

	/** ��ʱ����,���ƶ�����ת��Ϊ�ַ�����ʽ,�Թ���DCB�ṹ */
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

	/** ��ָ������,�ú����ڲ��Ѿ����ٽ�������,�����벻Ҫ�ӱ��� */
	if (!openPort(portNo))
	{
		return false;
	}

	/** �Ƿ��д����� */
	BOOL bIsSuccess = TRUE;

	/** ���ô��ڵĳ�ʱʱ��,����Ϊ0,��ʾ��ʹ�ó�ʱ���� */
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
		// ��ANSI�ַ���ת��ΪUNICODE�ַ���
		DWORD dwNum = MultiByteToWideChar(CP_ACP, 0, szDCBparam, -1, NULL, 0);
		wchar_t *pwText = new wchar_t[dwNum];
		if (!MultiByteToWideChar(CP_ACP, 0, szDCBparam, -1, pwText, dwNum))
		{
			bIsSuccess = TRUE;
		}

		/** ��ȡ��ǰ�������ò���,���ҹ��촮��DCB���� */
		bIsSuccess = GetCommState(m_hComm, &dcb) && BuildCommDCB(pwText, &dcb);
		/** ����RTS flow���� */
		dcb.fRtsControl = RTS_CONTROL_ENABLE;

		/** �ͷ��ڴ�ռ� */
		delete[] pwText;
	}

	if (bIsSuccess)
	{
		/** ʹ��DCB�������ô���״̬ */
		bIsSuccess = SetCommState(m_hComm, &dcb);
	}

	/**  ��մ��ڻ����� */
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	return bIsSuccess == TRUE;
}

bool ASerialUtls::openPort(UINT portNo)
{
	/** �Ѵ��ڵı��ת��Ϊ�豸�� */
	char szPort[50];
	sprintf_s(szPort, "COM%d", portNo);

	/** ��ָ���Ĵ��� */
	m_hComm = CreateFileA(szPort,		                /** �豸��,COM1,COM2�� */
		GENERIC_READ | GENERIC_WRITE,  /** ����ģʽ,��ͬʱ��д */
		0,                             /** ����ģʽ,0��ʾ������ */
		NULL,							/** ��ȫ������,һ��ʹ��NULL */
		OPEN_EXISTING,					/** �ò�����ʾ�豸�������,���򴴽�ʧ�� */
		0,
		0);

	/** �����ʧ�ܣ��ͷ���Դ������ */
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	return true;
}

void ASerialUtls::ClosePort()
{
	/** ����д��ڱ��򿪣��ر��� */
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

		FString msg("failure��");

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(2, 5.f, FColor::Red, msg);
		}

		return false;
	}

	/** �򻺳���д��ָ���������� */
	bResult = WriteFile(m_hComm, pData, length, &BytesToSend, NULL);
	if (!bResult)
	{
		DWORD dwError = GetLastError();
		/** ��մ��ڻ����� */
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		//** LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("pData=%s length=%d"), pData, length);


	return true;
}