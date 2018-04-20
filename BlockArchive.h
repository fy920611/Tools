/********************************************************************
	created:	2012/08/20	9:54
	filename: 	BlockArchive.h
	author:		Weiqy
	
	purpose:	数据封装序列化类，像MFC序列化类，将数据序列化为一个二进制buf
*********************************************************************/
#pragma once
#ifndef BlockArchive_h__
#define BlockArchive_h__

#include <string>
#include <vector>
#include <list>
#include <map>

//////////////////////////////////////////////////////////////////////////
// 本类的使用要注意事项和方法：
// 1、本类同时支持序列化输入和输出，不对输入输出做类型限制，
//    在同时做输入和输出时需要注意游标位置，否则会造成越界或返回不可预测的结果
//////////////////////////////////////////////////////////////////////////
class CBlockArchive
{
public:
    CBlockArchive(std::string& buf);
    ~CBlockArchive(void);

protected: // 屏蔽拷贝和赋值
    CBlockArchive(const CBlockArchive& arSrc);
    void operator=(const CBlockArchive& arSrc);

public:
    // 设置游标，相当于CFile中的Seek功能
    void SetCursor(size_t uCursor = 0);
    DWORD GetCursor() const;
    // 读最大长度为nMax字节的数据到buf中，返回实际读取长度
    UINT Read(std::string& buf, UINT nMax);
    // 写长度为nBytes字节的数据
    void Write(const void* lpBuf, UINT nBytes);
    // 返回buf
    const std::string& GetBuffer(void)const;

public:
    // insertion operations
    CBlockArchive& operator<<(__int8 chVal);
    CBlockArchive& operator<<(unsigned __int8 chVal);
    CBlockArchive& operator<<(__int16 iVal);
    CBlockArchive& operator<<(unsigned __int16 iVal);
    CBlockArchive& operator<<(__int32 dwVal);
    CBlockArchive& operator<<(unsigned __int32 dwVal);
    CBlockArchive& operator<<(__int64 dwdwVal);
    CBlockArchive& operator<<(unsigned __int64 dwdwVal);
    CBlockArchive& operator<<(const std::string& strVal);
    CBlockArchive& operator<<(bool bVal);
    CBlockArchive& operator<<(float fVal);
    CBlockArchive& operator<<(double dbVal);

    // extraction operations
    CBlockArchive& operator>>(__int8 &chVal);
    CBlockArchive& operator>>(unsigned __int8 &chVal);
    CBlockArchive& operator>>(__int16 &iVal);
    CBlockArchive& operator>>(unsigned __int16 &iVal);
    CBlockArchive& operator>>(__int32 &dwVal);
    CBlockArchive& operator>>(unsigned __int32 &dwVal);
    CBlockArchive& operator>>(__int64 &dwdwVal);
    CBlockArchive& operator>>(unsigned __int64 &dwdwVal);
    CBlockArchive& operator>>(std::string &strVal);
    CBlockArchive& operator>>(bool &bVal);
    CBlockArchive& operator>>(float &fVal);
    CBlockArchive& operator>>(double &dbVal);
    
private:
    std::string& m_refBuf;              /*序列化缓存*/
    std::string::size_type m_uCursor;   /*序列化游标位置*/
};

template<class T>
CBlockArchive& operator<<(CBlockArchive& ar, const std::vector<T>& aryVals)
{
    ar <<(unsigned __int32)aryVals.size();
    for(size_t i = 0; i < aryVals.size(); ++i)
    {
        ar << aryVals[i];
    }

    return ar;
}
template<class T>
CBlockArchive& operator>>(CBlockArchive& ar, std::vector<T>& aryVals)
{
    unsigned __int32 dwSize = 0;
    ar >> dwSize;
    aryVals.resize(dwSize);
    for(size_t i = 0; i < dwSize; ++i)
    {
        ar >> aryVals[i];
    }

    return ar;
}

template<class T>
CBlockArchive& operator<<(CBlockArchive& ar, const std::list<T>& lstVals)
{
    ar <<(unsigned __int32)lstVals.size();
    std::list<T>::const_iterator iter = lstVals.begin();
    for(; iter != lstVals.end(); ++iter)
    {
        ar << *iter;
    }

    return ar;
}
template<class T>
CBlockArchive& operator>>(CBlockArchive& ar, std::list<T>& lstVals)
{
    unsigned __int32 dwSize = 0;
    ar >> dwSize;
    lstVals.resize(dwSize);
    std::list<T>::iterator iter = lstVals.begin();
    for(; iter != lstVals.end(); ++iter)
    {
        ar >> *iter;
    }

    return ar;
}

template<class _Kty, class _Ty>
CBlockArchive& operator<<(CBlockArchive& ar, const std::map<_Kty, _Ty>& mapVals)
{
    ar <<(unsigned __int32)mapVals.size();
    std::map<_Kty, _Ty>::const_iterator iter = mapVals.begin();
    for(; iter != mapVals.end(); ++iter)
    {
        ar << iter->first;
        ar << iter->second;
    }

    return ar;
}

template<class _Kty, class _Ty>
CBlockArchive& operator>>(CBlockArchive& ar, std::map<_Kty, _Ty>& mapVals)
{
    unsigned __int32 dwSize = 0;
    ar >> dwSize;
    for(size_t i = 0; i < dwSize; ++i)
    {
        _Kty key;
        ar >> key;
        ar >> mapVals[key];
    }

    return ar;
}

#endif // BlockArchive_h__


