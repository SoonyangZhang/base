#include "mpsender.h"
#include "log.h"
using namespace zsy;
using namespace ns3;
int main()
{
    LogComponentEnable("MultipathSender", LOG_LEVEL_ALL);
    MultipathSender sender(NULL,1);
    char buf[4500]={0};
    sender.SendVideo(1,2,buf,4500);
    return 0;
}
