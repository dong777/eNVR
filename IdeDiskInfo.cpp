#include "stdafx.h"
#include "IdeDiskInfo.h"
#include "winioctl.h"
//#include "crypto.h"

CIdeDiskInfo::CIdeDiskInfo(void)
{
}

CIdeDiskInfo::~CIdeDiskInfo(void)
{
}


//打開設備
// filename:設備的“文件名 ​​”(設備路徑)
HANDLE CIdeDiskInfo::OpenDevice(LPCTSTR filename)
{
    HANDLE hDevice;
  
    //打開設備 
    hDevice = ::CreateFile(filename,             //文件名 
        GENERIC_READ | GENERIC_WRITE,           //讀寫方式 
        FILE_SHARE_READ | FILE_SHARE_WRITE,     //共享方式 
        NULL,                     //默認的安全描述符 
        OPEN_EXISTING,            //創建方式
        0 ,                        //不需設置文件屬性 
        NULL);                    //不需參照模板文件
    DWORD nRet = ::GetLastError();
    return hDevice;
}
  
//向驅動發“IDENTIFY DEVICE”命令，獲得設備信息
// hDevice:設備句柄
// pIdInfo:設備信息結構指針
BOOL CIdeDiskInfo::IdentifyDevice(HANDLE hDevice, PIDINFO pIdInfo)
{
    PSENDCMDINPARAMS pSCIP;       //輸入數據結構指針 
    PSENDCMDOUTPARAMS pSCOP;      //輸出數據結構指針 
    DWORD dwOutBytes;             // IOCTL輸出數據長度 
    BOOL bResult;                 // IOCTL返回值
  
    //申請輸入/輸出數據結構空間 
    pSCIP = (PSENDCMDINPARAMS)::GlobalAlloc(LMEM_ZEROINIT, sizeof (SENDCMDINPARAMS) - 1 );
    pSCOP = (PSENDCMDOUTPARAMS)::GlobalAlloc(LMEM_ZEROINIT, sizeof (SENDCMDOUTPARAMS) + sizeof (IDINFO) - 1 );
  
    //指定ATA/ATAPI命令的寄存器值
// pSCIP->irDriveRegs.bFeaturesReg = 0; 
// pSCIP->irDriveRegs.bSectorCountReg = 0; 
// pSCIP->irDriveRegs.bSectorNumberReg = 0; 
// pSCIP->irDriveRegs. bCylLowReg = 0; 
// pSCIP->irDriveRegs.bCylHighReg = 0; 
// pSCIP->irDriveRegs.bDriveHeadReg = 0;
    pSCIP->irDriveRegs.bCommandReg = IDE_ATA_IDENTIFY;
  
    //指定輸入/輸出數據緩衝區大小 
    pSCIP->cBufferSize = 0 ;
    pSCOP->cBufferSize = sizeof (IDINFO);
  
    // IDENTIFY DEVICE 
    bResult = ::DeviceIoControl(hDevice,         //設備句柄 
        DFP_RECEIVE_DRIVE_DATA,                  //指定IOCTL 
        pSCIP, sizeof (SENDCMDINPARAMS) - 1 ,      //輸入數據緩衝區 
        pSCOP, sizeof (SENDCMDOUTPARAMS) + sizeof (IDINFO) - 1 ,     //輸出數據緩衝區 
        &dwOutBytes,                 //輸出數據長度 
        (LPOVERLAPPED)NULL);         //用同步I/O
  
    //複製設備參數結構 
    ::memcpy(pIdInfo, pSCOP->bBuffer, sizeof (IDINFO));
  
    // 釋放輸入/輸出數據空間
    ::GlobalFree(pSCOP);
    ::GlobalFree(pSCIP);
  
    return bResult;
}
  
