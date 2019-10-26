
#include "pch.h"
#include "SharedMemory.h"

#include <regex>
#include <sstream>
#include <iomanip>
#include <locale>
#include <codecvt>
#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#endif

namespace sm {

static const size_t BLOCK_INTS = 16;
static const size_t BLOCK_BYTES = BLOCK_INTS * 4;

static void reset(uint32_t digest[], std::string &buffer, uint64_t &transforms)
{
    // Initialization constants
    digest[0] = 0x67452301;
    digest[1] = 0xefcdab89;
    digest[2] = 0x98badcfe;
    digest[3] = 0x10325476;
    digest[4] = 0xc3d2e1f0;

    // Reset counters
    buffer = "";
    transforms = 0;
}

static uint32_t rol(const uint32_t value, const size_t bits)
{
    return (value << bits) | (value >> (32 - bits));
}

static uint32_t blk(const uint32_t block[BLOCK_INTS], const size_t i)
{
    return rol(block[(i+13)&15] ^ block[(i+8)&15] ^ block[(i+2)&15] ^ block[i], 1);
}

static void R0(const uint32_t block[BLOCK_INTS], const uint32_t v, uint32_t &w, const uint32_t x, const uint32_t y, uint32_t &z, const size_t i)
{
    z += ((w&(x^y))^y) + block[i] + 0x5a827999 + rol(v, 5);
    w = rol(w, 30);
}

static void R1(uint32_t block[BLOCK_INTS], const uint32_t v, uint32_t &w, const uint32_t x, const uint32_t y, uint32_t &z, const size_t i)
{
    block[i] = blk(block, i);
    z += ((w&(x^y))^y) + block[i] + 0x5a827999 + rol(v, 5);
    w = rol(w, 30);
}

static void R2(uint32_t block[BLOCK_INTS], const uint32_t v, uint32_t &w, const uint32_t x, const uint32_t y, uint32_t &z, const size_t i)
{
    block[i] = blk(block, i);
    z += (w^x^y) + block[i] + 0x6ed9eba1 + rol(v, 5);
    w = rol(w, 30);
}

static void R3(uint32_t block[BLOCK_INTS], const uint32_t v, uint32_t &w, const uint32_t x, const uint32_t y, uint32_t &z, const size_t i)
{
    block[i] = blk(block, i);
    z += (((w|x)&y)|(w&x)) + block[i] + 0x8f1bbcdc + rol(v, 5);
    w = rol(w, 30);
}

static void R4(uint32_t block[BLOCK_INTS], const uint32_t v, uint32_t &w, const uint32_t x, const uint32_t y, uint32_t &z, const size_t i)
{
    block[i] = blk(block, i);
    z += (w^x^y) + block[i] + 0xca62c1d6 + rol(v, 5);
    w = rol(w, 30);
}

static void transform(uint32_t digest[], uint32_t block[BLOCK_INTS], uint64_t &transforms)
{
    // Copy digest[] to working vars
    uint32_t a = digest[0];
    uint32_t b = digest[1];
    uint32_t c = digest[2];
    uint32_t d = digest[3];
    uint32_t e = digest[4];

    // 4 rounds of 20 operations each. Loop unrolled.
    R0(block, a, b, c, d, e,  0); R0(block, e, a, b, c, d,  1);
    R0(block, d, e, a, b, c,  2); R0(block, c, d, e, a, b,  3);
    R0(block, b, c, d, e, a,  4); R0(block, a, b, c, d, e,  5);
    R0(block, e, a, b, c, d,  6); R0(block, d, e, a, b, c,  7);
    R0(block, c, d, e, a, b,  8); R0(block, b, c, d, e, a,  9);
    R0(block, a, b, c, d, e, 10); R0(block, e, a, b, c, d, 11);
    R0(block, d, e, a, b, c, 12); R0(block, c, d, e, a, b, 13);
    R0(block, b, c, d, e, a, 14); R0(block, a, b, c, d, e, 15);

    R1(block, e, a, b, c, d,  0); R1(block, d, e, a, b, c,  1);
    R1(block, c, d, e, a, b,  2); R1(block, b, c, d, e, a,  3);

    R2(block, a, b, c, d, e,  4); R2(block, e, a, b, c, d,  5);
    R2(block, d, e, a, b, c,  6); R2(block, c, d, e, a, b,  7);
    R2(block, b, c, d, e, a,  8); R2(block, a, b, c, d, e,  9);
    R2(block, e, a, b, c, d, 10); R2(block, d, e, a, b, c, 11);
    R2(block, c, d, e, a, b, 12); R2(block, b, c, d, e, a, 13);
    R2(block, a, b, c, d, e, 14); R2(block, e, a, b, c, d, 15);
    R2(block, d, e, a, b, c,  0); R2(block, c, d, e, a, b,  1);
    R2(block, b, c, d, e, a,  2); R2(block, a, b, c, d, e,  3);
    R2(block, e, a, b, c, d,  4); R2(block, d, e, a, b, c,  5);
    R2(block, c, d, e, a, b,  6); R2(block, b, c, d, e, a,  7);

    R3(block, a, b, c, d, e,  8); R3(block, e, a, b, c, d,  9);
    R3(block, d, e, a, b, c, 10); R3(block, c, d, e, a, b, 11);
    R3(block, b, c, d, e, a, 12); R3(block, a, b, c, d, e, 13);
    R3(block, e, a, b, c, d, 14); R3(block, d, e, a, b, c, 15);
    R3(block, c, d, e, a, b,  0); R3(block, b, c, d, e, a,  1);
    R3(block, a, b, c, d, e,  2); R3(block, e, a, b, c, d,  3);
    R3(block, d, e, a, b, c,  4); R3(block, c, d, e, a, b,  5);
    R3(block, b, c, d, e, a,  6); R3(block, a, b, c, d, e,  7);
    R3(block, e, a, b, c, d,  8); R3(block, d, e, a, b, c,  9);
    R3(block, c, d, e, a, b, 10); R3(block, b, c, d, e, a, 11);

    R4(block, a, b, c, d, e, 12); R4(block, e, a, b, c, d, 13);
    R4(block, d, e, a, b, c, 14); R4(block, c, d, e, a, b, 15);
    R4(block, b, c, d, e, a,  0); R4(block, a, b, c, d, e,  1);
    R4(block, e, a, b, c, d,  2); R4(block, d, e, a, b, c,  3);
    R4(block, c, d, e, a, b,  4); R4(block, b, c, d, e, a,  5);
    R4(block, a, b, c, d, e,  6); R4(block, e, a, b, c, d,  7);
    R4(block, d, e, a, b, c,  8); R4(block, c, d, e, a, b,  9);
    R4(block, b, c, d, e, a, 10); R4(block, a, b, c, d, e, 11);
    R4(block, e, a, b, c, d, 12); R4(block, d, e, a, b, c, 13);
    R4(block, c, d, e, a, b, 14); R4(block, b, c, d, e, a, 15);

    // Add the working vars back into digest[]
    digest[0] += a;
    digest[1] += b;
    digest[2] += c;
    digest[3] += d;
    digest[4] += e;

    // Count the number of transformations
    transforms++;
}

static void bufferToBlock(const std::string &buffer, uint32_t block[BLOCK_INTS])
{
    // Convert the std::string (byte buffer) to a uint32_t array
    for (size_t i = 0; i < BLOCK_INTS; i++) {
        block[i] = (buffer[4*i+3] & 0xff)
                | (buffer[4*i+2] & 0xff)<<8
                                          | (buffer[4*i+1] & 0xff)<<16
                                                                    | (buffer[4*i+0] & 0xff)<<24;
    }
}

#ifdef _WIN32
// char* -> wchar*
wchar_t* char2wchar(const char* pchar, int nLen)
{
    int nSize = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pchar, nLen, 0, 0);
    if (nSize <= 0) {
        return NULL;
    }
    WCHAR *pwszDst = new WCHAR[nSize + 1];
    if (NULL == pwszDst) {
        return NULL;
    }
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pchar, nLen, pwszDst, nSize);
    pwszDst[nSize] = 0;

    return pwszDst;
}
// wchar* -> char*
char* wchar2char(const wchar_t* pwchar)
{
    int nLen = WideCharToMultiByte(CP_ACP, 0, pwchar, -1, NULL, 0, NULL, NULL);
    if (nLen <= 0) {
        return NULL;
    }
    char* pszDst = new char[nLen];
    if (NULL == pszDst) {
        return NULL;
    }
    WideCharToMultiByte(CP_ACP, 0, pwchar, -1, pszDst, nLen, NULL, NULL);
    pszDst[nLen - 1] = 0;
    return pszDst;
}
// wchar_t* -> string
std::string wchar2string(const wchar_t* pwchar)
{
    int nLen = WideCharToMultiByte(CP_ACP, 0, pwchar, -1, NULL, 0, NULL, NULL);
    if (nLen <= 0){
        return std::string("");
    }
    char* pszDst = new char[nLen];
    if (NULL == pszDst) {
        return std::string("");
    }
    WideCharToMultiByte(CP_ACP, 0, pwchar, -1, pszDst, nLen, NULL, NULL);
    pszDst[nLen - 1] = 0;
    std::string strTemp(pszDst);
    delete [] pszDst;
    return strTemp;
}
// wstring -> string
std::string wstring2string(const std::wstring & wstr)
{
    return wchar2string(wstr.c_str());
}
// char* -> wstring
std::wstring char2wstring(const char* pchar, int nLen)
{
    int nSize = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pchar, nLen, 0, 0);
    if (nSize <= 0) {
        return NULL;
    }
    WCHAR *pwszDst = new WCHAR[nSize + 1];
    if (NULL == pwszDst) {
        return NULL;
    }
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pchar, nLen, pwszDst, nSize);
    pwszDst[nSize] = 0;
    // skip Oxfeff
    if (pwszDst[0] == 0xFEFF) {
        for (int i = 0; i < nSize; i++) {
            pwszDst[i] = pwszDst[i + 1];
        }
    }
    std::wstring wcharString(pwszDst);
    delete [] pwszDst;
    return wcharString;
}
// string -> wstring
std::wstring string2wstring(const std::string & str)
{
    return char2wstring(str.c_str(), str.size());
}

