/*!
 * \file IXInterfaces.h
 * \date 2017/10/19 18:56
 *
 * \author Moons
 *
 * \brief  提供C++接口的基础接口类及相关实现
 *
 *         包含：
 *         侵入式智能指针实现
 *         侵入式引用计数的接口超类及其实现
 *         各类枚举器接口及其实现
 *         回调接口及回调管理器的实现
 *         常用智能锁封装
 *         内存泄露监测
 *
 * \note   修改记录：
 *         v2.0:
 *         1、完善智能指针定义，增强类型安全型，提供移动语义的构造和赋值
 *         2、增加智能临界区锁、互斥锁
 *         3、增加几个常用功能宏
 *         4、增加简易的内存泄露监测功能
 *         5、增加使用示例
*/
#pragma once
#include <vector>
#include <map>
#include <set>
#include <string>
#include <assert.h>
#include <tchar.h>
#include <memory>
#include <utility>
#include <list>
#include <algorithm>
#include "LockHelper.h"
#include <io.h>

#ifndef verify
#if defined(_DEBUG) || defined(DEBUG)
#define verify(x) assert(x)
#else
#define verify(x) ((void)(x))
#endif
#endif

#define IXInterface_Version  200

#define MemLeakCheck

/// 用于在成员类内取父对象的地址
#define __VX_OFFSET_OF__(type,x) (long)(&(((type*)0)->x))
#define VX_PARENT_CLASS(type,x) reinterpret_cast<type*>(((char*)this) - __VX_OFFSET_OF__(type,x))
#ifndef GET_PARENT_CLASS
#define GET_PARENT_CLASS(type, x) VX_PARENT_CLASS(type,x)
#endif

/// 用于字符串拼接
#define __VX_MAKE_STR__(x) #x
#define __VX_MAKE_CHAR__(x) #@x
#define __VX_TOKEN_PASTER__(x,y) x##y
#define VX_CAT_TOKEN(x,y) __VX_TOKEN_PASTER__(x,y)
#define VX_TOSTRING(x) __VX_MAKE_STR__(x)
#define VX_TOWSTRING(x) VX_CAT_TOKEN(L, VX_TOSTRING(x))

/// TODO宏
#define VX_STATIC_WARNING(x) __pragma(message(""__FILE__ "(" VX_TOSTRING(__LINE__) "): warning: " VX_TOSTRING(x)))
#define VX_TODO(x) VX_STATIC_WARNING(VX_CAT_TOKEN(TODO\x20:\x20, x))
#ifndef TODO
#define TODO(x) VX_TODO(x) 
#endif

#pragma region 智能指针定义
template<typename T>
class VxSmartInterfacePtr
{
private:
    typedef VxSmartInterfacePtr this_type;

public:
    VxSmartInterfacePtr()
        : px(NULL)
    {

    }
    VxSmartInterfacePtr(std::nullptr_t)
        : px(nullptr)
    {

    }
    VxSmartInterfacePtr(T* p, bool addref = true)
        : px(p)
    {
        if(px && addref)  
        {
            px->AddRef();
        }
    }
    template<class U> VxSmartInterfacePtr(U* p, bool addref = true)
    {
        try
        {
            px = dynamic_cast<T*>(p);
        }
        catch (const std::bad_cast&)
        {

        }
        if (px && addref)
        {
            px->AddRef();
        }
    }

    template<class U> VxSmartInterfacePtr(VxSmartInterfacePtr<U> const & rhs)
    {
        try
        {
            px = dynamic_cast<T*>(rhs.get());
        }
        catch (const std::bad_cast&)
        {

        }
        if (px != 0)
        {
            px->AddRef();
        }
    }
    VxSmartInterfacePtr(VxSmartInterfacePtr const & rhs)
        : px( rhs.px )
    {
        if(px != 0) 
        {
            px->AddRef();
        }
    }

#if _MSC_VER >= 1600
    VxSmartInterfacePtr(VxSmartInterfacePtr && rhs)
        : px(rhs.px)
    {
        rhs.px = nullptr;
    }
#endif

    ~VxSmartInterfacePtr()
    {
        if(px)
        {
            px->Release();
        }
    }

    VxSmartInterfacePtr & operator=(std::nullptr_t)
    {
        reset();
        return *this;
    }

    template<class U> VxSmartInterfacePtr & operator=(VxSmartInterfacePtr<U> const & rhs)
    {
        VxSmartInterfacePtr(rhs).swap(*this);
        return *this;
    }

