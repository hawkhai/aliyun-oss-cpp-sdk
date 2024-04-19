#include <alibabacloud/oss/OssClient.h>
#undef WIN32_LEAN_AND_MEAN
#include <..\..\sdk\src\utils\FileSystemUtils.h>
using namespace AlibabaCloud::OSS;

#include "..\..\..\build\infoss.h"
#include <assert.h>
#include <iostream>
#include <regex>
#include <string>
#include <Windows.h>
#include "cppx\funclib.h"
#include "cppx\KMD5.h"
#include "Everything.h"

#undef GetObject
#undef CreateDirectory

// 添加依赖库
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "wldap32.lib")

std::string& replace_str(std::string& str, const std::string& to_replaced, const std::string& newchars)
{
    for (std::string::size_type pos(0); pos != std::string::npos; pos += newchars.length())
    {
        pos = str.find(to_replaced, pos);
        if (pos != std::string::npos)
            str.replace(pos, to_replaced.length(), newchars);
        else
            break;
    }
    return str;
}

bool CreateAllDirectories(const std::string& path) {

    size_t pos = 0;

    std::string pathk = path;
    pathk = replace_str(pathk, "/", "\\");
    pathk = replace_str(pathk, "\\\\", "\\");

    // 遍历路径中的每个目录
    while ((pos = pathk.find(L'\\', pos)) != std::string::npos) {
        std::string temp = pathk.substr(0, pos);

        // 尝试创建目录
        if (!CreateDirectoryA(temp.c_str(), NULL)) {
            // 如果目录已经存在，则继续
            if (GetLastError() != ERROR_ALREADY_EXISTS) {
                std::cerr << "CreateDirectory failed: " << GetLastError() << std::endl;
                return false;
            }
        }

        // 移动到下一个目录的起始位置
        pos++;
    }

    return true;
}

bool CopyLargeFile(const char* srcPath, const char* dstPath) {
    HANDLE hSrcFile = CreateFileA(srcPath, //
        GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSrcFile == INVALID_HANDLE_VALUE) {
        std::cerr << "CreateFile failed: " << GetLastError() << std::endl;
        return false;
    }

    if (!CreateAllDirectories(dstPath)) {
        return false;
    }

    HANDLE hDstFile = CreateFileA(dstPath, //
        GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDstFile == INVALID_HANDLE_VALUE) {
        std::cerr << "CreateFile failed: " << GetLastError() << std::endl;
        CloseHandle(hSrcFile);
        return false;
    }

    const DWORD BUFFER_SIZE = 1024 * 1024; // 1MB buffer
    BYTE* buffer = new BYTE[BUFFER_SIZE];
    DWORD bytesRead = 0;
    DWORD bytesWritten = 0;

    bool error = false;
    while (ReadFile(hSrcFile, buffer, BUFFER_SIZE, &bytesRead, NULL) && bytesRead > 0) {
        WriteFile(hDstFile, buffer, bytesRead, &bytesWritten, NULL);
        if (bytesWritten != bytesRead) {
            std::cerr << "WriteFile failed: " << GetLastError() << std::endl;
            error = true;
            break;
        }
    }

    delete[] buffer;
    CloseHandle(hSrcFile);
    CloseHandle(hDstFile);

    return !error;
}

//+ fmd5	"F269BC66EC5D497BAB9F328EEDEB5639"	std::string
//+ localfile	"install/20231001.1/bin/models/caj2pdf32_1-2.7z"	std::string
//+ objkey	"install/20231001.1/bin/models/caj2pdf32_1-2.7z"	std::string
bool evSearch(int64_t fsize, std::string fmd5, std::string objkey, std::string localfile) {

    auto index = objkey.find_last_of('/');
    if (index == -1) {
        return false;
    }

    objkey = "\\" + objkey.substr(index + 1);
    Everything_SetSearchW(A2W_ANSI(objkey.c_str()).c_str());
    if (!Everything_QueryW(TRUE)) {
        return false;
    }

    int count = Everything_GetNumResults();
    for (int i = 0; i < count && i < 100; i++)
    {
        wchar_t buffer[1024] = { 0 };
        auto len = Everything_GetResultFullPathNameW(i, buffer, 1024);
        std::wstring fpath = buffer;
        if (getFileSize(W2A_ANSI(fpath.c_str()).c_str()) != fsize) {
            continue;
        }
        KMD5 kmd5;
        std::wstring md5 = kmd5.GetMD5Str(fpath.c_str());
        std::transform(md5.begin(), md5.end(), md5.begin(), ::toupper); // 将小写的都转换成大写
        if (md5 == A2W_ANSI(fmd5.c_str())) {
            printf("Everything Search %s\r\n", W2A_ANSI(fpath.c_str()).c_str());
            if (CopyLargeFile(W2A_ANSI(fpath.c_str()).c_str(), localfile.c_str())) {
                return true;
            }
        }
    }

    return false;
}

