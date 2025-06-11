#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#include <iostream>

struct AutoElevate {
    AutoElevate(int argc,char**argv){
        BOOL admin = FALSE;
        SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
        PSID AdministratorsGroup;
        if(AllocateAndInitializeSid(&NtAuthority,2,SECURITY_BUILTIN_DOMAIN_RID,DOMAIN_ALIAS_RID_ADMINS,0,0,0,0,0,0,&AdministratorsGroup)){
            CheckTokenMembership(NULL,AdministratorsGroup,&admin);
            FreeSid(AdministratorsGroup);
        }
        if(!admin){
            std::wstring cmd(GetCommandLineW());
            SHELLEXECUTEINFOW sei={sizeof(sei)};
            sei.lpVerb=L"runas";
            sei.lpFile=argv[0];
            sei.lpParameters=GetCommandLineW();
            sei.nShow=SW_SHOWNORMAL;
            if(!ShellExecuteExW(&sei)){
                std::cerr<<"Failed to elevate"<<std::endl;exit(1);
            }
            exit(0);
        }
    }
};
static AutoElevate elevate_guard(__argc,__argv);
#endif
