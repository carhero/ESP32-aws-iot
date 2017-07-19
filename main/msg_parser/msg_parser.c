#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "msg_parser.h"

extern int sock;

typedef enum {

    /* For Controlling MRX-x20 */
    eCOMMAND_POWER  = 0,
    eCOMMAND_VOLUME,
    eCOMMAND_FUNCTION,
    eCOMMAND_VOLUME_LEVEL,
    eCOMMAND_TUNER,

    /* For Controlling ESP32 */
    eCOMMAND_LED_CTRL,
    eCOMMAND_GAS_CTRL,

    eCOMMAND_MAX,

} eCOMMAND_INDEX;

typedef enum {
    eFM_SEEK_UP = 0,
    eFM_SEEK_DN,
    eFM_TUNE_UP,
    eFM_TUNE_DN,    
    eFM_MAX

} FM_INDDX;

eCOMMAND_INDEX command_index = 0;

static void SystemCtrl_Power(char* data)
{

    printf("atoi(data):%d\n",atoi(data));

    // switch(data[0]-'0')
    switch(atoi(data))
    {
        case 0:     // off
            write(sock, "Z1POW0;", strlen("Z1POW0;"));
            break;
        case 1:     // on
            write(sock, "Z1POW1;", strlen("Z1POW1;"));
            break;
        default:
            break;
    }
}

static void SystemCtrl_Volume(char* data)
{
    unsigned int VolData = 0;
    unsigned char desiredVolLev = 0;
    char sendBuf[20] = {0,};
    
    printf("atoi(data):%d\n",atoi(data));
    VolData = atoi(data);

    if(VolData & 0x10000)
    {
        memset(sendBuf, 0, sizeof(sendBuf));
        desiredVolLev = (unsigned char)((VolData >> 8) & 0x000FF);
    }
    else
    {
        desiredVolLev = 2;  // temporary volume step
    }

    switch(VolData & 0x000FF)
    {
        case 0:     // volume down
            if(VolData & 0x10000)   // if it has specific volume level control
            {                
                sprintf(sendBuf, "Z1VDN%d;", desiredVolLev);                
            }
            else
            {
                sprintf(sendBuf, "Z1VDN%d;", desiredVolLev);
            }
            printf("volume-down-sendBuf:%s", sendBuf);
            write(sock, sendBuf, strlen(sendBuf));
            break;
        case 1:     // volume up
            if(VolData & 0x10000)   // if it has specific volume level control
            {                
                sprintf(sendBuf, "Z1VUP%d;", desiredVolLev);                
            }
            else
            {
                sprintf(sendBuf, "Z1VUP%d;", desiredVolLev);
            }
            printf("volume-up-sendBuf:%s", sendBuf);
            write(sock, sendBuf, strlen(sendBuf));
            break;
        case 2:     // volume mute
            write(sock, "Z1MUT1;", strlen("Z1MUT1;"));
            break;
        case 3:     // volume unmute
            write(sock, "Z1MUT0;", strlen("Z1MUT0;"));
            break;
        default:
            break;
    }
}

static void SystemCtrl_Function(char* data)
{   
    int indexoffunc = 0;
    char sendMsg[12] = {0,};

    printf("atoi(data):%d\n",atoi(data));
    indexoffunc = atoi(data);

    sprintf(sendMsg, "Z1INP%d;", indexoffunc);
    write(sock, sendMsg, strlen(sendMsg));
}

static void SystemCtrl_SetVolLevel(char* data)
{   
    char sendBuf[10] = {0,};
    int volume = atoi(data);
    
    memset(sendBuf, 0, sizeof(sendBuf));
    printf("atoi(data):%d\n",volume);

    // Z1VOL-35, Z1VOL+10
    if(volume <= 90) 
    {
        sprintf(sendBuf, "Z1VOL-%d;", 90-volume);
    } 
    else 
    {
        sprintf(sendBuf, "Z1VOL+%d;", volume-90);
    }

    write(sock, sendBuf, strlen(sendBuf));
}

