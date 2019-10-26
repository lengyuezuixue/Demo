#ifndef SharedMemory_H
#define SharedMemory_H

#include <cstdint>
#include <string>

#ifdef _WIN32
#include <windows.h>
#define _INFINITE (INFINITE)
#else
#include <sys/types.h>
#define _INFINITE (1000)
#endif

namespace sm {

class SystemSemaphore
{
public:
    enum AccessMode
    {
        Open,
        Create
    };

    enum SystemSemaphoreError
    {
        NoError,
        PermissionDenied,
        KeyError,
        AlreadyExists,
        NotFound,
        OutOfResources,
        UnknownError
    };

    explicit SystemSemaphore(const std::string &key, int initialValue = 0, AccessMode mode = Open);
    ~SystemSemaphore();

    void setKey(const std::string &key, int initialValue = 0, AccessMode mode = Open);
    const std::string key() const;

    bool acquire();
    bool acquire(unsigned long millisecond);
    bool release(int n = 1);

    SystemSemaphoreError error() const;
    const std::string errorString() const;

private:
    inline void setError(SystemSemaphore::SystemSemaphoreError e, const std::string &msg)
    {
        m_error = e;
        m_errorString = msg;
    }

    inline void clearError()
    {
        setError(SystemSemaphore::NoError, std::string());
    }

#ifdef _WIN32
    HANDLE handle(SystemSemaphore::AccessMode mode = SystemSemaphore::Open);
#else
    key_t handle(SystemSemaphore::AccessMode mode = SystemSemaphore::Open);
#endif
    void cleanHandle();
    bool modifySemaphore(int count, unsigned long millisecond = _INFINITE);
    void setErrorString(const std::string &function);

private:
    std::string                            m_key;
    std::string                            m_fileName;
    int                                    m_initialValue;
#ifdef _WIN32
    HANDLE                                 m_semaphore;
#else
    key_t                                  m_unixKey;
    int                                    m_semaphore;
    bool                                   m_createdFile;
    bool                                   m_createdSemaphore;
#endif
    SystemSemaphore::SystemSemaphoreError  m_error;
    std::string                            m_errorString;
};

class SharedMemory
{
public:
    enum AccessMode
    {
        ReadOnly,
        ReadWrite
    };

    enum SharedMemoryError
    {
        NoError,
        PermissionDenied,
        InvalidSize,
        KeyError,
        AlreadyExists,
        NotFound,
        LockError,
        OutOfResources,
        UnknownError
    };

    explicit SharedMemory();
    explicit SharedMemory(const std::string &key);
    ~SharedMemory();

    void setKey(const std::string &key);

    std::string key() const;
    std::string nativeKey() const;

    bool create(int size, AccessMode mode = ReadWrite);
    int size() const;

    bool attach(AccessMode mode = ReadWrite);
    bool isAttached() const;
    bool detach();

    void *data();
    const void *constData() const;
    const void *data() const;

    bool lock();
    bool tryLock(unsigned long millisecond);
    bool unlock();

    SharedMemoryError error() const;
    std::string errorString() const;

public:
    static std::string getAppDir();
    static std::string getTempDir();
    static std::string getHomeDir();
    static std::string getDesktopDir();
    static bool removeFile(const std::string &path);
    static std::string makePlatformSafeKey(const std::string &key,
                                           const std::string &prefix = std::string("sm_sharedmemory_"));
    static std::string strToUtf8(const std::string & str);
    static std::string sha1(const std::string& str);
#ifndef _WIN32
    static int createUnixKeyFile(const std::string &fileName);
#endif
    static void print(const std::string &str);

private:
    bool initKey();
#ifdef _WIN32
    HANDLE handle(AccessMode mode = ReadWrite);
#else
    int handle(AccessMode mode = ReadWrite);
    unsigned long  getFileSize(const std::string &path);
    static void delSpecifiedFiles(const std::string &path, const std::string prefix = std::string());
#endif
    bool cleanHandle();
    void setErrorString(const std::string &function);

private:
    void                            *m_memory;
    unsigned long                    m_size;
    std::string                      m_key;
    std::string                      m_nativeKey;
    SharedMemory::SharedMemoryError  m_error;
    std::string                      m_errorString;
    SystemSemaphore                  m_systemSemaphore;
    bool                             m_lockedByMe;
#ifdef _WIN32
    HANDLE                           m_handle;
#else
    int                              m_handle;
    bool                             m_createdFile;
#endif
};
}

#endif // SharedMemory_H