    VxSmartInterfacePtr & operator=(VxSmartInterfacePtr const & rhs)
    {
        VxSmartInterfacePtr(rhs).swap(*this);
        return *this;
    }

#if _MSC_VER >= 1600
    VxSmartInterfacePtr & operator=(VxSmartInterfacePtr && rhs)
    {
        if (&rhs != this)
        {
            reset();

            rhs.swap(*this);
        }

        return *this;
    }
#endif

    void reset()
    {
        VxSmartInterfacePtr<T>().swap( *this );
    }

    void reset( T * rhs, bool addref = true )
    {
        VxSmartInterfacePtr( rhs, addref ).swap( *this );
    }

    T * get() const
    {
        return px;
    }

    T * detach()
    {
        T * tmp = px;
        px = nullptr;
        return tmp;
    }

    T ** address()
    {
        reset();
        return &px;
    }

    T & operator*() const
    {
        assert( px != 0 );
        return *px;
    }

    T * operator->() const
    {
        assert( px != 0 );
        return px;
    }

    void swap(VxSmartInterfacePtr & rhs)
    {
        T * tmp = px;
        px = rhs.px;
        rhs.px = tmp;
    }

    bool operator! () const // never throws
    {
        return px == 0;
    }

    typedef T * this_type::*unspecified_bool_type;
    operator unspecified_bool_type() const // never throws
    {
        return px == 0 ? 0 : &this_type::px;
    }

private:
    T*      px;
};

template<class T> inline bool operator==(VxSmartInterfacePtr<T> const & a, VxSmartInterfacePtr<T> const & b)
{
    return a.get() == b.get();
}

template<class T> inline bool operator!=(VxSmartInterfacePtr<T> const & a, VxSmartInterfacePtr<T> const & b)
{
    return !(a == b);
}

template<class T> inline bool operator==(VxSmartInterfacePtr<T> const & a, T * b)
{
    return a.get() == b;
}

template<class T> inline bool operator!=(VxSmartInterfacePtr<T> const & a, T * b)
{
    return !(a == b);
}

template<class T> inline bool operator==(T * a, VxSmartInterfacePtr<T> const & b)
{
    return a == b.get();
}

template<class T> inline bool operator!=(T * a, VxSmartInterfacePtr<T> const & b)
{
    return !(a == b);
}

template<class T, class U> inline bool operator==(VxSmartInterfacePtr<T> const & a, VxSmartInterfacePtr<U> const & b)
{
    return a == decltype(a)(b);
}

template<class T, class U> inline bool operator!=(VxSmartInterfacePtr<T> const & a, VxSmartInterfacePtr<U> const & b)
{
    return !(a == b);
}

template<class T, class U> inline bool operator==(VxSmartInterfacePtr<T> const & a, U * b)
{
    return VxSmartInterfacePtr<U>(a) == b;
}

template<class T, class U> inline bool operator!=(VxSmartInterfacePtr<T> const & a, U * b)
{
    return !(a == b);
}

template<class T, class U> inline bool operator==(T * a, VxSmartInterfacePtr<U> const & b)
{
    return a == VxSmartInterfacePtr<T>(b);
}

template<class T, class U> inline bool operator!=(T * a, VxSmartInterfacePtr<U> const & b)
{
    return !(a == b);
}

template<class T> inline bool operator<(VxSmartInterfacePtr<T> const & a, VxSmartInterfacePtr<T> const & b)
{
    return std::less<T *>()(a.get(), b.get());
}

template<class T> void swap(VxSmartInterfacePtr<T> & lhs, VxSmartInterfacePtr<T> & rhs)
{
    lhs.swap(rhs);
}

#ifdef USE_BOOST
#include <boost/smart_ptr/intrusive_ptr.hpp>
// boost库智能指针支持
namespace boost
{
    template<class T>
    inline void intrusive_ptr_add_ref(T* p)
    {
        p->AddRef();
    }

    template<class T>
    inline void intrusive_ptr_release(T* p)
    {
        p->Release();
    }
}

#define X_SMARTPTR_TYPEDEF(Interface)     typedef boost::intrusive_ptr<Interface> Interface ## Ptr
#else
#define X_SMARTPTR_TYPEDEF(Interface)     typedef VxSmartInterfacePtr<Interface> Interface ## Ptr
#endif

#pragma endregion

#pragma region 智能锁定义
struct IXSmartLock
{
    virtual void Lock()     = 0;
    virtual void Unlock()   = 0;
};

class VxCriticalSectionLock : public IXSmartLock
{
public:
    VxCriticalSectionLock()
    {
        InitializeCriticalSection(&m_CS);
    }