//向SCSI MINI-PORT驅動發“IDENTIFY DEVICE”命令，獲得設備信息
// hDevice:設備句柄
// pIdInfo:設備信息結構指針 
BOOL CIdeDiskInfo::IdentifyDeviceAsScsi(HANDLE hDevice, int nDrive, PIDINFO pIdInfo)
{
    PSENDCMDINPARAMS pSCIP;      //輸入數據結構指針 
    PSENDCMDOUTPARAMS pSCOP;     //輸出數據結構指針 
    PSRB_IO_CONTROL pSRBIO;      // SCSI輸入輸出數據結構指針 
    DWORD dwOutBytes;            // IOCTL輸出數據長度 
    BOOL bResult;                // IOCTL返回值
  
    // 申請輸入/輸出數據結構空間
    pSRBIO = (PSRB_IO_CONTROL)::GlobalAlloc(LMEM_ZEROINIT,
        sizeof (SRB_IO_CONTROL) + sizeof (SENDCMDOUTPARAMS) + sizeof (IDINFO) - 1 );
    pSCIP = (PSENDCMDINPARAMS)(( char *)pSRBIO + sizeof (SRB_IO_CONTROL));
    pSCOP = (PSENDCMDOUTPARAMS)(( char *)pSRBIO + sizeof (SRB_IO_CONTROL));
  
    //填充輸入/輸出數據 
    pSRBIO->HeaderLength = sizeof (SRB_IO_CONTROL);
    pSRBIO->Timeout = 10000 ;
    pSRBIO->Length = sizeof (SENDCMDOUTPARAMS) + sizeof (IDINFO) - 1 ;
    pSRBIO->ControlCode = IOCTL_SCSI_MINIPORT_IDENTIFY;
    ::strncpy (( char *)pSRBIO->Signature, "SCSIDISK" , 8 );
  
    //指定ATA/ATAPI命令的寄存器值
// pSCIP->irDriveRegs.bFeaturesReg = 0; 
// pSCIP->irDriveRegs.bSectorCountReg = 0; 
// pSCIP->irDriveRegs.bSectorNumberReg = 0; 
// pSCIP->irDriveRegs. bCylLowReg = 0; 
// pSCIP->irDriveRegs.bCylHighReg = 0; 
// pSCIP->irDriveRegs.bDriveHeadReg = 0;
    pSCIP->irDriveRegs.bCommandReg = IDE_ATA_IDENTIFY;
    pSCIP->bDriveNumber = nDrive;
  
    // IDENTIFY DEVICE 
    bResult = ::DeviceIoControl(hDevice,     //設備句柄 
        IOCTL_SCSI_MINIPORT,                 //指定IOCTL 
        pSRBIO, sizeof (SRB_IO_CONTROL) + sizeof (SENDCMDINPARAMS) - 1 ,     //輸入數據緩衝區 
        pSRBIO, sizeof (SRB_IO_CONTROL) + sizeof (SENDCMDOUTPARAMS) + sizeof (IDINFO) - 1 ,     //輸出數據緩衝區 
        &dwOutBytes,             //輸出數據長度 
        (LPOVERLAPPED)NULL);     //用同步I/O
  
    //複製設備參數結構 
    ::memcpy(pIdInfo, pSCOP->bBuffer, sizeof (IDINFO));
  
    // 釋放輸入/輸出數據空間
    ::GlobalFree(pSRBIO);
  
    return bResult;
}
  
//將串中的字符兩兩顛倒
//原因是ATA/ATAPI中的WORD，與Windows採用的字節順序相反
//驅動程序中已經將收到的數據全部反過來，我們來個負負得正
void CIdeDiskInfo::AdjustString( char * str, int len)
{
    char ch;
     int i;
  
    //兩兩顛倒
    for (i = 0 ; i < len; i += 2 )
    {
        ch = str[i];
        str[i] = str[i + 1 ];
        str[i + 1 ] = ch;
    }
  
    //若是右對齊的，調整為左對齊(去掉左邊的空格) 
    i = 0 ;
     while ((i < len) && (str[i] == ' ' )) i++;
  
    ::memmove(str, &str[i], len - i);
  
    //去掉右邊的空格 
    i = len - 1 ;
     while ((i >= 0 ) && (str[i] == ' ' ))
    {
        str[i] = '\0' ;
        i--;
    }
}
  