#endif

/*----------------------------------------------------------------------------------------*/

SystemSemaphore::SystemSemaphore(const std::string &key, int initialValue, AccessMode mode)
    : m_fileName(std::string())
    #ifdef _WIN32
    , m_semaphore(0)
    #else
    , m_unixKey(-1)
    , m_semaphore(-1)
    , m_createdFile(false)
    , m_createdSemaphore(false)
    #endif
    , m_error(SystemSemaphore::NoError)
    , m_errorString(std::string())
{
    setKey(key, initialValue, mode);
}

SystemSemaphore::~SystemSemaphore()
{
    cleanHandle();
}

void SystemSemaphore::setKey(const std::string &key, int initialValue, SystemSemaphore::AccessMode mode)
{
    if (key == m_key && mode == Open) {
        return;
    }

    clearError();

#ifndef _WIN32
    // Create
    if (key == m_key && mode == Create && m_createdSemaphore && m_createdFile) {
        m_initialValue = initialValue;
        m_unixKey = -1;
        handle(mode);
        return;
    }
#endif

    cleanHandle();

    m_key = key;
    m_initialValue = initialValue;
    m_fileName = SharedMemory::makePlatformSafeKey(m_key, std::string("ipc_systemsem_"));

    handle(mode);
}

const std::string SystemSemaphore::key() const
{
    return m_key;
}