    ~VxCriticalSectionLock()
    {
        DeleteCriticalSection(&m_CS);
    }

private: // 拒绝拷贝
    VxCriticalSectionLock(const VxCriticalSectionLock&);
    VxCriticalSectionLock& operator=(const VxCriticalSectionLock&);

public:
    void Lock()
    {
        EnterCriticalSection(&m_CS);
    }

    void Unlock()
    {
        LeaveCriticalSection(&m_CS);
    }

private:
    CRITICAL_SECTION	m_CS;
};

class VxMutexLock : public IXSmartLock
{
public:
    VxMutexLock(const char * name)
    {
        m_hMutex = CreateMutexA(NULL, FALSE, name);
        DWORD dwErrorId = GetLastError();
        if (ERROR_ALREADY_EXISTS == dwErrorId)
        {
            m_bAlreadyExist = true;
        }
        else if (ERROR_ACCESS_DENIED == dwErrorId)
        {
            m_bAlreadyExist = true;
            assert(NULL == m_hMutex);
            m_hMutex = OpenMutexA(SYNCHRONIZE, FALSE, name);
        }
    }
    VxMutexLock(const wchar_t * name)
    {
        m_hMutex = CreateMutexW(NULL, FALSE, name);
        DWORD dwErrorId = GetLastError();
        if (ERROR_ALREADY_EXISTS == dwErrorId)
        {
            m_bAlreadyExist = true;
        }
        else if (ERROR_ACCESS_DENIED == dwErrorId)
        {
            m_bAlreadyExist = true;
            assert(NULL == m_hMutex);
            m_hMutex = OpenMutexW(SYNCHRONIZE, FALSE, name);
        }
    }
    ~VxMutexLock()
    {
        if(m_hMutex)
        {
            CloseHandle(m_hMutex);
            m_hMutex = NULL;
        }
    }

private: // 拒绝拷贝
    VxMutexLock(const VxMutexLock&);
    VxMutexLock& operator=(const VxMutexLock&);

public:
    void Lock()
    {
        assert(m_hMutex != NULL);
        if(m_hMutex)
        {
            WaitForSingleObject(m_hMutex,INFINITE);
        }
    }

    void Unlock()
    {
        assert(m_hMutex != NULL);
        if(m_hMutex)
        {
            ReleaseMutex(m_hMutex);
        }
    }

    BOOL IsValid() const
    {
        return m_hMutex != NULL;
    }

    bool IsAlreadyExist() const
    {
        return m_bAlreadyExist;
    }

public:
    HANDLE	m_hMutex;
    bool    m_bAlreadyExist;
};

class VxLockHelper
{
public:
    VxLockHelper(IXSmartLock* pLock):m_pLock(pLock)
    {
        if(m_pLock)
        {
            m_pLock->Lock();
        }
    }

    ~VxLockHelper()
    {
        if(m_pLock)
        {
            m_pLock->Unlock();
        }
    }

private: // 拒绝拷贝
    VxLockHelper(const VxLockHelper&);
    VxLockHelper& operator=(const VxLockHelper&);

private:
    IXSmartLock* m_pLock;
};
#define VX_SCOPED_SAFELOCK(cs) VxLockHelper VX_CAT_TOKEN(lock_, __LINE__)(cs)
#pragma endregion

#define _IID_IXInterface  "{F9F27C3C-5A5C-4C39-B8B4-D3FEC6A609C9}"
struct __declspec(uuid(_IID_IXInterface)) __declspec(novtable) IXInterface
{
    virtual long AddRef(void) = 0;
    virtual long Release(void) = 0; 
};
X_SMARTPTR_TYPEDEF(IXInterface);

#ifdef MemLeakCheck
struct MemLeakHelper
{
    struct MemLeakInfo
    {
        VxCriticalSectionLock       lock;
		std::map<LPVOID, ULONG>	addresses;
		std::map<ULONG, std::vector<LPVOID>> callstack_alloc;
        ~MemLeakInfo()
        {
            if(!addresses.empty())
            {
                OutputDebugStringA("Checked memory leak!\n");
                std::vector<LPVOID> vtb_addrs(addresses.size());
				std::vector<std::vector<LPVOID>> callstack_leak;
                size_t i = 0;
                for(auto iter = addresses.begin(); iter != addresses.end(); ++iter)
                {
                    auto addr = *iter;
                    vtb_addrs[i++] = *(void **)addr.first;
					if(callstack_alloc.find(addr.second) != callstack_alloc.end())
					{
						callstack_leak.push_back(callstack_alloc[addr.second]);
						callstack_alloc.erase(addr.second);
					}
					
                    char buf[128] = {0};
                    sprintf_s(buf, "memory leak address: obj_addr = 0x%p, vtable_addr = 0x%p\n", addr.first, *(void **)addr.first);
                    OutputDebugStringA(buf);
                }
                OutputDebugStringA("memory leak output finished!\n");
				if(_access("debug_mode", 0) != -1)
				{
					assert(0 && _T("检测到内存泄漏，请截取此时的dump"));
				}
            }
        }
    };
    static MemLeakInfo			leak_info;
};
__declspec(selectany) MemLeakHelper::MemLeakInfo MemLeakHelper::leak_info;
#endif

