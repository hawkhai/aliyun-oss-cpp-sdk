#include <alibabacloud/oss/OssClient.h>
#include <..\..\sdk\src\utils\FileSystemUtils.h>
using namespace AlibabaCloud::OSS;

#include "..\..\..\build\infoss.h"
#include <assert.h>
#include <iostream>
#include <regex>
#include <string>
#include <Windows.h>
#include "..\..\..\..\cppx\cppx\include\funclib.h"

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

//int main(int argc, char** argv)
int main(int argc, char** argv)
{
    std::string signid;
    bool input = false;
    std::regex regexStr("^[0-9]{8}\\.[0-9]+$");
    std::smatch matchResult;
    for (int i = 1; i < argc; i++) {
        std::string inputData = argv[i];
        if (std::regex_match(inputData, matchResult, regexStr)) {
            for (auto ele : matchResult) {
                signid = ele;
                input = false;
            }
        }
    }
    while (signid.empty()) {
        std::cout << "请输入构建 ID（形如“20230926.7”的字符串）：" << std::endl;
        std::string inputData;
        std::getline(std::cin, inputData);

        if (std::regex_match(inputData, matchResult, regexStr)) {
            for (auto ele : matchResult) {
                signid = ele;
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

    std::string targetdir = "install/" + signid + "/";
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
                printf("下载文件 %s\r\n", obj.Key().c_str());

                std::string fdir = obj.Key();
                int index = -1;
                if ((index = fdir.rfind('/')) != -1) {
                    fdir = fdir.substr(0, index);
                    fdir = replace_str(fdir, "/", "\\");
                    auto temp = AlibabaCloud::OSS::CreateDirectory(fdir);
                    assert(temp);
                }
                auto status = client.GetObject(INFOSS_BUCKET, obj.Key(), obj.Key());
                assert(status.isSuccess());
            }
        }
    }

    // 关闭SDK
    ShutdownSdk();
    if (input) {
        printf("ok（按回车结束）");
        getchar();

        std::wstring target = L" /select, install\\" + A2W_ANSI(signid.c_str()) + L"\\ ";

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
