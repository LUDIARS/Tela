// @spec SPEC-TL-BRIDGE
#include "pipe_security.hpp"
#include <sddl.h>
#include <string>
#include <stdexcept>

namespace tela::windows {
PipeSecurity::PipeSecurity() {
    HANDLE token{};
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token))throw std::runtime_error("Cannot query current user");
    DWORD size{};GetTokenInformation(token,TokenUser,nullptr,0,&size);
    std::vector<unsigned char> info(size);
    const bool ok=GetTokenInformation(token,TokenUser,info.data(),size,&size)!=FALSE;
    CloseHandle(token);
    if(!ok)throw std::runtime_error("Cannot read current user SID");
    LPWSTR sid{};
    if(!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(info.data())->User.Sid,&sid))throw std::runtime_error("Cannot format current user SID");
    const std::wstring acl=L"D:P(A;;GA;;;"+std::wstring(sid)+L")";LocalFree(sid);
    if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(acl.c_str(),SDDL_REVISION_1,&descriptor_,nullptr))throw std::runtime_error("Cannot create current-user pipe DACL");
    attributes_.lpSecurityDescriptor=descriptor_;
}
PipeSecurity::~PipeSecurity(){if(descriptor_)LocalFree(descriptor_);}
}