template <typename IDerive>
class __declspec(novtable) IXInterfaceImpl : public IDerive
{
protected:
    IXInterfaceImpl()
        : m_lRefCount(0)
    {
#ifdef MemLeakCheck
        VX_SCOPED_SAFELOCK(&MemLeakHelper::leak_info.lock);

		std::vector<LPVOID> call_stack(10, 0);
		ULONG hash = 0;
		CaptureStackBackTrace(0, call_stack.size(), &call_stack[0], &hash);

		MemLeakHelper::leak_info.addresses[this] = hash;
		MemLeakHelper::leak_info.callstack_alloc[hash] = call_stack;
#endif
    }
    virtual ~IXInterfaceImpl()
    {   
#ifdef MemLeakCheck
        VX_SCOPED_SAFELOCK(&MemLeakHelper::leak_info.lock);
        MemLeakHelper::leak_info.addresses.erase(this);
#endif
    }   

    IXInterfaceImpl(const IXInterfaceImpl&)
        : m_lRefCount(0)
    {
        // 拷贝构造时，引用计数不拷贝
#ifdef MemLeakCheck
        VX_SCOPED_SAFELOCK(&MemLeakHelper::leak_info.lock);

		std::vector<LPVOID> call_stack(10, 0);
		ULONG hash = 0;
		CaptureStackBackTrace(0, call_stack.size(), &call_stack[0], &hash);

		MemLeakHelper::leak_info.addresses[this] = hash;
		MemLeakHelper::leak_info.callstack_alloc[hash] = call_stack;
#endif
    }
    IXInterfaceImpl& operator=(const IXInterfaceImpl&)
    {
        // 赋值时，保持当前引用计数不变
        return *this;
    }

public:
    virtual long AddRef()
    {
        return _InterlockedIncrement(&m_lRefCount);
    }
    virtual long Release()
    {
        long ret = _InterlockedDecrement(&m_lRefCount);
        assert(ret >= 0);
        if (ret == 0)
        {
            delete this;
        }
        return ret;
    }

private:
    volatile long m_lRefCount;
};

/// 用于单例模式
template <typename IDerive>
class __declspec(novtable) IXInterfaceImpl_NoRef : public IDerive
{
protected:
    IXInterfaceImpl_NoRef()
    {    }
    virtual ~IXInterfaceImpl_NoRef()
    {    }   

public:
    virtual long AddRef()
    {
        return 1;
    }
    virtual long Release()
    {
        return 2;
    }
};

#pragma region 智能枚举器接口
template<class T>
class __declspec(novtable) IXEnumerator : public IXInterface
{
public:
    typedef T ElementType;

    virtual unsigned long Count() = 0;
    virtual void	Reset() = 0;
    virtual bool	MoveNext() = 0;
    virtual ElementType Current() = 0;
};

#ifdef USE_BOOST
#if _MSC_VER >= 1900
using CSmartEnumerPtr = boost::intrusive_ptr<IXEnumerator<T>>;
#else
template<typename T>
struct CSmartEnumerPtr : public boost::intrusive_ptr<IXEnumerator<T>>
{
public:
    CSmartEnumerPtr(IXEnumerator<T>* p, bool bAddRef = false)
        : boost::intrusive_ptr<IXEnumerator<T>>(p, bAddRef)
    {

    }

    // 本类仅是一个辅助类，屏蔽各类拷贝和赋值运算
private:
    template<class U> CSmartEnumerPtr(CSmartEnumerPtr<U> const & rhs);
    CSmartEnumerPtr(CSmartEnumerPtr const & rhs);
    CSmartEnumerPtr & operator=(T * rhs);
    template<class U> CSmartEnumerPtr & operator=(CSmartEnumerPtr<U> const & rhs);
    CSmartEnumerPtr & operator=(CSmartEnumerPtr const & rhs);
};
#endif

#else