bool SystemSemaphore::acquire()
{
    return modifySemaphore(-1);
}

bool SystemSemaphore::acquire(unsigned long millisecond)
{
    return modifySemaphore(-1, millisecond);
}

bool SystemSemaphore::release(int n)
{
    if (n == 0) {
        return true;
    }
    if (n < 0) {
        SharedMemory::print("SystemSemaphore::release : n is negative.");
        return false;
    }
    return modifySemaphore(n);
}

SystemSemaphore::SystemSemaphoreError SystemSemaphore::error() const
{
    return m_error;
}

const std::string SystemSemaphore::errorString() const
{
    return m_errorString;
}
#ifdef _WIN32
HANDLE SystemSemaphore::handle(SystemSemaphore::AccessMode mode)
{
    if (m_key.empty()) {
        return 0;
    }

    // Create it if it doesn't already exists.
    if (m_semaphore == 0) {
        m_semaphore = CreateSemaphore(0, m_initialValue, MAXLONG, string2wstring(m_fileName).c_str());
        if (m_semaphore == 0) {
            setErrorString(std::string("SystemSemaphore::handle"));
        }
    }

    return m_semaphore;
}
#else
key_t SystemSemaphore::handle(SystemSemaphore::AccessMode mode)
{
    if (m_key.empty()){
        m_error = SystemSemaphore::KeyError;
        m_errorString = std::string("SystemSemaphore::handle : key is empty");
        return -1;
    }

    // Ftok requires that an actual file exists somewhere
    if (-1 != m_unixKey) {
        return m_unixKey;
    }

    // Create the file needed for ftok
    int built = SharedMemory::createUnixKeyFile(m_fileName);
    if (-1 == built) {
        m_errorString = std::string("SystemSemaphore::handle : unable to make key");
        m_error = SystemSemaphore::KeyError;
        return -1;
    }

    m_createdFile = (1 == built);

    // Get the unix key for the created file
    m_unixKey = ftok(m_fileName.c_str(), 'S');
    if (-1 == m_unixKey) {
        m_errorString = std::string("SystemSemaphore::handle : ftok failed");
        m_error = SystemSemaphore::KeyError;
        return -1;
    }

    // Get semaphore
    m_semaphore = semget(m_unixKey, 1, 0600 | IPC_CREAT | IPC_EXCL);
    if (-1 == m_semaphore) {
        if (errno == EEXIST) {
            m_semaphore = semget(m_unixKey, 1, 0600 | IPC_CREAT);
        }

        if (-1 == m_semaphore) {
            setErrorString(std::string("SystemSemaphore::handle"));
            cleanHandle();
            return -1;
        }
    } else {
        m_createdSemaphore = true;
        m_createdFile = true;
    }

    if (mode == SystemSemaphore::Create) {
        m_createdSemaphore = true;
        m_createdFile = true;
    }

    // Created semaphore so initialize its value.
    if (m_createdSemaphore && m_initialValue >= 0) {
        union semun sem_union;
        sem_union.val = m_initialValue;
        if (semctl(m_semaphore, 0, SETVAL, sem_union) == -1) {
            setErrorString(std::string("SystemSemaphore::handle"));
            cleanHandle();
            return -1;
        }
    }

    return m_unixKey;
}
#endif