static void SystemCtrl_Tuner(char* data)
{   
    // char sendBuf[10] = {0,};
    int state = atoi(data);
    char *comBuf[] = {        
        "T1KUP",
        "T1KDN",
        "T1TUP",
        "T1TDN",
    };
    char sendBuf[10] = {0,};

    // T1TUP	Tune up
    // T1TDN	Tune down
    // T1KUP	Seek up
    // T1KDN	Seek down

    printf("atoi(data):%d\n",state);

    if(state > eFM_MAX-1)
    {
        printf("index overflow error : %d\n",state);
        return;
    }
    
    memset(sendBuf, 0, sizeof(sendBuf));

    sprintf(sendBuf, "%s;",comBuf[state]);

    write(sock, sendBuf, strlen(sendBuf));
}

static void SystemCtrl_esp32_ledctrl(char* data)
{   
    //ESP32 gpio control is needed to control LED(Off, On, Blink)
    unsigned int value = atoi(data);    
    
    // 0x00~0x02 : BLUE LED
    // 0x10~0x12 : RED LED
    // 0x*0~0x*2 : *** LED (TBD)

    /* 0:LED OFF, 1:LED ON, 2:LED BLINK */


}

static void SystemCtrl_esp32_gasctrl(char* data)
{   
    //ESP32 servo moter control is needed (lock gas valve / unlock gas valve)

}

static eCOMMAND_INDEX msg_parser_command_data(char *msg)
{
    eCOMMAND_INDEX ret = eCOMMAND_MAX;
    
    // if new command type is appered
    if(strstr(msg, "command") != NULL)
    {
        if(strstr(msg, "power") != NULL) 
        {
            ret = eCOMMAND_POWER;
        } 
        else if(strstr(msg, "vol_level") != NULL) 
        {
            ret = eCOMMAND_VOLUME_LEVEL;
        }
        else if(strstr(msg, "volume") != NULL) 
        {
            ret = eCOMMAND_VOLUME;
        }
        else if(strstr(msg, "function") != NULL) 
        {
            ret = eCOMMAND_FUNCTION;
        }
        else if(strstr(msg, "tuner") != NULL) 
        {
            ret = eCOMMAND_TUNER;
        }
        /* For Controlling ESP32 */
        else if(strstr(msg, "ledcontrol") != NULL) 
        {
            ret = eCOMMAND_LED_CTRL;
        }
        else if(strstr(msg, "gasvalve") != NULL) 
        {
            ret = eCOMMAND_GAS_CTRL;
        }
    }

    return ret;
}

void msg_parser_mqtt_data(unsigned char* str, unsigned int length)
{
    char *strptr = NULL;
    int cnt = 0;

    char *msg = (char *)str;
    
    // if new command type is appered
    command_index = msg_parser_command_data(msg);

    strptr = strstr(msg, "value");
    printf("strptr : %s, command_index:%d\n",strptr, command_index);

    if(strptr == NULL) 
    {
        printf("str is NULL\n");
        return;
    }
    
    strptr += 7;

    for(cnt = 0; cnt < 20; cnt += 1) 
    {
        if(strptr[0] >= '0' && strptr[0] <= '9') 
        {
            break;
        }
        else 
        {
            strptr += 1;                
        }
    }
    
    printf("strptr+%d : %s\n",cnt, strptr);        

    switch(command_index)
    {
        /* For contorlling MRX-x20 */
        case eCOMMAND_POWER:
        {
            SystemCtrl_Power(strptr);
            break;
        }
        case eCOMMAND_VOLUME:
        {
            SystemCtrl_Volume(strptr);
            break;
        }
        case eCOMMAND_FUNCTION:
        {
            SystemCtrl_Function(strptr);
            break;
        }
        case eCOMMAND_VOLUME_LEVEL:
        {
            SystemCtrl_SetVolLevel(strptr);
            break;
        }
        case eCOMMAND_TUNER:
        {
            SystemCtrl_Tuner(strptr);
            break;
        }
        /* For contorlling ESP32 */
        case eCOMMAND_LED_CTRL:
        {
            SystemCtrl_esp32_ledctrl(strptr);
            break;
        }
        case eCOMMAND_GAS_CTRL:
        {
            SystemCtrl_esp32_gasctrl(strptr);
            break;
        }
        default:    break;
    }
}

void msg_parser_ddsp_data(const char* msg, unsigned int length)
{
    
}