#if _MSC_VER >= 1900
template<typename T>
using CSmartEnumerPtr = VxSmartInterfacePtr<IXEnumerator<T>>;
#else
template<typename T>
struct CSmartEnumerPtr : public VxSmartInterfacePtr<IXEnumerator<T>>
{
public:
    CSmartEnumerPtr(IXEnumerator<T>* p, bool bAddRef = false)
        : VxSmartInterfacePtr<IXEnumerator<T>>(p, bAddRef)
    {

    }

    // 本类仅是一个辅助类，屏蔽各类拷贝和赋值运算
private:
    template<class U> CSmartEnumerPtr(CSmartEnumerPtr<U> const & rhs);
    CSmartEnumerPtr(CSmartEnumerPtr const & rhs);
    CSmartEnumerPtr & operator=(T * rhs);
    template<class U> CSmartEnumerPtr & operator=(CSmartEnumerPtr<U> const & rhs);
    CSmartEnumerPtr & operator=(CSmartEnumerPtr const & rhs);
};
#endif

#endif

namespace std
{
    template<class _Arg, class _Result>
    struct convert: public std::unary_function<_Arg, _Result>
    {	
        result_type operator()(const argument_type& _Left) const
        {	
            return _Result(_Left);
        }
    };

    template<>
    struct convert<std::string, const char*> : public std::unary_function<std::string, const char*>
    {
        const char* operator()(const std::string& _Left) const
        {	
            return _Left.c_str();
        }
    };

    template<class _Fn1>
    class unary_convert : public std::unary_function<typename _Fn1::argument_type, typename _Fn1::result_type>
    {
    public:
        explicit unary_convert(const _Fn1& _Func)
            : _Functor(_Func)
        {	// construct from functor
        }

        typename _Fn1::result_type operator()(const typename _Fn1::argument_type& _Left) const
        {	// apply functor to operand
            return (_Functor(_Left));
        }

    protected:
        _Fn1 _Functor;	// the functor to apply
    };

    template<class T>
    struct convert<VxSmartInterfacePtr<T>, T*> : public unary_function<VxSmartInterfacePtr<T>, T*>
    {
        T* operator()(const VxSmartInterfacePtr<T>& _Left) const
        {
            return _Left.get();
        }
    };

#ifdef USE_BOOST
    template<class T>
    struct convert<boost::intrusive_ptr<T>, T*> : public unary_function<boost::intrusive_ptr<T>, T*>
    {
        T* operator()(const boost::intrusive_ptr<T>& _Left) const
        {
            return _Left.get();
        }
    };
#endif
}

template<class T_In, 
class T_Out = T_In, 
class Pr = std::convert<T_In, T_Out>,
class Ax = std::allocator<T_In> > 
class CArrayEnumerator 
    : public std::vector<T_In, Ax>
    , public IXInterfaceImpl<IXEnumerator<T_Out> >
{
    typedef typename IXEnumerator<T_Out>::ElementType ElementType;

public:
    CArrayEnumerator()
    {
        bNeedReset = true;
        bReset = false;
    }

    virtual ~CArrayEnumerator()
    {

    }

    virtual unsigned long Count()
    {
        return size();
    }

    virtual void Reset()
    {
        bNeedReset = false;
        bReset = true;
        m_iterCur = begin();
    }

    virtual bool MoveNext()
    {
        if(bNeedReset)
        {
            Reset();
            bNeedReset = false;
        }

        if(m_iterCur == end())
        {
            bNeedReset = true;
            return false;
        }

        if(bReset)
        {
            bReset = false;
            return true;
        }

        if(++m_iterCur == end())
        {
            bNeedReset = true;
            return false;
        }

        return true;
    }

    virtual ElementType Current()
    {
        if(bReset)
        {
            throw std::exception("Please call MoveNext() before call Current().");
        }
        else
        {
            if(m_iterCur == end())
            {
                throw std::exception("End position");
            }
        }

        return convFunc(*m_iterCur);
    }

private:
    const_iterator      m_iterCur;
    bool                bReset;
    bool                bNeedReset;
    Pr                  convFunc;
};

