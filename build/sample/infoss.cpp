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

// ���������
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
        std::cout << "�����빹�� ID�����硰20230926.7�����ַ�������" << std::endl;
        std::string inputData;
        std::getline(std::cin, inputData);

        if (std::regex_match(inputData, matchResult, regexStr)) {
            for (auto ele : matchResult) {
                signid = ele;
                input = true;
            }
        }
    }

    // ��ʼ��SDK
    InitializeSdk();

    // ����ʵ��
    ClientConfiguration conf;
    conf.verifySSL = false;
    OssClient client(INFOSS_REGION_ID, INFOSS_ACCESS_KEY_ID, INFOSS_ACCESS_KEY_SECRET, conf);

    if (true) {
        // ����API����
        ListBucketsRequest request;
        auto outcome = client.ListBuckets(request);
        if (!outcome.isSuccess()) {
            // �쳣����
            std::cout << "ListBuckets fail" <<
                ",code:" << outcome.error().Code() <<
                ",message:" << outcome.error().Message() <<
                ",requestId:" << outcome.error().RequestId() << std::endl;
            ShutdownSdk();
            if (input) {
                printf("fail�����س�������");
                getchar();
            }
            return -1;
        }
    }

    std::string targetdir = "install/" + signid + "/";
    if (true) {
        ListObjectsV2Request request(INFOSS_BUCKET);
        request.setMaxKeys(1000); // һ�����ꡣ
        request.setPrefix(targetdir);
        auto outcome = client.ListObjectsV2(request);
        if (outcome.isSuccess()) {
            auto result = outcome.result();
            auto count = result.KeyCount();
            auto objs = result.ObjectSummarys();
            for (auto obj : objs) {
                printf("�����ļ� %s\r\n", obj.Key().c_str());

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

    // �ر�SDK
    ShutdownSdk();
    if (input) {
        printf("ok�����س�������");
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
