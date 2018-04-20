#include "StdAfx.h"
#include "BlockArchive.h"
#include <exception>
#include <WINSOCK2.H>

CBlockArchive::CBlockArchive(std::string& buf)
    : m_refBuf(buf)
    , m_uCursor(0)
{
}

CBlockArchive::~CBlockArchive(void)
{
}

// operations
void CBlockArchive::SetCursor(size_t uCursor /* = 0 */)
{
    m_uCursor = min(uCursor, m_refBuf.length());
}

DWORD CBlockArchive::GetCursor() const
{
    return m_uCursor;
}

UINT CBlockArchive::Read(std::string& buf, UINT nMax)
{
    UINT uRealCount = min(nMax, m_refBuf.length() - m_uCursor);
    buf.assign(m_refBuf, m_uCursor, uRealCount);
    m_uCursor += uRealCount;

    return uRealCount;
}

void CBlockArchive::Write(const void* lpBuf, UINT nMaxBytes)
{
    m_refBuf.replace(m_uCursor, nMaxBytes, (const char*)lpBuf, nMaxBytes);
    m_uCursor += nMaxBytes;
}

// Insertion operations
CBlockArchive& CBlockArchive::operator<<(__int8 chVal)
{
    const size_t nBytes = sizeof(chVal);
    ASSERT(nBytes == 1);

    if(m_uCursor < m_refBuf.length())
    {
        m_refBuf[m_uCursor] = (char)chVal;
    }
    else
    {
        m_refBuf += (char)chVal;
    }

    m_uCursor += nBytes;

    return *this;
}

CBlockArchive& CBlockArchive::operator<<(unsigned __int8 chVal)
{
    return (*this)<<__int8(chVal);
}

CBlockArchive& CBlockArchive::operator<<(__int16 iVal)
{
    const size_t nBytes = sizeof(iVal);
    ASSERT(nBytes == 2);

    u_short nTmp = htons(iVal);
    m_refBuf.replace(m_uCursor, nBytes, (char *)&nTmp, nBytes);
    m_uCursor += nBytes;

    return *this;
}

CBlockArchive& CBlockArchive::operator<<(unsigned __int16 iVal)
{
    return (*this)<<__int16(iVal);
}

CBlockArchive& CBlockArchive::operator<<(__int32 dwVal)
{
    const size_t nBytes = sizeof(dwVal);
    ASSERT(nBytes == 4);

    u_long nTmp = htonl(dwVal);
    m_refBuf.replace(m_uCursor, nBytes, (char *)&nTmp, nBytes);
    m_uCursor += nBytes;

    return *this;
}

CBlockArchive& CBlockArchive::operator<<(unsigned __int32 dwVal)
{
    return (*this)<<__int32(dwVal);
}

CBlockArchive& CBlockArchive::operator<<(__int64 dwdwVal)
{
    // 高位
    __int32 dwTmp = (__int32)(dwdwVal >> 32 & 0xFFFFFFFF);
    (*this) << dwTmp;

    // 低位
    dwTmp = (__int32)(dwdwVal & 0xFFFFFFFF);
    (*this) << dwTmp;

    return *this;
}

CBlockArchive& CBlockArchive::operator<<(unsigned __int64 dwdwVal)
{
    return (*this)<<__int64(dwdwVal);
}

CBlockArchive& CBlockArchive::operator<<(const std::string& strVal)
{
    (*this)<<(unsigned __int32)strVal.length();

    m_refBuf.replace(m_uCursor, strVal.length(), strVal);
    m_uCursor += strVal.length();

    return *this;
}

CBlockArchive& CBlockArchive::operator<<(bool bVal)
{
    return (*this) << (__int8)bVal;
}

CBlockArchive& CBlockArchive::operator<<( float fVal )
{
    const size_t nBytes = sizeof(fVal);
    ASSERT(nBytes == 4);

    // 将其内存数据视为32位整型来序列化，只序列化内存数据，不关心数据类型
    __int32 i32Temp = *(__int32*)&fVal;
    (*this) << i32Temp;

    return *this;
}

CBlockArchive& CBlockArchive::operator<<( double dbVal )
{
    const size_t nBytes = sizeof(double);
    ASSERT(nBytes == 8);

    // 将其内存数据视为64位整型来序列化
    __int64 i64Temp = *(__int64*)&dbVal;
    (*this) << i64Temp;

    return *this;
}