template<class T_In, 
class T_Out = T_In, 
class Pr = std::convert<T_In, T_Out>,
class Fr = std::less<T_In>,
class Ax = std::allocator<T_In> > 
class CSetEnumerator 
    : public std::set<T_In, Fr, Ax>
    , public IXInterfaceImpl<IXEnumerator<T_Out> >
{
    typedef typename IXEnumerator<T_Out>::ElementType ElementType;

public:
    CSetEnumerator()
    {
        bNeedReset = true;
        bReset = false;
    }

    virtual ~CSetEnumerator()
    {

    }

    virtual unsigned long Count()
    {
        return size();
    }

    virtual void Reset()
    {
        bNeedReset = false;
        bReset = true;
        m_iterCur = begin();
    }

    virtual bool MoveNext()
    {
        if(bNeedReset)
        {
            Reset();
            bNeedReset = false;
        }

        if(m_iterCur == end())
        {
            bNeedReset = true;
            return false;
        }

        if(bReset)
        {
            bReset = false;
            return true;
        }

        if(++m_iterCur == end())
        {
            bNeedReset = true;
            return false;
        }

        return true;
    }

    virtual ElementType Current()
    {
        if(bReset)
        {
            throw std::exception("Please call MoveNext() before call Current().");
        }
        else
        {
            if(m_iterCur == end())
            {
                throw std::exception("End position");
            }
        }

        return convFunc(*m_iterCur);
    }

private:
    const_iterator      m_iterCur;
    bool                bReset;
    bool                bNeedReset;
    Pr                  convFunc;
};

template<class Kty, class Ty>
class __declspec(novtable) IXMapEnumerator : public IXInterface
{
public:
    typedef std::pair<const Kty, Ty> ElementType;

    virtual unsigned long Count() = 0;
    virtual void	Reset() = 0;
    virtual bool	MoveNext() = 0;
    virtual ElementType Current() = 0;
};

template<class Kty_in,
class Ty_in,
class Kty_out = Kty_in,
class Ty_out = Ty_in,
class Kfn = std::convert<Kty_in, Kty_out> ,
class Fn = std::convert<Ty_in, Ty_out> ,
class Pr = std::less<Kty_in> ,
class Alloc = std::allocator<std::pair<const Kty_in, Ty_in> > >
class CMapEnumerator
    : public std::map<Kty_in, Ty_in, Pr, Alloc>
    , public IXInterfaceImpl<IXMapEnumerator<Kty_out, Ty_out> >
{
    typedef typename IXMapEnumerator<Kty_out, Ty_out>::ElementType ElementType;

public:
    CMapEnumerator()
    {
        bNeedReset = true;
        bReset = false;
    }

    virtual ~CMapEnumerator()
    {

    }

    virtual unsigned long Count()
    {
        return size();
    }

    virtual void Reset()
    {
        bNeedReset = false;
        bReset = true;
        m_iterCur = begin();
    }

    virtual bool MoveNext()
    {
        if(bNeedReset)
        {
            Reset();
            bNeedReset = false;
        }

        if(m_iterCur == end())
        {
            bNeedReset = true;
            return false;
        }

        if(bReset)
        {
            bReset = false;
            return true;
        }

        if(++m_iterCur == end())
        {
            bNeedReset = true;
            return false;
        }

        return true;
    }

    virtual ElementType Current()
    {
        if(bReset)
        {
            throw std::exception("Please call MoveNext() before call Current().");
        }
        else
        {
            if(m_iterCur == end())
            {
                throw std::exception("End position");
            }
        }

        return std::make_pair(kfn(m_iterCur->first), fn(m_iterCur->second));
    }

private:
    const_iterator      m_iterCur;
    bool                bReset;
    bool                bNeedReset;
    Kfn                 kfn;
    Fn                  fn;
};
#pragma endregion


#pragma region 字符串接口
template<typename T>
struct __declspec(novtable) IXString : public IXInterface
{
    virtual const T* GetString()const = 0;
};
typedef IXString<char>      IXStringA;
typedef IXString<wchar_t>   IXStringW;
typedef IXString<TCHAR>     IXStringT;
X_SMARTPTR_TYPEDEF(IXStringA);
X_SMARTPTR_TYPEDEF(IXStringW);
X_SMARTPTR_TYPEDEF(IXStringT);

template<typename T>
class CXString 
    : public std::basic_string<T, std::char_traits<T>, std::allocator<T>>
    , public IXInterfaceImpl<IXString<T>>
{
    typedef typename std::basic_string<T, std::char_traits<T>, std::allocator<T>> BasicString;
public:
    CXString(){};
    CXString(const T* lpszBuf)
        : BasicString(lpszBuf){}
    CXString(const BasicString & buf)
        : BasicString(buf){}
    virtual ~CXString(){}

public:
    virtual const T * GetString()const{
        return c_str();
    }
};

struct __declspec(novtable) IXMemBuf : public IXInterface
{
    virtual const char* Data()const = 0;
    virtual unsigned long Size()const = 0;
};
X_SMARTPTR_TYPEDEF(IXMemBuf);