//讀取IDE硬盤的設備信息，必須有足夠權限
// nDrive:驅動器號(0=第一個硬盤，1=0=第二個硬盤，......) 
// pIdInfo:設備信息結構指針 
BOOL CIdeDiskInfo::GetPhysicalDriveInfoInNT( int nDrive, PIDINFO pIdInfo)
{
    HANDLE hDevice;          //設備句柄 
    BOOL bResult;            //返回結果
    TCHAR szFileName[64] ={0};     //文件名
  
    ::_stprintf(szFileName, _T("\\\\.\\PhysicalDrive%d") , nDrive);
    //::sprintf(szFileName, _T("\\\\.\\PhysicalDrive%d") , nDrive);
  
    hDevice = OpenDevice(szFileName);
  
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }
  
    // IDENTIFY DEVICE
    bResult = IdentifyDevice(hDevice, pIdInfo);
  
    if (bResult)
    {
        //調整字符串 
        AdjustString(pIdInfo->sSerialNumber, 20 );
        AdjustString(pIdInfo->sModelNumber, 40 );
        AdjustString(pIdInfo->sFirmwareRev, 8 );
    }
  
    ::CloseHandle (hDevice);
  
    return bResult;
}
  
//用SCSI驅動讀取IDE硬盤的設備信息，不受權限制約
// nDrive:驅動器號(0=Primary Master, 1=Promary Slave, 2=Secondary master, 3=Secondary slave) 
// pIdInfo:設備信息結構指針 
BOOL CIdeDiskInfo::GetIdeDriveAsScsiInfoInNT( int nDrive, PIDINFO pIdInfo)
{
    HANDLE hDevice;          //設備句柄 
    BOOL bResult;            //返回結果
    TCHAR szFileName[64]={0};     //文件名
  
    ::_stprintf(szFileName, _T("\\\\.\\Scsi%d:") , nDrive/ 2 );
    //::sprintf(szFileName, _T("\\\\.\\Scsi%d:") , nDrive/ 2 );
  
    hDevice = OpenDevice(szFileName);
  
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }
  
    // IDENTIFY DEVICE 
    bResult = IdentifyDeviceAsScsi(hDevice, nDrive% 2 , pIdInfo);
  
    //檢查是不是空串
    if (pIdInfo->sModelNumber[ 0 ] == '\0' )
    {
        bResult = FALSE;
    }
  
    if (bResult)
    {
        //調整字符串 
        AdjustString(pIdInfo->sSerialNumber, 20 );
        AdjustString(pIdInfo->sModelNumber, 40 );
        AdjustString(pIdInfo->sFirmwareRev, 8 );
    }
  
    return bResult;
}

bool CIdeDiskInfo::bAuthorization(void)
{
    for(int i=0; i < 4; i++)   
    {   
        CString strInfo;
        IDINFO IdInfo;  
        if(GetPhysicalDriveInfoInNT(i, &IdInfo))   
        {   
            char szSn[21];
            strncpy(szSn, IdInfo.sSerialNumber, sizeof(IdInfo.sSerialNumber));
        }   
    }       
    return false;
}
void CIdeDiskInfo::SaveValue(char* szValue, DWORD nLen)
{
    CRegKey regKey;
    TCHAR tzForder[] = _T("SOFTWARE\\Etrovision Technology\\");
    LONG nRet = regKey.Open(HKEY_LOCAL_MACHINE, tzForder);
    if(nRet != ERROR_SUCCESS)
    {
        if( regKey.Create(HKEY_LOCAL_MACHINE, tzForder) != 0)
        {
            TRACE(_T("Create Registry Fail."));
            //return 0; 
        }
    }
    nRet = regKey.SetValue(_T("Prometheus"), REG_BINARY, szValue, nLen);
    regKey.Close();
}
bool CIdeDiskInfo::bReadValue(char* szValue, DWORD* pnLen)
{
    CRegKey regKey;
    TCHAR tzForder[] = _T("SOFTWARE\\Etrovision Technology\\");
    LONG nRet = regKey.Open(HKEY_LOCAL_MACHINE, tzForder);
    if(nRet != ERROR_SUCCESS)
    {
        return false;
    }
    nRet = regKey.QueryBinaryValue(_T("Prometheus"), szValue, pnLen);
    regKey.Close();
}

