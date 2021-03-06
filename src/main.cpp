#include "iostream"
#include "cstdlib"
#include "string"

#include "video.h"
#include "audio.h"
#include "utils.h"

#include "connection.h"
// #include "tcpServer/tcpServer.h"

using namespace std;

#define ID_LIFE_TIME 99 //micro-seconds
#define ID_CIRCLE 100

int id = 0;

// TcpServer *svr = NULL;

int main(int argc, char **argv)
{
    struct config *cfg = connectToServer();

    string dev(cfg->videoDevice);
    int xres = cfg->xres;
    int yres = cfg->yres;
    int fps = cfg->fps;
    string audioDevice = cfg->audioDevice1;
    string audioDevice2 = cfg->audioDevice2;
    string vUrl = cfg->videoUrl;
    string aUrl = cfg->audioUrl1;
    string aUrl2 = cfg->audioUrl2;

    Video video(dev, xres, yres, fps);
    Audio audio(audioDevice);
    Audio audio2(audioDevice2);

    video.start(vUrl);
    cerr << "video start" << endl;
    audio.start(aUrl, 15000);
    cerr << "audio1 start" << endl;
    audio2.start(aUrl2, 15000);
    cerr << "audio2 start" << endl;

    cout << "\nrecording\n" << endl;

    int id = 0;
    long long lastTime = 0;
    while(true)
    {
        long long now = getTime();
        if(now - lastTime > ID_LIFE_TIME)
        {
            video.setId(id);
            audio.setId(id);
            audio2.setId(id);

            if(id >= ID_CIRCLE)
            {
                id = 0;
            }
            else
            {
                id++;
            }
            
            lastTime = now;
        }
    }

    return 0;
}