class CXMemBuf : public IXInterfaceImpl<IXMemBuf>, public std::string
{
public:
    CXMemBuf(){}
    CXMemBuf(const char* lpBuf, unsigned long ulSize)
        : std::string(lpBuf, ulSize){}
    CXMemBuf(const std::string& buf)
        : std::string(buf){}
    virtual ~CXMemBuf(){}

public:
    virtual const char* Data()const{
        return data();
    }
    virtual unsigned long Size()const{
        return size();
    }
};

#pragma endregion

#pragma region 回调接口
// 回调接口，一般不使用智能指针，因为很容易导致智能指针的循环引用，从而无法释放内存
struct __declspec(novtable) IXCallBack
{
    virtual HRESULT OnCallBack(DWORD dwParam) = 0;
};

typedef long HXCALLBACK;
#define INVALID_HXCALLBACK_VALUE  ((HXCALLBACK)(long)-1)
// 由于回调对象无法使用智能指针，用在异步调用上时，很容易出现野指针。故增加统一的回调生命周期管理接口，以避免野指针出现
// 使用限制：创建方和调用方需共用同一个该回调管理对象
struct __declspec(novtable) IXCallBackMgr : public IXInterface
{
    // 为指定回调分配一个句柄，创建者保存该句柄并将该句柄传递给调用者
    virtual HXCALLBACK Assign(IXCallBack* pCallback) = 0;
    // 创建者控制其生命周期，再该回调对象结束生命周期结束前，关闭之
    virtual void Close(HXCALLBACK hCallback) = 0;
    // 调用者触发回调，若该回调已释放则其依旧是安全的
    virtual HRESULT Fire(HXCALLBACK hCallback, DWORD dwParam) = 0;
};
X_SMARTPTR_TYPEDEF(IXCallBackMgr);

class CXCallBackMgr : public IXInterfaceImpl<IXCallBackMgr>
{
public:
    CXCallBackMgr()
        : m_spLock(new VxCriticalSectionLock)
        , m_lRefAssignedSeed(0)
    {}
    ~CXCallBackMgr(){}

public:
    virtual HXCALLBACK Assign(IXCallBack* pCallback)
    {
        if(nullptr == pCallback)
        {
            return INVALID_HXCALLBACK_VALUE;
        }
        VX_SCOPED_SAFELOCK(m_spLock.get());
        HXCALLBACK hTarget = (HXCALLBACK)_InterlockedIncrement(&m_lRefAssignedSeed);
        if(hTarget < 0)
        {
            m_lRefAssignedSeed = 0;
            hTarget = (HXCALLBACK)_InterlockedIncrement(&m_lRefAssignedSeed);
        }
        m_mapCallback[hTarget] = pCallback;

        return hTarget;
    }
    virtual void Close(HXCALLBACK hCallback)
    {
        VX_SCOPED_SAFELOCK(m_spLock.get());
        m_mapCallback.erase(hCallback);
    }
    virtual HRESULT Fire(HXCALLBACK hCallback, DWORD dwParam)
    {
        VX_SCOPED_SAFELOCK(m_spLock.get());
        if(m_mapCallback.find(hCallback) == m_mapCallback.end())
        {
            return S_FALSE;
        }

        return m_mapCallback[hCallback]->OnCallBack(dwParam);
    }

private:
    std::unique_ptr<VxCriticalSectionLock>      m_spLock;
    volatile long                               m_lRefAssignedSeed;
    std::map<HXCALLBACK, IXCallBack*>           m_mapCallback;
};
#pragma endregion

/// 使用标准示例
#if 0
#pragma region interface.h
#include <IXInterface.h>

/// 只能使用POD类型结构体
#pragma pack(push, 4)
struct NORMAL_DATA
{
    unsigned int _ver;
    int _int;
    float _float;
    char _char[128];
};
#pragma pack(pop)

struct INormalLogic : public IXInterface
{
    virtual long Func_InnerType(int param) = 0;
    virtual long Func_PodType(const NORMAL_DATA & data) = 0;
};
X_SMARTPTR_TYPEDEF(INormalLogic);

struct IDeriveLogic : public virtual INormalLogic
{
    virtual long Func_OutIntf(INormalLogic ** ppOut) = 0;
    inline INormalLogicPtr Func_OutIntf(void)
    {
        INormalLogicPtr spOut;
        long lRet = Func_OutIntf(spOut.address());
        SetLastError(lRet);
        return spOut;
    }
    virtual long Func_InIntf(INormalLogic * pIn) = 0;
};
X_SMARTPTR_TYPEDEF(IDeriveLogic);