void SystemSemaphore::cleanHandle()
{
#ifdef _WIN32
    if (m_semaphore && !CloseHandle(m_semaphore)) {
        SharedMemory::print("SystemSemaphore::CloseHandle : sem failed");
    }
    m_semaphore = 0;
#else
    m_unixKey = -1;

    // Remove the file if we made it
    if (m_createdFile) {
        SharedMemory::removeFile(m_fileName);
        m_createdFile = false;
    }

    if (m_createdSemaphore) {
        if (-1 != m_semaphore) {
            if (-1 == semctl(m_semaphore, 0, IPC_RMID, 0)) {
                setErrorString(std::string("SystemSemaphore::cleanHandle"));
            }
            m_semaphore = -1;
        }
        m_createdSemaphore = false;
    }
#endif
}

bool SystemSemaphore::modifySemaphore(int count, unsigned long millisecond)
{
#ifdef _WIN32
    if (0 == handle()) {
        return false;
    }

    if (count > 0) {
        if (0 == ReleaseSemaphore(m_semaphore, count, 0)) {
            setErrorString(std::string("SystemSemaphore::modifySemaphore"));
            return false;
        }
    } else {
        if (WAIT_OBJECT_0 != WaitForSingleObjectEx(m_semaphore, millisecond, FALSE)) {
            setErrorString(std::string("SystemSemaphore::modifySemaphore"));
            return false;
        }
    }
#else
    if (-1 == handle()) {
        return false;
    }

    struct sembuf operation;
    operation.sem_num = 0;
    operation.sem_op = count;
    operation.sem_flg = SEM_UNDO;

    int res = semop(m_semaphore, &operation, 1);
    if (-1 == res) {
        // If the semaphore was removed be nice and create it and then modifySemaphore again
        if (errno == EIDRM) {
            m_semaphore = -1;
            cleanHandle();
            handle();
            return modifySemaphore(count);
        }
        setErrorString(std::string("SystemSemaphore::modifySemaphore"));
        return false;
    }
#endif

    clearError();

    return true;
}

void SystemSemaphore::setErrorString(const std::string &function)
{
#ifdef _WIN32
    BOOL windowsError = GetLastError();
    if (windowsError == 0) {
        return;
    }

    std::stringstream ss;
    ss << windowsError;

    switch (windowsError) {
    case ERROR_NO_SYSTEM_RESOURCES:
    case ERROR_NOT_ENOUGH_MEMORY:
        m_error = SystemSemaphore::OutOfResources;
        m_errorString = function + std::string("SystemSemaphore : out of resources");
        break;
    case ERROR_ACCESS_DENIED:
        m_error = SystemSemaphore::PermissionDenied;
        m_errorString = function + std::string("SystemSemaphore : permission denied");
        break;
    default:
        m_error = SystemSemaphore::UnknownError;
        m_errorString = function + std::string("SystemSemaphore : unknown error ") + std::string(ss.str());
    }
#else
    std::stringstream ss;
    ss << errno;

    switch (errno) {
    case EPERM:
    case EACCES:
        m_error = SystemSemaphore::PermissionDenied;
        m_errorString = function + std::string("SystemSemaphore : permission denied");
        break;
    case EEXIST:
        m_error = SystemSemaphore::AlreadyExists;
        m_errorString = function + std::string("SystemSemaphore : already exists");
        break;
    case ENOENT:
        m_error = SystemSemaphore::NotFound;
        m_errorString = function + std::string("SystemSemaphore : does not exist");
        break;
    case ERANGE:
    case ENOSPC:
        m_error = SystemSemaphore::OutOfResources;
        m_errorString = function + std::string("SystemSemaphore : out of resources");
        break;
    default:
        m_error = SystemSemaphore::UnknownError;
        m_errorString = function + std::string("SystemSemaphore : unknown error ") + std::string(ss.str());
    }
#endif
}