//int main(int argc, char** argv)
int main(int argc, char** argv)
{
    std::string buildid;
    bool input = false;
    std::regex regexStr("^[0-9]{8}\\.[0-9]+$");
    std::smatch matchResult;
    for (int i = 1; i < argc; i++) {
        std::string inputData = argv[i];
        if (std::regex_match(inputData, matchResult, regexStr)) {
            for (auto ele : matchResult) {
                buildid = ele;
                input = false;
            }
        }
    }
    while (buildid.empty()) {
        std::cout << "请输入构建 ID（形如“20230927.33”的字符串）：" << std::endl;
        std::string inputData;
        std::getline(std::cin, inputData);

        if (std::regex_match(inputData, matchResult, regexStr)) {
            for (auto ele : matchResult) {
                buildid = ele;
                input = true;
            }
        }
    }

    // 初始化SDK
    InitializeSdk();

    // 配置实例
    ClientConfiguration conf;
    conf.verifySSL = false;
    OssClient client(INFOSS_REGION_ID, INFOSS_ACCESS_KEY_ID, INFOSS_ACCESS_KEY_SECRET, conf);

    if (true) {
        // 创建API请求
        ListBucketsRequest request;
        auto outcome = client.ListBuckets(request);
        if (!outcome.isSuccess()) {
            // 异常处理
            std::cout << "ListBuckets fail" <<
                ",code:" << outcome.error().Code() <<
                ",message:" << outcome.error().Message() <<
                ",requestId:" << outcome.error().RequestId() << std::endl;
            ShutdownSdk();
            if (input) {
                printf("fail（按回车结束）");
                getchar();
            }
            return -1;
        }
    }

    std::string targetdir = "install/" + buildid + "/";
    if (true) {
        ListObjectsV2Request request(INFOSS_BUCKET);
        request.setMaxKeys(1000); // 一次拉完。
        request.setPrefix(targetdir);
        auto outcome = client.ListObjectsV2(request);
        if (outcome.isSuccess()) {
            auto result = outcome.result();
            auto count = result.KeyCount();
            auto objs = result.ObjectSummarys();
            for (auto obj : objs) {

                std::string objkey = obj.Key(); // utf8
                std::string localfile = UTF8_TO_ANSI(objkey);

                std::string fdir = localfile;
                int index = -1;
                if ((index = fdir.rfind('/')) != -1) {
                    fdir = fdir.substr(0, index);
                    fdir = replace_str(fdir, "/", "\\");
                    auto temp = AlibabaCloud::OSS::CreateDirectory(fdir);
                    assert(temp);
                }
                if (PathFileExistsA(localfile.c_str())) {
                    KMD5 kmd5;
                    std::wstring md5 = kmd5.GetMD5Str(A2W_ANSI(localfile.c_str()).c_str());
                    std::transform(md5.begin(), md5.end(), md5.begin(), ::toupper); // 将小写的都转换成大写
                    if (md5 == A2W_ANSI(obj.ETag().c_str())) {
                        printf("校验通过 %s\r\n", localfile.c_str());
                        continue;
                    }
                }

                if (evSearch(obj.Size(), obj.ETag(), objkey, localfile)) {
                    printf("本机命中 %s\r\n", localfile.c_str());
                    continue;
                }

                printf("下载文件 %s\r\n", localfile.c_str());
                auto status = client.GetObject(INFOSS_BUCKET, objkey, localfile);
                assert(status.isSuccess());
            }
        }
    }

    // 关闭SDK
    ShutdownSdk();
    if (input) {
        printf("ok（按回车结束）");
        getchar();

        std::wstring target = L" /select, install\\" + A2W_ANSI(buildid.c_str()) + L"\\ ";

        SHELLEXECUTEINFO shex = { 0 };
        shex.cbSize = sizeof(SHELLEXECUTEINFO);
        shex.lpFile = L"explorer";
        shex.lpParameters = target.c_str();
        shex.lpVerb = L"open";
        shex.nShow = SW_SHOWDEFAULT;
        shex.lpDirectory = NULL;
        ShellExecuteEx(&shex);
    }
    return 0;
}
