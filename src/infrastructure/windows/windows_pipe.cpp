// @spec SPEC-TL-BRIDGE
#include "pipe_security.hpp"
#include <tela/windows_pipe.hpp>
#include <array>
#include <mutex>
#include <thread>
#include <stdexcept>

namespace tela {
namespace {
struct Handle {
    HANDLE value{};
    ~Handle(){if(value && value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
    Handle()=default; explicit Handle(HANDLE h):value(h){}
    Handle(const Handle&)=delete;
};
}
struct WindowsPipe::Impl {
    windows::PipeSecurity security;
    Handle stop{CreateEventW(nullptr,TRUE,FALSE,nullptr)}, wake{CreateEventW(nullptr,TRUE,FALSE,nullptr)};
    Handle operation{CreateEventW(nullptr,TRUE,FALSE,nullptr)};
    Handle pipe;
    std::mutex mutex;
    std::vector<PipeEvent> queue;
    std::thread worker;
    explicit Impl(const std::string& name) {
        if(name.empty() || name.size()>100 || name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.")!=std::string::npos)
            throw std::invalid_argument("Invalid Tela pipe name");
        if(!stop.value||!wake.value||!operation.value)throw std::runtime_error("Cannot create pipe events");
        const std::wstring path=L"\\\\.\\pipe\\"+std::wstring(name.begin(),name.end());
        pipe.value=CreateNamedPipeW(path.c_str(),PIPE_ACCESS_INBOUND|FILE_FLAG_OVERLAPPED|FILE_FLAG_FIRST_PIPE_INSTANCE,
            PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT|PIPE_REJECT_REMOTE_CLIENTS,1,65536,65536,0,security.attributes());
        if(pipe.value==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot create authenticated Tela pipe: "+std::to_string(GetLastError()));
        worker=std::thread([this]{run();});
    }
    ~Impl(){SetEvent(stop.value);CancelIoEx(pipe.value,nullptr);if(worker.joinable())worker.join();}
    bool wait(OVERLAPPED& ov,DWORD& bytes,DWORD timeout) {
        HANDLE handles[]{stop.value,operation.value};
        auto result=WaitForMultipleObjects(2,handles,FALSE,timeout);
        if(result!=WAIT_OBJECT_0+1){CancelIoEx(pipe.value,&ov);GetOverlappedResult(pipe.value,&ov,&bytes,TRUE);return false;}
        return GetOverlappedResult(pipe.value,&ov,&bytes,FALSE)!=FALSE;
    }
    bool read(void* data,DWORD length) {
        auto* dst=static_cast<unsigned char*>(data);DWORD offset=0;
        while(offset<length){
            if(WaitForSingleObject(stop.value,0)==WAIT_OBJECT_0)return false;
            OVERLAPPED ov{};ov.hEvent=operation.value;ResetEvent(operation.value);DWORD bytes{};
            if(!ReadFile(pipe.value,dst+offset,length-offset,&bytes,&ov)){
                if(GetLastError()!=ERROR_IO_PENDING || !wait(ov,bytes,3000))return false;
            }
            if(!bytes)return false;offset+=bytes;
        }
        return true;
    }
    bool push(PipeEvent event) {
        std::lock_guard lock(mutex);
        if(queue.size()>=256)return false;
        queue.push_back(std::move(event));SetEvent(wake.value);return true;
    }
    void disconnected(std::uint64_t generation,const std::string& error) {
        std::lock_guard lock(mutex);
        // Discard the old connection atomically; never expose an up after a dropped down.
        queue.clear();queue.push_back({generation,0,std::nullopt,error});SetEvent(wake.value);
    }
    void run() noexcept {
        std::uint64_t connection=0;
        while(WaitForSingleObject(stop.value,0)!=WAIT_OBJECT_0){
            std::string error;
            try {
                OVERLAPPED ov{};ov.hEvent=operation.value;ResetEvent(operation.value);DWORD bytes{};
                bool connected=ConnectNamedPipe(pipe.value,&ov)!=FALSE;
                if(!connected){auto code=GetLastError();connected=code==ERROR_PIPE_CONNECTED || (code==ERROR_IO_PENDING && wait(ov,bytes,INFINITE));}
                if(!connected)break;
                ++connection;ULONG pid{};
                if(!GetNamedPipeClientProcessId(pipe.value,&pid))throw std::runtime_error("Cannot authenticate pipe client process");
                while(true){
                    std::array<unsigned char,4> prefix{};if(!read(prefix.data(),4))break;
                    const std::uint32_t length=prefix[0]|(prefix[1]<<8)|(prefix[2]<<16)|(static_cast<std::uint32_t>(prefix[3])<<24);
                    if(length<36 || length>bridge_max_frame)throw std::runtime_error("Invalid pipe frame length");
                    std::vector<unsigned char> payload(length);if(!read(payload.data(),length))break;
                    auto m=decode_bridge(payload);
                    if(m.kind==BridgeKind::hello){DWORD owner{};GetWindowThreadProcessId(reinterpret_cast<HWND>(static_cast<std::uintptr_t>(m.host_window)),&owner);if(owner!=pid)throw std::runtime_error("Pipe client does not own host HWND");}
                    if(!push({connection,pid,std::move(m),{}}))throw std::runtime_error("Tela pipe delivery queue overflow");
                }
            } catch(const std::exception& e){error=e.what();}
            disconnected(connection,error);DisconnectNamedPipe(pipe.value);
        }
    }
};
WindowsPipe::WindowsPipe(const std::string& name):impl_(std::make_unique<Impl>(name)){}
WindowsPipe::~WindowsPipe()=default;
std::vector<PipeEvent> WindowsPipe::drain(){std::lock_guard lock(impl_->mutex);auto result=std::move(impl_->queue);impl_->queue.clear();ResetEvent(impl_->wake.value);return result;}
std::uintptr_t WindowsPipe::wake_handle() const noexcept{return reinterpret_cast<std::uintptr_t>(impl_->wake.value);}
}
