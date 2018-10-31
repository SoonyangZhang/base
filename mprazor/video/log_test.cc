#include <stdio.h>
#include "log.h"
using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("MainFile");
void oom(){
NS_LOG_FUNCTION("hi");
}
int main(){
//LogComponentEnable("MainFile", LOG_LEVEL_ERROR);
LogComponentEnable("MainFile", LOG_LEVEL_ALL);
NS_LOG_INFO ("Run Simulation.");
NS_LOG_ERROR("fatal error");
    printf("what the fuck\n");
oom();
}