// extraction operations
CBlockArchive& CBlockArchive::operator>>(__int8 &chVal)
{
    if (m_uCursor + sizeof(chVal) <= m_refBuf.length())
    {
        chVal = m_refBuf[m_uCursor++];
    }
    else
    {
        throw std::out_of_range("invalid buffer position");
    }

    return *this;
}
CBlockArchive& CBlockArchive::operator>>(unsigned __int8 &chVal)
{
    __int8 chTemp = 0;
    (*this)>>chTemp;
    chVal = chTemp;

    return *this;
}
CBlockArchive& CBlockArchive::operator>>(__int16 &iVal)
{
    const size_t nBytes = sizeof(iVal);
    ASSERT((nBytes == 2));

    if (m_uCursor + nBytes <= m_refBuf.length())
    {
        __int16 nTmp;
#if _MSC_VER <= 1200 //VC6
		m_refBuf.copy((char *)&nTmp, nBytes, m_uCursor);
#else
        m_refBuf._Copy_s((char *)&nTmp, nBytes, nBytes, m_uCursor);
#endif
        m_uCursor += nBytes;
        iVal = ntohs(nTmp);
    }
    else
    {
        throw std::out_of_range("invalid buffer position");
    }
    return *this;
}
CBlockArchive& CBlockArchive::operator>>(unsigned __int16 &iVal)
{
    __int16 iTmp = 0;
    (*this)>>iTmp;
    iVal = iTmp;

    return *this;
}
CBlockArchive& CBlockArchive::operator>>(__int32 &dwVal)
{
    const size_t nBytes = sizeof(dwVal);
    ASSERT(nBytes == 4);

    if (m_uCursor + nBytes <= m_refBuf.length())
    {
        __int32 dwTmp;
#if _MSC_VER <= 1200 // VC6
		m_refBuf.copy((char *)&dwTmp, nBytes, m_uCursor);
#else
        m_refBuf._Copy_s((char *)&dwTmp, nBytes, nBytes, m_uCursor);
#endif
        m_uCursor += nBytes;
        dwVal = ntohl(dwTmp);
    }
    else
    {
        throw std::out_of_range("invalid buffer position");
    }

    return *this;
}
CBlockArchive& CBlockArchive::operator>>(unsigned __int32 &dwVal)
{
    __int32 dwTmp = 0;
    (*this)>>dwTmp;
    dwVal = dwTmp;

    return *this;
}
CBlockArchive& CBlockArchive::operator>>(__int64 &dwdwVal)
{
	__int32 dwHigh = 0;
	(*this) >> dwHigh;
	__int32 dwLow = 0;
	(*this) >> dwLow;


    // __int64(dwlow)的高4字节可能是0xffffffff
	dwdwVal = (__int64(dwHigh) << 32) | (__int64(dwLow) & 0xFFFFFFFF);

    return *this;
}
CBlockArchive& CBlockArchive::operator>>(unsigned __int64 &dwdwVal)
{
    __int64 dwdwTemp = 0;
    (*this)>>dwdwTemp;
    dwdwVal = dwdwTemp;

    return *this;
}
CBlockArchive& CBlockArchive::operator>>(std::string &strVal)
{
    unsigned __int32 dwLen = 0;
    (*this) >> dwLen;

    if (m_uCursor + dwLen <= m_refBuf.length())
    {
        strVal.assign(m_refBuf, m_uCursor, dwLen);
        m_uCursor += dwLen;
    }
    else
    {
        throw std::out_of_range("invalid buffer position");
    }

    return *this;
}
CBlockArchive& CBlockArchive::operator>>(bool &bVal)
{
    __int8 chTemp;
    (*this)>>chTemp;
    bVal = chTemp!=0;

    return *this;
}

CBlockArchive& CBlockArchive::operator>>( float &fVal )
{
    __int32 i32Temp = 0;
    (*this) >> i32Temp;
    fVal = *(float*)&i32Temp;

    return *this;
}

CBlockArchive& CBlockArchive::operator>>( double &dbVal )
{
    __int64 i64Temp = 0;
    (*this) >> i64Temp;
    dbVal = *(double*)&i64Temp;

    return *this;
}

const std::string& CBlockArchive::GetBuffer( void ) const
{
    return m_refBuf;
}