SharedMemory::SharedMemory()
    : m_memory(0)
    , m_size(0)
    , m_error(SharedMemory::NoError)
    , m_systemSemaphore(std::string())
    , m_lockedByMe(false)
    #ifdef _WIN32
    , m_handle(0)
    #else
    , m_handle(-1)
    , m_createdFile(false)
    #endif
{

}

SharedMemory::SharedMemory(const std::string &key)
    : m_memory(0)
    , m_size(0)
    , m_error(SharedMemory::NoError)
    , m_systemSemaphore(std::string())
    , m_lockedByMe(false)
    #ifdef _WIN32
    , m_handle(0)
    #else
    , m_handle(-1)
    , m_createdFile(false)
    #endif
{
    setKey(key);
}

SharedMemory::~SharedMemory()
{
    setKey(std::string());
}

void SharedMemory::setKey(const std::string &key)
{
    if (key == m_key && makePlatformSafeKey(key) == m_nativeKey) {
        return;
    }

    if (isAttached()) {
        detach();
    }

    cleanHandle();

    m_key = key;
    m_nativeKey = makePlatformSafeKey(key);
}

std::string SharedMemory::key() const
{
    return m_key;
}

std::string SharedMemory::nativeKey() const
{
    return m_nativeKey;
}

bool SharedMemory::create(int size, SharedMemory::AccessMode mode)
{
    if (!initKey()) {
        return false;
    }

    m_systemSemaphore.setKey(m_key, 1, SystemSemaphore::Create);

    const std::string function("SharedMemory::create");

    if (size <= 0) {
        m_error = SharedMemory::InvalidSize;
        m_errorString = function + std::string(" : create size is less then 0");
        return false;
    }

    if (m_nativeKey.empty()) {
        m_error = SharedMemory::KeyError;
        m_errorString = std::string("SharedMemory::create : key error");
        return false;
    }

#ifdef _WIN32
    // Create the file mapping.
    m_handle = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, size, string2wstring(m_nativeKey).c_str());
    setErrorString(function);

    // Handle is valid when it already exists unlike unix so explicitly check
    if (m_error == SharedMemory::AlreadyExists || !m_handle) {
        return false;
    }
#else
    int fd = open(m_nativeKey.c_str(), O_CREAT | O_RDWR | O_EXCL, 0600);
    if ( -1 == fd) {
        setErrorString(function);
        if (m_error != SharedMemory::AlreadyExists) {
            removeFile(m_nativeKey);
        }
        return false;
    }
    // Alloc space
    lseek(fd, size - 1, SEEK_SET);
    write(fd, "", 1);
    lseek(fd, 0, SEEK_SET);
    close(fd);
    m_createdFile = true;
#endif
    return attach(mode);
}

int SharedMemory::size() const
{
    return m_size;
}

bool SharedMemory::attach(SharedMemory::AccessMode mode)
{
    if (isAttached() || !initKey() || !handle(mode)) {
        return false;
    }

    bool isReadOnly = (mode == SharedMemory::ReadOnly);

#ifdef _WIN32
    // Grab a pointer to the memory block
    m_memory = (void *)MapViewOfFile(m_handle, isReadOnly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS, 0, 0, 0);

    if (0 == m_memory) {
        setErrorString(std::string("SharedMemory::attach"));
        cleanHandle();
        return false;
    }
    // Grab the size of the memory we have been given (a multiple of 4K on windows)
    MEMORY_BASIC_INFORMATION info;
    if (!VirtualQuery(m_memory, &info, sizeof(info))) {
        m_error = SharedMemory::UnknownError;
        m_errorString = std::string("SharedMemory::attach : size query failed");
        return false;
    }
    m_size = info.RegionSize;
#else
    m_size = getFileSize(m_nativeKey);
    m_memory = (void *)mmap(NULL, m_size, (isReadOnly ? PROT_READ : (PROT_READ | PROT_WRITE)), MAP_SHARED, m_handle, 0);
    if (MAP_FAILED == m_memory) {
        setErrorString(std::string("SharedMemory::attach"));
        cleanHandle();
        return false;
    }
#endif
    return true;
}