bool CIdeDiskInfo::bEnroll(DWORD nChannel)
{
    bool bRet = false;
    char szKey[30]={NULL};
  
    CString strInfo;
    IDINFO IdInfo;  
    bool bCheckVenderCode(false);
    int i=0;
    while (GetPhysicalDriveInfoInNT(i++, &IdInfo))   
    {   
        memcpy(szKey, &IdInfo.wReserved89[40], 8);
        if (strcmp(szKey, "80674436") !=0)
        {
            continue;
        }
        szKey[8] = nChannel;
        memcpy(&szKey[9], IdInfo.sSerialNumber, 20);
        TRACE(szKey); 
        bCheckVenderCode = true;
        break;
    }   

    if (bCheckVenderCode == false)
    {
        //AfxMessageBox(_T("SataDom檢查失敗,認證失敗!!!"));
        return bRet;
    }    

    TCHAR szPassword[]=_T("s~!a@f#d$s%a^f&e*r(t)uAGyDtE9K087654K3L21q?></.,+_))(*&^%$#!@#$%wesrx");  
    CCrypto m_crypto;
    m_crypto.DeriveKey(szPassword);
    CByteArray arData;
    CByteArray arDataSrc;
    arDataSrc.SetSize(sizeof(szKey)-1); 
    for (int ci=0; ci < sizeof(szKey)-1; ci++)
    {
        arDataSrc[ci]=szKey[ci];
    }
    //arDataSrc.Copy(*(CByteArray*)szKey);
    //arData.Copy(*(CByteArray*)szKey);

	//	Write the data to the array.
	if(m_crypto.Encrypt(arDataSrc, arData) == false)
    {
    }
    SaveValue((char*)(arData.GetData()), arData.GetSize());
    CString cs;
    CByteArray arCheck;
    arCheck.SetSize(arData.GetSize());
    
    if(m_crypto.Decrypt(arData, arCheck) == true)
    {
        if (strncmp((char*)arCheck.GetData(), (char*)arDataSrc.GetData(), arDataSrc.GetSize()) == 0)
        {
           bRet = true;
        }
    }
    return bRet;

}
bool CIdeDiskInfo::bAuthorized(BYTE* pnChannel)
{
//#ifdef _DEBUG
//#define INVINCIBLE
//#endif
#ifdef INVINCIBLE
    *pnChannel = 0xFF;
    return true;
#endif
    bool bRet = false;
    char szKey[30]={NULL};
    bool bCheckVenderCode(false);
    int i=0;
    IDINFO IdInfo;
    while (GetPhysicalDriveInfoInNT(i++, &IdInfo))   
    {   
        memcpy(szKey, &IdInfo.wReserved89[40], 8);
        if (strcmp(szKey, "80674436") !=0)
        {
            continue;
        }
        szKey[8] = *pnChannel;
        memcpy(&szKey[9], IdInfo.sSerialNumber, 20);
        TRACE(szKey); 
        bCheckVenderCode = true;
        break;
    }   

    if (bCheckVenderCode == false)
    {
        //AfxMessageBox(_T("SataDom檢查失敗,認證失敗!!!"));
        *pnChannel = 0;
        return bRet;
    }       

    TCHAR szPassword[]=_T("s~!a@f#d$s%a^f&e*r(t)uAGyDtE9K087654K3L21q?></.,+_))(*&^%$#!@#$%wesrx");
    CCrypto m_crypto;
    m_crypto.DeriveKey(szPassword);
 
    const DWORD BUFFSIZE(1024);
    DWORD nSize(BUFFSIZE);
    //char szStoreKeyValue[BUFFSIZE]={0};
    CByteArray arData;
    arData.SetSize(nSize);
    bReadValue((char*)arData.GetData(), &nSize);
    arData.SetSize(nSize);


    CString cs;
    CByteArray arResult;
    arResult.SetSize(nSize);
// data shell be "80674436?20_bytes_serial_no" ? = binary channel size
    if(m_crypto.Decrypt(arData, arResult) == true)
    {
        bool bCheck1 = (0 == memcmp(arResult.GetData(), szKey, 8));
        bool bCheck2 = (0 == memcmp(&arResult.GetData()[9], &szKey[9], 20));
        if (bCheck1 && bCheck2)
        {
            *pnChannel =  arResult.GetData()[8];
            bRet = true;
        }
    }
    return bRet;
}