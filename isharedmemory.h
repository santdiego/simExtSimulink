/**********************************************************************************************
Author : Dr. Ing. Diego Daniel Santiago.
diego.daniel.santiago@gmail.com
dsantiago@inaut.unsj.edu.ar
Facultad de Ingeniería
Universidad Nacional de San Juan
San Juan - Argentina

INAUT - Instituto de Automática
http://www.inaut.unsj.edu.ar/
CONICET - Consejo Nacional de Investigaciones Científicas y Técnicas.
http://www.conicet.gov.ar/

Version beta 1.2
************************************************************************************************/


#ifndef ISHAREDMEMORY_H
#define ISHAREDMEMORY_H

#pragma once
#ifdef UNICODE
#undef UNICODE
#endif



//ACIVAR BANDERA PARA DEPURAR
//#define DEBUG_SHMCLASS

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <sys/types.h>
#include <process.h>
#define getpid() _getpid()
typedef HANDLE SEM_TYPE ;
#endif /* _WIN32 */

#if defined(__linux) || defined(__APPLE__)


#include <stdio.h>
/* exit() etc */
#include <unistd.h>
/* shm_* stuff, and mmap() */
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include<semaphore.h>
/* Posix IPC object name [system dependant] - see
http://mij.oltrelinux.com/devel/unixprg/index2.html#ipc__posix_objects */

/* how many types of messages we recognize (fantasy) */
#define TYPES 8
typedef sem_t SEM_TYPE ;
typedef unsigned long       DWORD;
#endif /* __linux || __APPLE__ */


#include <string>
#include <string.h>
#include <vector>
#include <iostream>
#include "itypes.h"
#undef min
#undef max
#include <algorithm>
#define MAXLENGTH size_t(40)
#define MINLENGTH 3
#define MAXISHM 400
#define MAXPROCESSESWAITING 10
#define MAXPROCESSATTACHED 40
#define GLOBALTABLENAME "IShmTable"
#define MAX_SEM_COUNT 10


namespace ilib {

enum SharedMemoryError:uint8_t { NoError, PermissionDenied, InvalidSize, KeyError, AlreadyExists, NotFound,LockError,OutOfResources,UnknownError,TableError };
enum ShmState:uint8_t  { BUSY, FREE, NON_SYNC };

struct IShTable{ //Tabla en memoria compartida que se crea cada vez que una ISharedMemory es instanciada
    char name[MAXISHM][MAXLENGTH];
    int size[MAXISHM];
    int pid[MAXISHM][MAXPROCESSATTACHED]; //process attached
    int lock[MAXISHM][MAXPROCESSESWAITING];// Semaforo
    int  num; //Cantidad de memorias compartidas de tipo ISharedMemory
};

class BaseSharedMemory{
public:
    BaseSharedMemory    ();
    ~BaseSharedMemory   ();
    SharedMemoryError   open                (const char *shmName, size_t size, AccessMode mode); //Equaly for attach or create
    SharedMemoryError   close               ();
    void *              data                ();
protected:
    void                *pData;
    char                _name[MAXLENGTH];
    size_t              _size;
    bool                _isattached;
#ifdef _WIN32
    HANDLE              hShare;  // system Handle of share memory
#endif            /* _WIN32 */
    int                 shmfd;
};

class ISharedMemory :public BaseSharedMemory {
public:
    ISharedMemory       ();
    ISharedMemory       (const char * name, int pid=-1);
    ~ISharedMemory      ();
    bool                attach              (AccessMode mode = readWrite);
    const void *        constData           () const;
    bool                create              (size_t size, AccessMode mode = readWrite);
    bool                reload              (size_t size, AccessMode mode = readWrite);
    void                *data               ();

    //    template <class T>
    //    size_t              loadData            (const T *ptr_source, size_t size);
    //    template <class T>
    //    size_t              asingData2          (T *ptr_dest, size_t size);


    template <class D=char,class H=char>
    /**
     * @brief setData Copy the data into memory and optionally write a data header to the beginning of the memory region.
     * @param pSource pointer to the data source without including the header. It will be copied into memory after the header.
     * @param source_Dsize It is the size of the data expressed in its original type. The size in bytes is computed internally from the type.
     * @param pHead Optional. Pointer to the source of the data header. Left empty if not using.
     * @param head_Dsize Optional. It is the size of the header expressed in its original type. If this field is non-zero, it represents the offset (measured in data type H) in memory from which the data will be copied. The size in bytes is computed internally from the type. Left empty if not using.
     * @return returns returns the total amount of data written to memory.
     */
    size_t              setData(const D *pSourceBody, size_t sourceBodyDsize,const H *pHead=nullptr, size_t head_Dsize=0);