bool SharedMemory::isAttached() const
{
    return (0 != m_memory);
}

bool SharedMemory::detach()
{
    if (!isAttached()) {
        return false;
    }

#ifdef _WIN32
    // umap memory
    if (!UnmapViewOfFile(m_memory)) {
        setErrorString(std::string("SharedMemory::detach"));
        return false;
    }
#else
    if (munmap(m_memory, m_size) == -1) {
        setErrorString(std::string("SharedMemory::detach"));
        return false;
    }
    // close handle
    if (m_createdFile) {
        removeFile(m_nativeKey);
        m_createdFile = false;
    }
#endif
    m_memory = 0;
    m_size = 0;

    return cleanHandle();
}

void *SharedMemory::data()
{
    return m_memory;
}

const void *SharedMemory::constData() const
{
    return m_memory;
}

const void *SharedMemory::data() const
{
    return m_memory;
}

bool SharedMemory::lock()
{
    if (m_lockedByMe) {
        return true;
    }

    if (m_systemSemaphore.acquire()) {
        m_lockedByMe = true;
        return true;
    }

    m_error = SharedMemory::LockError;
    m_errorString = std::string("SharedMemory::lock : unable to lock");

    return false;
}

bool SharedMemory::tryLock(unsigned long millisecond)
{
    if (m_lockedByMe) {
        return true;
    }

    if (m_systemSemaphore.acquire(millisecond)) {
        m_lockedByMe = true;
        return true;
    }

    m_error = SharedMemory::LockError;
    m_errorString = std::string("SharedMemory::tryLock : unable to tryLock");

    return false;
}

bool SharedMemory::unlock()
{
    if (!m_lockedByMe) {
        return false;
    }

    m_lockedByMe = false;

    if (m_systemSemaphore.release()) {
        return true;
    }

    m_error = SharedMemory::LockError;
    m_errorString = std::string("SharedMemory::unlock : unable to unlock");

    return true;
}

SharedMemory::SharedMemoryError SharedMemory::error() const
{
    return m_error;
}

std::string SharedMemory::errorString() const
{
    return m_errorString;
}

bool SharedMemory::initKey()
{
    // Make sure the semaphore is correct, first clear m_key
    m_systemSemaphore.setKey(std::string(), 1);
    m_systemSemaphore.setKey(m_key, 1);

    if (m_systemSemaphore.error() != SystemSemaphore::NoError) {
        std::string function = std::string("SharedMemoryPrivate::initKey ");
        m_errorString = function + m_systemSemaphore.errorString();
        switch(m_systemSemaphore.error()) {
        case SystemSemaphore::PermissionDenied:
            m_error = SharedMemory::PermissionDenied;
            break;
        case SystemSemaphore::KeyError:
            m_error = SharedMemory::KeyError;
            break;
        case SystemSemaphore::AlreadyExists:
            m_error = SharedMemory::AlreadyExists;
            break;
        case SystemSemaphore::NotFound:
            m_error = SharedMemory::NotFound;
            break;
        case SystemSemaphore::OutOfResources:
            m_error = SharedMemory::OutOfResources;
            break;
        case SystemSemaphore::UnknownError:
        default:
            m_error = SharedMemory::UnknownError;
            break;
        }
        SharedMemory::print(std::string("initKey : " ) +  m_errorString);
        return false;
    }

    m_errorString = std::string();
    m_error = SharedMemory::NoError;

    return true;
}

std::string SharedMemory::makePlatformSafeKey(const std::string &key, const std::string &prefix)
{
    if (key.empty()) {
        return std::string();
    }

    std::string result = prefix;
    std::string part = key;
    part = std::regex_replace(part, std::regex("[^A-Za-z]"), std::string());
    result.append(part);

    std::string hex = sha1(strToUtf8(key));
    result.append(hex);

#ifdef _WIN32
    return result;
#else
    return getTempDir() + std::string("/") + result;
#endif
}

std::string SharedMemory::strToUtf8(const std::string &str)
{
#ifdef _WIN32
    int nwLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);

    wchar_t *pwBuf = new wchar_t[nwLen + 1];
    ZeroMemory(pwBuf, nwLen * 2 + 2);

    MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), pwBuf, nwLen);
    std::string retStr = wchar2string(pwBuf);

    delete[] pwBuf;
    pwBuf = NULL;

    return retStr;
#else
    return std::string(str);
#endif
}

