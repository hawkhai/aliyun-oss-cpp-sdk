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
        std::cout << "�����빹�� ID�����硰20230927.33�����ַ�������" << std::endl;
        std::string inputData;
        std::getline(std::cin, inputData);

        if (std::regex_match(inputData, matchResult, regexStr)) {
            for (auto ele : matchResult) {
                buildid = ele;
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

    std::string targetdir = "install/" + buildid + "/";
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

                std::string objkey = obj.Key(); // utf8
                std::string lobalfile = UTF8_TO_ANSI(objkey);

                std::string fdir = lobalfile;
                int index = -1;
                if ((index = fdir.rfind('/')) != -1) {
                    fdir = fdir.substr(0, index);
                    fdir = replace_str(fdir, "/", "\\");
                    auto temp = AlibabaCloud::OSS::CreateDirectory(fdir);
                    assert(temp);
                }
                if (PathFileExistsA(lobalfile.c_str())) {
                    KMD5 kmd5;
                    std::wstring md5 = kmd5.GetMD5Str(A2W_ANSI(lobalfile.c_str()).c_str());
                    std::transform(md5.begin(), md5.end(), md5.begin(), ::toupper); // ��Сд�Ķ�ת���ɴ�д
                    if (md5 == A2W_ANSI(obj.ETag().c_str())) {
                        printf("У��ͨ�� %s\r\n", lobalfile.c_str());
                        continue;
                    }
                }

                printf("�����ļ� %s\r\n", lobalfile.c_str());
                auto status = client.GetObject(INFOSS_BUCKET, objkey, lobalfile);
                assert(status.isSuccess());
            }
        }
    }

    // �ر�SDK
    ShutdownSdk();
    if (input) {
        printf("ok�����س�������");
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