    template <class D=char,class H=char>
    /**
     * @brief getData Retrieves the data contained in memory and optionally divides the memory data between header and body.
     * @param pDestination pointer to which the memory data will be copied without including the header. Make sure the pointer has enough allocated memory.
     * @param destBodyDsize size of the destination, without the header, expressed in its data type. The size in bytes is computed internally from the type.
     * @param pDestHeads Pointer to where the memory headers will be copied. Make sure the pointer has enough memory allocated.
     * @param head_Dsize size of each header expressed in its data type. If this field is non-zero, it represents the offset (measured in data type H), from which pDestinationBody is read. The size in bytes is computed internally from the type.
     * @return returns the total amount of data readed from memory.
     */
    size_t              getData(D* pDestinationBody,size_t destBodyDsize,H *pDestHeads=nullptr, size_t head_Dsize=0);

    bool                detach              ();
    SharedMemoryError   error               () const;
    std::string         errorString         () const;
    bool                isAttached          () const;
    const char          *key                () const;

    void                setKey              (const char *key);
    size_t              totalSize           () const;
    size_t              size                () const; //returns the size of the writable memory (total size - offset)
    void                setSize             (size_t);
    bool                lock                ();
    bool                unlock              ();
    bool                setOffset           (unsigned offset);
    size_t              getOffset           ();
private:
    bool                loadSemaphore       ();
    bool                loadTable           ();
    int                 findISharedMemory   (const char*);
    IShTable*           ptrShmTable;
    BaseSharedMemory    shmTable;
    SharedMemoryError   _error;
    int                 _pid;
    int                 tableIndex;
    size_t              memOffset; //le indica a la memoria compartida a partir de cual byte tiene que leer/escribir
    SEM_TYPE            ghSemaphore;
};
static  int          _instance=0;
class ISharedMemoryContainer
{
public:
    ISharedMemoryContainer(){}
    void                        closeAll            ();
    void                        removeAll           ();
    ISharedMemory*              at                  (size_t idx);
    void                        insert              (ISharedMemory* theShareMemory);
    void                        removeFromName      (const char *theName);
    ISharedMemory*              getFromName         (const char *theName);
    size_t                      getCount            ();
    char*                       getList             ();
    std::vector<ISharedMemory*> getAll()            {return _allShareMemorys;}
protected:
    std::vector<ISharedMemory*> _allShareMemorys;
    int                         getNewID            ();
};

inline BaseSharedMemory::BaseSharedMemory(){
    //std::cout<<"BaseSharedMemory::BaseSharedMemory"<<std::endl;
    pData=nullptr;
    //hShare=nullptr;
    _size=0;
    _isattached=false;

}

inline BaseSharedMemory::~BaseSharedMemory(){
#ifdef DEBUG_SHMCLASS
    std::cout<<"~BaseSharedMemory() name:"<<_name<<std::endl;
# endif
    if(_isattached)
        close();
}

inline SharedMemoryError BaseSharedMemory::close(){

    //std::cout<<"close()"<<_name<<std::endl;
    SharedMemoryError ret =NoError;
    if (!_isattached){
        //std::cout<<"ISharedMemory::detach() !_is_attached: NAME:"<<_name<<std::endl;
        return NotFound;
    }
#ifdef _WIN32
    try{
        if((pData != nullptr)) {
            if( FlushViewOfFile(pData, _size) == 0){
                ret= UnknownError;
            }
            if( UnmapViewOfFile(pData) == 0){
                ret= UnknownError;
            }
            _isattached=false;
            pData=nullptr;
        }
        //        if(hShare!=nullptr)
        //            if( CloseHandle(hShare) == 0)
        //                ret= UnknownError;
        hShare=nullptr;
    }
    catch (std::exception& e){
        std::cout << e.what() << '\n';
    }
#endif
#if defined (__linux) || defined (__APPLE__)
    if (shm_unlink(_name) != 0) {
        perror("In shm_unlink()");
        return UnknownError;
    }
#endif /* __linux || __APPLE__ */

    _isattached=false;
    return ret;
}

inline SharedMemoryError BaseSharedMemory::open(const char *shmName, size_t shmSize, AccessMode mode = readWrite){
    if (strlen(shmName)<MINLENGTH){
        return KeyError;
    }

    if(shmSize<=0){
        return InvalidSize;
    }
    SharedMemoryError ret;
#ifdef _WIN32
    DWORD access;

    switch(mode){
    case read:
        access =PAGE_READONLY;
        break;
    case write:
        access =PAGE_READWRITE;
        break;
    default:
        access =PAGE_READWRITE;
    }

    hShare = CreateFileMappingA(INVALID_HANDLE_VALUE,nullptr, access, 0,DWORD(shmSize), shmName);
    if(hShare == nullptr){
        return PermissionDenied;
    }else{
        pData = MapViewOfFile(hShare,FILE_MAP_ALL_ACCESS,0, 0, shmSize);
    }
    if(pData == nullptr){
        return UnknownError;
    }
    if(GetLastError()==ERROR_ALREADY_EXISTS){
        ret= AlreadyExists;
    }
    else{
        memset(pData,0,shmSize);
        ret=NoError;
    }
#endif /* _WIN32 */

#if defined (__linux) || defined (__APPLE__)
    /* creating the shared memory object    --  shm_open()  */
    shmfd = shm_open(shmName, O_CREAT|O_RDWR, S_IRWXU | S_IRWXG);
    if (shmfd < 0) {
        perror("In shm_open()");
        return UnknownError;
    }
    //printf(stderr, "Created shared memory object %s\n", shmName);

    /* adjusting mapped file size (make room for the whole segment to map)      --  ftruncate() */
    ftruncate(shmfd, shmSize);
    /* requesting the shared segment    --  mmap() */
    pData = mmap(nullptr, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (pData == nullptr) {
        perror("In mmap()");
        return UnknownError;
    }
    //printf(stderr, "Shared memory segment allocated correctly (%d bytes).\n", _size);
#endif /* __linux || __APPLE__ */
    //ALL OK
    //strncpy(_name,shmName,std::min(size_t(MAXLENGTH),strlen(shmName)));
    strcpy_s(_name,shmName);
    _size=shmSize;
    _isattached=true;
    return ret;
}

inline void* BaseSharedMemory::data() {
    if (!_isattached){
        //_error=NotFound;
        return nullptr;
    }
    return pData;
}
/////////////////////////ISharedMemory/////////////////////////////////////////
//inline int ISharedMemory::_instance = 0;

inline ISharedMemory::ISharedMemory() :BaseSharedMemory(){
    //ghSemaphore=nullptr;
    _pid=getpid();
    _instance++;
    if(!loadTable())
        exit(1);
    memOffset=0;
    _size=0;
}

inline ISharedMemory::ISharedMemory(const char* name, int pid) {
    //ghSemaphore=nullptr;
    //std::cout<<"ISharedMemory"<<std::endl;
    if (pid == -1)
        _pid=getpid();
    else
        _pid=pid;
    _instance++;
    setKey(name);
    if(!loadTable())
        std::cerr<<"Could Not load Table"<<std::endl;
    memOffset=0;
    _size=0;
}

inline bool ISharedMemory::loadTable(){
#ifdef DEBUG_SHMCLASS
    std::cout<<"loadTable"<<std::endl;
#endif
    _error=shmTable.open(GLOBALTABLENAME,sizeof(IShTable),readWrite);
    switch(_error){
    case AlreadyExists:
        break;
    case NoError:
        break;
    default:
        std::cerr<<"Error "<< _error<<", loading ITable "<<std::endl;
        return false;
    }
    ptrShmTable=static_cast<IShTable*>(shmTable.data());
    if(ptrShmTable==nullptr){
        std::cout<<_name<<std::endl;
        return false;
    }
    if(findISharedMemory(GLOBALTABLENAME)!=0){ //No hay ISharedMemory Cargados
        strcpy_s(ptrShmTable->name[0],GLOBALTABLENAME);
        // strncpy(ptrShmTable->name[0],GLOBALTABLENAME,strlen(GLOBALTABLENAME));
        ptrShmTable->size[0]=sizeof(IShTable);
        ptrShmTable->pid[0][0]=_pid;
        ptrShmTable->num=1;
        for (size_t i=1;i<MAXISHM;i++){ //clean table
            for(size_t j=0;j<MAXPROCESSATTACHED;j++)
                ptrShmTable->pid[i][j]=-1;
            for(size_t j=0;j<MAXPROCESSESWAITING;j++)
                ptrShmTable->lock[i][j]=-1;
            ptrShmTable->size[i]=-1;
        }
    }
    return true;
}

inline void ISharedMemory::setSize(size_t size){
    _size=size;
}

inline ISharedMemory::~ISharedMemory() {
#ifdef DEBUG_SHMCLASS
    std::cout<<"ISharedMemory::~ISharedMemory()"<<std::endl;
#endif
    if(isAttached()){
        detach();
    }
    _instance--;
}

inline int ISharedMemory::findISharedMemory(const char *lookFor){
    //std::cout<<"findISharedMemory"<<std::endl;
    if(ptrShmTable==nullptr)
        return -1;
    for (int i=0;i<MAXISHM;i++){
        if(strcmp(ptrShmTable->name[i],lookFor)==0){
            return i;
        }
    }
    return -1;
}

inline bool ISharedMemory::create(size_t size, AccessMode mode) {
#ifdef DEBUG_SHMCLASS
    std::cout<<"create"<<std::endl;
#endif
    bool ret=false;
    if(ptrShmTable==nullptr){
        _error=TableError;
        return false;
    }
    if(findISharedMemory(_name)>-1){
#ifdef DEBUG_SHMCLASS
        std::cout<<"Memory Already Exists, trying to Attach  "<<std::endl;
#endif
        _error=AlreadyExists;
        return attach(mode);
    }
    _isattached=false;
    if (size<=0){
        _error= InvalidSize;
        return false;
    }
    //ATENCION USANDO LA LIBRERIA SIEMPRE SE CREA LA SHM COMO NO NATIVA
    _error=open(_name,size,mode);
    switch(_error){
    case NoError:
    { //Actualizar tabla
#ifdef DEBUG_SHMCLASS
        std::cout<<"No Error: update Table"<<std::endl;
#endif
        _isattached=true;
        int i =1;
        for(i=1;i<MAXISHM;i++)
            if(ptrShmTable->size[i]<0)
                break;
        tableIndex=i;
        //strncpy(ptrShmTable->name[i],_name,std::min(size_t(MAXLENGTH),strlen(_name)));
        strcpy_s(ptrShmTable->name[i],_name);
        ptrShmTable->size[i]=int(size);
        ptrShmTable->pid[i][0]=_pid;
        for(size_t j=0;j<MAXPROCESSESWAITING;j++)
            ptrShmTable->lock[i][j]=-1;
        ptrShmTable->num++;
        ret= true;
        break;
    }
    case AlreadyExists:
    { //La memoria no es del tipo ISharedMemory pero fue creada : La agrego a la tabla

        _isattached=true;
        int i =1;
        for(i=1;i<MAXISHM;i++)
            if(ptrShmTable->size[i]<0)
                break;
        tableIndex=i;
        //strncpy(ptrShmTable->name[i],_name,std::min(strlen(_name),MAXLENGTH));
        ptrShmTable->size[i]=int(size);
        int j=0;
        for(j=0;j<MAXPROCESSATTACHED;j++)
            if(ptrShmTable->pid[tableIndex][j]==_pid)
                break;
        ptrShmTable->pid[i][j]=_pid;
        for(j=0;j<MAXPROCESSESWAITING;j++)
            ptrShmTable->lock[i][j]=-1;
        ptrShmTable->num++;
#ifdef DEBUG_SHMCLASS
        std::cout<<"The memory is not of the ISharedMemory type but it was created: add it to the table index:" <<tableIndex <<std::endl;
#endif
        ret= true;
        break;
    }
    default:
    {
        return false;
    }
    }

    if(!loadSemaphore())
        ret=false;
    return ret;
}

inline bool ISharedMemory::reload(size_t _size, AccessMode mode){
    close();
    if(!create(_size,mode))
        return false;
    if(size()!=_size)
        return false;
    return true;
}

inline bool ISharedMemory::attach(AccessMode mode) {
    //std::cout<<"attach"<<std::endl;
    _isattached=false;
    if(strlen(_name)<MINLENGTH){
        //std::cerr<<"Shared Memory key not valid"<<std::endl;
        _error=KeyError;
        return false;
    }
    tableIndex=findISharedMemory(_name);
    if(tableIndex>0){
        if(ptrShmTable->size[tableIndex]<0){
            //std::cerr<<"Invalid size"<<std::endl;
            _error= InvalidSize;
            return false;
        }
        else{
            _size=unsigned(ptrShmTable->size[tableIndex]);
            _error=open(_name,_size,mode);
            switch(_error){
            case NoError :
                _isattached=true;
                loadSemaphore();
                return true;
            case AlreadyExists:
                _isattached=true;
                loadSemaphore();
                _error=NoError;
                return true;
            default:
                return false;
            }
        }
    }
    else{
        //std::cerr<<"Memory not found"<<std::endl;
        _error= NotFound;
        return false;
    }
}

inline bool ISharedMemory::detach() {
    bool ret=true;
#ifdef DEBUG_SHMCLASS
    std::cout<<"ISharedMemory::detach() Index: "<<tableIndex<<"name: "<<ptrShmTable->name[tableIndex]<<std::endl;
#endif
    if(!_isattached)
        ret=false;
#ifdef _WIN32
    if(ghSemaphore!=nullptr){
        CloseHandle(ghSemaphore);
    }
    if(hShare!=nullptr){
        CloseHandle(hShare);
    }
#endif
# ifndef _WIN32
    //sem_close(shmfd);
    sem_destroy(&ghSemaphore);
    shm_unlink(_name);
#endif
    bool last=true;
    for(size_t j=0;j<MAXPROCESSATTACHED;j++){
        if(ptrShmTable->pid[tableIndex][j]==_pid)
            ptrShmTable->pid[tableIndex][j]=-1;
        if(ptrShmTable->pid[tableIndex][j]>0){
            last=false;
            break;
        }
    }
    if(last){
#ifdef DEBUG_SHMCLASS
        std::cout<<"Erase Table: Index: "<<tableIndex<<"   name:"<<ptrShmTable->name[tableIndex]<<std::endl<<std::endl<<std::endl;
#endif
        //strncpy(ptrShmTable->name[tableIndex],"none",5);
        strcpy_s(ptrShmTable->name[tableIndex],"none");

        ptrShmTable->size[tableIndex]=-1;
        for (size_t j=0;j<MAXPROCESSATTACHED;j++)
            ptrShmTable->pid[tableIndex][j]=-1;
        for(size_t j=0;j<MAXPROCESSESWAITING;j++)
            ptrShmTable->lock[tableIndex][j]=-1;
        ptrShmTable->num--;
    }

    close();
    return ret;
}

inline const void* ISharedMemory::constData() const {
    if (!_isattached){
        //_error=NotFound;
        return nullptr;
    }
    return pData;
}

inline void* ISharedMemory::data() {
    if (!_isattached){
        _error=NotFound;
        return nullptr;
    }
    return (static_cast<char*>(pData)+memOffset);
}

/*
template<class T>
inline size_t ISharedMemory::loadData(const T* ptr_source, size_t dat_size){
    const void *ptr=reinterpret_cast<const void*>(ptr_source);
    size_t sz = std::min(size(),dat_size*sizeof (T));
    if(lock()){
        memcpy(data(),ptr,sz);
        if(unlock())
            return sz;
    }
    return 0;
}

template <class T>
inline size_t ISharedMemory::asingData2(T *ptr_dest, size_t dat_size){
    size_t sz=std::min(size(),dat_size*sizeof (T));
    if(lock()){
        memcpy(reinterpret_cast<void*>(ptr_dest),data(),sz);
        if(unlock()){
            return sz;
        }
    }
    return 0;
}
*/
template <class D,class H>
inline size_t ISharedMemory::setData(const D *pSourceBody, size_t sourceBodyDsize,const H *pHead, size_t head_Dsize){
    size_t written=0;
    size_t offset=head_Dsize*sizeof(H);
    size_t size_in_bytes =sourceBodyDsize*sizeof (D);

    if(lock()){
        //Write header first
        if(pHead){
            size_t sz=std::min(size(),offset);
            memcpy(data(),pHead,sz);
            written+=sz;
        }
        //next write body
        if(pSourceBody && size()>offset){
            size_t sz=std::min(size()-offset,size_in_bytes);
            memcpy(reinterpret_cast<char*>(data())+offset,pSourceBody,sz);
            written+=sz;
        }
        if(unlock()){
            return written;
        }
    }

    return 0;
}

template <class D,class H>
inline size_t ISharedMemory::getData(D* pDestinationBody,size_t destBodyDsize,H *pDestHeads, size_t head_Dsize)
{
    size_t read=0;
    size_t offset=head_Dsize*sizeof(H);
    size_t size_in_bytes =destBodyDsize*sizeof (D);
    if(lock()){
        //Read header first
        if(pDestHeads){
            size_t sz=std::min(size(),offset);
            memcpy(pDestHeads,data(),sz);
            read+=sz;
        }
        //next write body
        if(pDestinationBody && size()>offset){
            size_t sz=std::min(size()-offset,size_in_bytes);
            memcpy(pDestinationBody,reinterpret_cast<char*>(data())+offset,sz);
            read+=sz;
        }
        if(unlock())
            return read;
    }
    return 0;
}

inline bool ISharedMemory::setOffset(unsigned offset){
    if(offset< _size){
        memOffset=offset;
        return true;
    }
    else
        return false;
}

inline size_t ISharedMemory::getOffset(){
    return memOffset;
}

inline SharedMemoryError ISharedMemory::error() const {
    return _error;
}

inline std::string ISharedMemory::errorString() const {
    std::string the_error;
    switch(_error){
    case NoError:
        the_error= std::string("NoError : No error occurred.");
        break;
    case PermissionDenied:
        the_error=std::string("PermissionDenied: The operation failed because the caller didn't have the required permissions.");
        break;
    case InvalidSize:
        the_error=std::string("InvalidSize: A create operation failed because the requested size was invalid.");
        break;
    case KeyError:
        the_error=std::string("KeyError: The operation failed because of an invalid key.");
        break;
    case AlreadyExists:
        the_error=std::string("AlreadyExists: A create() operation failed because a shared memory segment with the specified key already existed.");
        break;
    case NotFound:
        the_error=std::string("NotFound: An attach() failed because a shared memory segment with the specified key could not be found.");
        break;
    case LockError:
        the_error=std::string("LockError:The attempt to lock() the shared memory segment failed because create() or attach() failed and returned false.");
        break;
    case OutOfResources:
        the_error=std::string("OutOfResources: A create() operation failed because there was not enough memory available to fill the request");
        break;
    case UnknownError:
        the_error=std::string("UnknownError: Something else happened and it was bad.");
        break;
    case TableError:
        the_error=std::string("ShmTable could not be found");
        break;
    }
    return the_error;
}

inline bool ISharedMemory::isAttached() const {
    return _isattached;
}

inline bool ISharedMemory::lock() {
    //std::cout<<"lock: "<<_name<<std::endl;
    if (!_isattached){
        _error=NotFound;
        return false;
    }

#if defined(__linux) || defined(__APPLE__)
    timespec time;
    time.tv_sec=1;
    if(sem_timedwait(&ghSemaphore,&time)==ETIMEDOUT){
        std::cerr<<"ISharedMemory::lock : wait timed out"<<std::endl;
        return false;
    }
#endif /* __linux || __APPLE__ */

#ifdef _WIN32

    DWORD               dwWaitResult;
    dwWaitResult = WaitForSingleObject(ghSemaphore,15L);
    switch (dwWaitResult){

    case WAIT_OBJECT_0: // The semaphore object was signaled.
        return true;
    case WAIT_TIMEOUT:
        std::cerr<<"ISharedMemory::lock : wait timed out. Name:"<<_name<<std::endl;// The semaphore was nonsignaled, so a time-out occurred.
        _error=LockError;
        return false;
    }
    return false;
#endif
    /* _WIN32 */
    return true;
}

inline bool ISharedMemory::unlock() {
    //std::cout<<"unlock: "<<_name<<std::endl;
    if (!_isattached){
        _error=NotFound;
        return false;
    }
#if defined(__linux) || defined(__APPLE__)
    if(sem_post(&ghSemaphore)!=0){

        return false;
    }
#endif /* __linux || __APPLE__ */
    // Release the semaphore when task is finished
#ifdef _WIN32
    if (ghSemaphore!=nullptr){
        if (!ReleaseSemaphore(ghSemaphore,1,nullptr))
        {
            std::cerr<<"ReleaseSemaphore :"<<_name<<". Error: "<<GetLastError()<<std::endl;
            _error=LockError;
            loadSemaphore(); //try to reload semaphore
            return false;
        }
    }
#endif
    return true;
}

inline const char* ISharedMemory::key() const {
    return _name;
}

inline void ISharedMemory::setKey(const char* key) {
    //strncpy(_name,key,std::min(MAXLENGTH,strlen(key)));
    strcpy_s(_name,key);
}

inline size_t ISharedMemory::totalSize() const{
    if (!_isattached){
        //_error=NotFound;
        return 0;
    }
    return _size;
}

inline size_t ISharedMemory::size() const {
    if (!_isattached){
        //_error=NotFound;
        return 0;
    }
    return (_size-memOffset);
}

inline bool ISharedMemory::loadSemaphore(){
    std::string semName(_name);
    semName+="_sem";
#ifdef _WIN32
    if(ghSemaphore!=nullptr)
        CloseHandle(ghSemaphore);
    ghSemaphore = CreateSemaphoreA(
                nullptr,           // default security attributes
                MAX_SEM_COUNT,  // initial count
                MAX_SEM_COUNT,  // maximum count
                semName.c_str());          // unnamed semaphore
    if(ghSemaphore == nullptr){
        _error=UnknownError;
        return false;
    }
#endif
#ifndef _WIN32
    sem_init(&ghSemaphore, 1, 1);
#endif
    return true;
}

inline void ISharedMemoryContainer::closeAll()    {
    for (size_t i=0;i<getCount();i++){
        if(_allShareMemorys[i]->detach()==false){
            printf("ERROR in 'ISharedMemoryContainer::closeAll()': THE SHARE MEMORY WASN'T CLOSE \n");
        }
    }
}

inline void ISharedMemoryContainer::removeAll()    {
    closeAll();
    for (size_t i=0;i<getCount();i++){
        _allShareMemorys[i]->close();
        delete _allShareMemorys[i];
    }
    _allShareMemorys.clear();

}

inline ISharedMemory* ISharedMemoryContainer::at(size_t idx)    {
    if(idx<getCount()){
        return(_allShareMemorys[idx]);
    }
    return nullptr;
}

inline void ISharedMemoryContainer::insert(ISharedMemory* theShareMemory)    {
    _allShareMemorys.push_back(theShareMemory);
}

inline void ISharedMemoryContainer::removeFromName(const char *theName)    {

    for (size_t i=0;i<getCount();i++)
    {
        if(strcmp(_allShareMemorys[i]->key(),theName)==0)
        {
            //                if(offset>=0){
            //ssPrintf("_allShareMemorys[i]->getOffset: %i\n",_allShareMemorys[i]->getOffset());
            //ssPrintf("offset %i\n",offset);

            if(_allShareMemorys[i]->detach()==false)
                printf("ERROR in 'ISharedMemoryContainer::removeFromID': THE SHARE MEMORY WASN'T CLOSE \n");
            _allShareMemorys[i]->close();
            delete _allShareMemorys[i];
            _allShareMemorys.erase(_allShareMemorys.begin()+i);
            //                }
            //                else{
            //                    if(_allShareMemorys[i]->getOffset()==offset){
            //                        if(_allShareMemorys[i]->detach()==false)
            //                            printf("ERROR in 'ISharedMemoryContainer::removeFromID': THE SHARE MEMORY WASN'T CLOSE \n");
            //                        _allShareMemorys[i]->close();
            //                        delete _allShareMemorys[i];
            //                        _allShareMemorys.erase(_allShareMemorys.begin()+i);
            //                    }
            //                }
            break;
        }
    }
}

inline ISharedMemory* ISharedMemoryContainer::getFromName(const char *theName)    {
    for (size_t i=0;i<getCount();i++)
    {
        if(strcmp(_allShareMemorys[i]->key(),theName)==0){
            return(_allShareMemorys[i]);
        }
    }
    return nullptr;
}

inline size_t ISharedMemoryContainer::getCount()    {
    return(_allShareMemorys.size());
}

}
#endif  // ISHAREDMEMORY_H