std::string SharedMemory::sha1(const std::string& str)
{
    uint32_t digest[5];
    std::string buffer;
    uint64_t transforms;

    reset(digest, buffer, transforms);

    std::istringstream is(str);

    while (true) {
        char sbuf[BLOCK_BYTES];
        is.read(sbuf, BLOCK_BYTES - buffer.size());
        buffer.append(sbuf, (uint32_t)is.gcount());
        if (buffer.size() != BLOCK_BYTES) {
            break;
        }
        uint32_t block[BLOCK_INTS];
        bufferToBlock(buffer, block);
        transform(digest, block, transforms);
        buffer.clear();
    }

    uint64_t totalBits = (transforms*BLOCK_BYTES + buffer.size()) * 8;

    // Append
    buffer += 0x80;

    size_t orig_size = buffer.size();
    while (buffer.size() < BLOCK_BYTES) {
        buffer += (char)0x00;
    }

    uint32_t block[BLOCK_INTS];
    bufferToBlock(buffer, block);

    if (orig_size > BLOCK_BYTES - 8) {
        transform(digest, block, transforms);
        for (size_t i = 0; i < BLOCK_INTS - 2; i++) {
            block[i] = 0;
        }
    }

    // Append totalBits, split this uint64_t into two uint32_t
    block[BLOCK_INTS - 1] = (uint32_t)totalBits;
    block[BLOCK_INTS - 2] = (totalBits >> 32);
    transform(digest, block, transforms);

    // Hex std::string
    std::ostringstream result;
    for (size_t i = 0; i < sizeof(digest) / sizeof(digest[0]); i++) {
        result << std::hex << std::setfill('0') << std::setw(8);
        result << digest[i];
    }

    reset(digest, buffer, transforms);

    return result.str();
}

bool SharedMemory::removeFile(const std::string &path)
{
#ifdef _WIN32
    if (remove(path.c_str()) < 0) {
        return false;
    }
#else
    if (access(path.c_str(), F_OK) == 0) {
        if (unlink(path.c_str()) < 0) {
            return false;
        }
    }
#endif
    return true;
}


std::string SharedMemory::getAppDir()
{
    const int MAPATH = 260;
    char buffer[MAPATH];

#ifdef _WIN32
    _getcwd(buffer, MAPATH);
#else
    getcwd(buffer, MAPATH);
#endif

    return std::string(buffer);
}

std::string SharedMemory::getTempDir()
{
#ifdef _WIN32
    wchar_t buffer[MAX_PATH];
    GetEnvironmentVariable(L"TEMP", buffer, MAX_PATH);
    return std::string(wchar2string(buffer));
#else
    char *temp = getenv("TMPDIR");
    std::string tempDir(temp ? temp : "/tmp");
    int len = tempDir.length();
    return tempDir.at(len-1) == '/' ? tempDir.substr(0, len-1) : tempDir;
#endif
}

std::string SharedMemory::getHomeDir()
{
#ifdef _WIN32
    wchar_t buffer[MAX_PATH];
    GetEnvironmentVariable(L"USERPROFILE", buffer, MAX_PATH);
    return std::string(wchar2string(buffer));
#else
    char *homeDir = getenv("HOME");
    return std::string(homeDir ? homeDir : getpwuid(getuid())->pw_dir);
#endif
}

std::string SharedMemory::getDesktopDir()
{
    return getHomeDir() + std::string("/Desktop");
}

#ifdef _WIN32
HANDLE SharedMemory::handle(AccessMode mode)
{
    if (!m_handle) {
        const std::string function("SharedMemory::handle");
        if (m_nativeKey.empty()) {
            m_error = SharedMemory::KeyError;
            m_errorString = std::string("SharedMemory::handle : unable to make key");
            return 0;
        }
        m_handle = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, string2wstring(m_nativeKey).c_str());
        if (!m_handle) {
            setErrorString(function);
            return 0;
        }
    }
    return m_handle;
}
#else
int SharedMemory::handle(AccessMode mode)
{
    bool isReadOnly = (mode == SharedMemory::ReadOnly);
    m_handle = open(m_nativeKey.c_str(), (isReadOnly ? O_RDONLY : O_RDWR), (isReadOnly ? 0400 : 0600));
    if (m_handle < 0) {
        setErrorString(std::string("SharedMemory::handle"));
        return 0;
    }
    return m_handle;
}

unsigned long SharedMemory::getFileSize(const std::string &path)
{
    unsigned long fileSize = 0;
    struct stat statBuff;
    if(stat(path.c_str(), &statBuff) < 0){
        return fileSize;
    }else{
        fileSize = statBuff.st_size;
    }
    return fileSize;
}

