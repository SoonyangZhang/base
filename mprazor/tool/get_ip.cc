#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include<iostream>
#include<map>
#include <string>
using namespace std;
typedef std::map<string,string> settings_t;

void loadLocalIp (settings_t &ipConfig)
{
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;      

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa ->ifa_addr->sa_family==AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

            string key(ifa->ifa_name);
            string value(addressBuffer);
            ipConfig.insert(std::pair<string,string>(key, value));
         }
     }
    if (ifAddrStruct!=NULL) 
        freeifaddrs(ifAddrStruct);//remember to free ifAddrStruct
}

int main()
{
    settings_t ipConfig;
    loadLocalIp(ipConfig);
    for(auto it=ipConfig.begin();it!=ipConfig.end();it++){
    std::cout<<it->second<<std::endl;
    }
    //cout<<ipConfig.at("enp2s0")<<endl;
    return 0;
}
