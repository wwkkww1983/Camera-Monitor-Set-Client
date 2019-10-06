#include "iostream"
#include "cstdlib"
#include "string"

#include "video.h"
#include "audio.h"
#include "utils.h"

#include "udpBroadcast/udpBroadcast.h"
#include "udpClient/udpClient.h"

using namespace std;

#define ID_LIFE_TIME 99 //micro-seconds
#define ID_CIRCLE 100

int id = 0;

/////
#include <stdio.h>      
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
/////

int main(int argc, char **argv)
{
    vector<string> vec = UdpBroadcast::getAddrs();
    UdpBroadcast* target[vec.size()];

    char msg[] = "hello";

    for(int i = 0; i < vec.size(); i++)
    {
        target[i] = new UdpBroadcast(6000, (char*) vec[i].c_str());
        target[i]->sendMsg(msg, sizeof(msg)-1);
    }

    // string dev(argv[1]);
    // int xres = atoi(argv[2]);
    // int yres = atoi(argv[3]);
    // int fps = atoi(argv[4]);
    // string audioDevice = argv[5];
    // string audioDevice2 = argv[6];
    // string vUrl(argv[7]);
    // string aUrl(argv[8]);
    // string aUrl2(argv[9]);

    // Video video(dev, xres, yres, fps);
    // Audio audio(audioDevice);
    // Audio audio2(audioDevice2);

    // video.start(vUrl);
    // cerr << "video start" << endl;
    // audio.start(aUrl, 15000);
    // cerr << "audio1 start" << endl;
    // audio2.start(aUrl2, 15000);
    // cerr << "audio2 start" << endl;

    // int id = 0;
    // long long lastTime = 0;
    // while(true)
    // {
    //     long long now = getTime();
    //     if(now - lastTime > ID_LIFE_TIME)
    //     {
    //         video.setId(id);
    //         audio.setId(id);
    //         audio2.setId(id);

    //         if(id >= ID_CIRCLE)
    //         {
    //             id = 0;
    //         }
    //         else
    //         {
    //             id++;
    //         }
            
    //         lastTime = now;
    //     }
    // }


    return 0;
}