// if prefix is empty or not be specified, the function will delete all files.
void SharedMemory::delSpecifiedFiles(const std::string &path, const std::string prefix)
{
    if (path.empty()) {
        return;
    }
    const char * dirName = path.c_str();

    struct stat s;
    lstat(dirName , &s);
    if (!S_ISDIR(s.st_mode)) {
        print(std::string("dirName is not a valid directory!"));
        return;
    }

    DIR * dir = opendir(dirName);
    if (NULL == dir) {
        print(std::string("Can not open dir ") + path);
        return;
    }

    struct dirent * filename;

    bool isDelAll = prefix.empty() ? true : false;

    while ((filename = readdir(dir)) != NULL) {
        if (strcmp( filename->d_name , "." ) == 0 ||
                strcmp( filename->d_name , "..") == 0) {
            continue;
        }
        if (isDelAll) {
            removeFile(path + std::string("/") + std::string(filename->d_name));
        } else {
            if (strncmp(filename->d_name, prefix.c_str(), prefix.size()) == 0) {
                removeFile(path + std::string("/") + std::string(filename->d_name));
            }
        }
    }
}

int SharedMemory::createUnixKeyFile(const std::string &fileName)
{
    int fd = open(fileName.c_str(), O_EXCL | O_CREAT | O_RDWR, 0640);
    if (-1 == fd) {
        if (errno == EEXIST) {
            return 0;
        } else {
            return -1;
        }
    } else {
        close(fd);
    }

    return 1;
}
#endif

void SharedMemory::print(const std::string &str)
{
    std::cout << str << std::endl;
    fflush(stdout);
}

bool SharedMemory::cleanHandle()
{
#ifdef _WIN32
    if (m_handle != 0 && !CloseHandle(m_handle)) {
        m_handle = 0;
        setErrorString(std::string("SharedMemory::cleanHandle"));
        return false;
    }
    m_handle = 0;
#else
    if (m_handle != -1 && close(m_handle) < 0 ) {
        m_handle = -1;
        setErrorString(std::string("SharedMemory::cleanHandle"));
        return false;
    }
    m_handle = -1;
#endif
    return true;
}

void SharedMemory::setErrorString(const std::string &function)
{
#ifdef _WIN32
    DWORD windowsError = GetLastError();
    if (windowsError == 0) {
        return;
    }

    std::stringstream ss;
    ss << windowsError;

    switch (windowsError) {
    case ERROR_ALREADY_EXISTS:
        m_error = SharedMemory::AlreadyExists;
        m_errorString = function + std::string(" : already exists");
        break;
    case ERROR_FILE_NOT_FOUND:
        m_error = SharedMemory::NotFound;
        m_errorString = function + std::string(" : doesn't exist");
        break;
    case ERROR_COMMITMENT_LIMIT:
        m_error = SharedMemory::InvalidSize;
        m_errorString = function + std::string(" : invalid size");
        break;
    case ERROR_NO_SYSTEM_RESOURCES:
    case ERROR_NOT_ENOUGH_MEMORY:
        m_error = SharedMemory::OutOfResources;
        m_errorString = function + std::string(" : out of resources");
        break;
    case ERROR_ACCESS_DENIED:
        m_error = SharedMemory::PermissionDenied;
        m_errorString = function + std::string(" : permission denied");
        break;
    default:
        m_error = SharedMemory::UnknownError;
        m_errorString = function + std::string(" : unknown error ") + std::string(ss.str());
    }
#else
    std::stringstream ss;
    ss << errno;

    switch (errno) {
    case EEXIST:
        m_error = SharedMemory::AlreadyExists;
        m_errorString = function + std::string(" : already exists");
        break;
    case ENOENT:
        m_error = SharedMemory::NotFound;
        m_errorString = function + std::string(" : doesn't exist");
        break;
    case EINVAL:
        m_error = SharedMemory::InvalidSize;
        m_errorString = function + std::string(" : invalid size");
        break;
    case EFAULT:
    case EMFILE:
    case ENOMEM:
        m_error = SharedMemory::OutOfResources;
        m_errorString = function + std::string(" : out of resources");
        break;
    case EACCES:
        m_error = SharedMemory::PermissionDenied;
        m_errorString = function + std::string(" : permission denied");
        break;
    default:
        m_error = SharedMemory::UnknownError;
        m_errorString = function + std::string(" : unknown error ") + std::string(ss.str());
    }
    SharedMemory::print(std::string("setErrorString : ") + m_errorString);
#endif
}

}