#define XXX_EXPORTS /// defined in project config

#ifdef XXX_EXPORTS
#define XXX_API extern "C" __declspec(dllexport)
#else
#define XXX_API extern "C" __declspec(dllimport)
#endif

XXX_API long CreateNormalLogic(INormalLogic ** ppOut);
XXX_API long CreateDeriveLogic(IDeriveLogic ** ppOut);

inline INormalLogicPtr CreateNormalLogic(void)
{
    INormalLogicPtr spOut;
    long lRet = CreateNormalLogic(spOut.address());
    SetLastError(lRet);
    return spOut;
}

inline IDeriveLogicPtr CreateDeriveLogic(void)
{
    IDeriveLogicPtr spOut;
    long lRet = CreateDeriveLogic(spOut.address());
    SetLastError(lRet);
    return spOut;
}

#pragma endregion

#pragma region Interface_impl.h/cpp
#include <IXInterfaceImpl.h>

#ifndef NOT_USE_TEMPLATE
template<typename IDerive>
class INormalLogicImpl : public IXInterfaceImpl<IDerive>
{
public:
    INormalLogicImpl() {}
    virtual ~INormalLogicImpl() {};


    virtual long Func_InnerType(int param) override
    {
        if (param < 0)
        {
            return E_INVALIDARG;
        }

        /// Todo : something

        return S_OK;
    }


    virtual long Func_PodType(const NORMAL_DATA & data) override
    {
        switch (data._ver)
        {
        case 1:
            /// do something
            break;
        case 2:
            /// do something
            break;
        default:
            return E_INVALIDARG;
            break;
        }
        return S_OK;
    }

private:

};
typedef INormalLogicImpl<INormalLogic> CNormalLogic;

class CDeriveLogic final : public INormalLogicImpl<IDeriveLogic>
{
public:
    CDeriveLogic() {}
    ~CDeriveLogic() {}


    virtual long Func_OutIntf(INormalLogic ** ppOut) override
    {
        if (nullptr == ppOut)
        {
            return E_INVALIDARG;
        }

        INormalLogicPtr spOut = new CNormalLogic;
        *ppOut = spOut.detach();

        return S_OK;
    }


    virtual long Func_InIntf(INormalLogic * pIn) override
    {
        if (!pIn)
        {
            return E_INVALIDARG;
        }

        _spNormalLogic.reset(pIn, true);

        return S_OK;
    }

private:
    INormalLogicPtr			_spNormalLogic;
};

#else
class CNormalLogic : public IXInterfaceImpl<INormalLogic>
{
public:
    CNormalLogic() {}
    virtual ~CNormalLogic() {}


    virtual long Func_InnerType(int param) override
    {
        throw std::logic_error("The method or operation is not implemented.");
    }


    virtual long Func_PodType(const NORMAL_DATA & data) override
    {
        throw std::logic_error("The method or operation is not implemented.");
    }

};

class CDeriveLogic final
    : public virtual IDeriveLogic
    , public CNormalLogic
{
public:
    CDeriveLogic() {}
    ~CDeriveLogic() {}


    virtual long Func_OutIntf(INormalLogic ** ppOut) override
    {
        throw std::logic_error("The method or operation is not implemented.");
    }


    virtual long Func_InIntf(INormalLogic * pIn) override
    {
        throw std::logic_error("The method or operation is not implemented.");
    }

};

#endif // NOT_USE_TEMEPLATE

XXX_API long CreateNormalLogic(INormalLogic ** ppOut)
{
    if (nullptr == ppOut)
    {
        return E_INVALIDARG;
    }

    IDeriveLogicPtr spOut = new CDeriveLogic;
    *ppOut = spOut.detach();

    return S_OK;
}

XXX_API long CreateDeriveLogic(IDeriveLogic ** ppOut)
{
    if (nullptr == ppOut)
    {
        return E_INVALIDARG;
    }

    CDeriveLogic * pTarget = new CDeriveLogic;
    if (pTarget)
    {
        pTarget->AddRef();
    }

    *ppOut = dynamic_cast<IDeriveLogic*>(pTarget);

    return S_OK;
}

#pragma endregion


#pragma region client.cpp
void client_func()
{
    CreateDeriveLogic()->Func_OutIntf()->Func_InnerType(1);

    auto spLogic = CreateNormalLogic();
    NORMAL_DATA data = { 1, 0 };
    spLogic->Func_PodType(data);
}
#pragma endregion
#endif
