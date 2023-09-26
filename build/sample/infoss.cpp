#include <alibabacloud/oss/OssClient.h>
using namespace AlibabaCloud::OSS;

#include "..\..\..\build\infoss.h"

//int main(int argc, char** argv)
int main(void)
{
    // ��ʼ��SDK
    InitializeSdk();

    // ����ʵ��
    ClientConfiguration conf;
    conf.verifySSL = false;
    OssClient client(INFOSS_REGION_ID, INFOSS_ACCESS_KEY_ID, INFOSS_ACCESS_KEY_SECRET, conf);

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
        return -1;
    }



    // �ر�SDK
    ShutdownSdk();
    return 0;
}
