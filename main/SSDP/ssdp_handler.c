/*
 * crate_thread.c
 *
 *  Created on: Jun 07, 2017
 *      Author: yhcha
 */

// #include <stdio.h>
// #include <string.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include "pthread.h"

// network define
// #include <arpa/inet.h>
// #include <netdb.h>
// #include <net/if.h>
// #include <dlfcn.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"

#include <sys/socket.h>
#include "msg_parser.h"
#include "ssdp_handler.h"

// #include "_pthreadtypes.h"

#define BUF_SIZE  1024

static const char *TAG = "ssdp-parser";


xSemaphoreHandle xSemaphore = NULL;

// SSDP packet data structure
typedef struct _device_descriptor_t
{
	int8_t	unique_header[0x4];
	int8_t	reserved1_rw0;
	int8_t	reserved2_rw0;
	int8_t	announce;
	int8_t	powering_off;
	int32_t	version;
	int32_t	tcp_ip_port;
	int8_t	device_name[0x10];
	int8_t	model_name[0x10];
	int8_t	serial_number[0x10];
} device_descriptor_t /*__attribute__ ((__packed__))*/;

device_descriptor_t st_packet = {
	.unique_header = "PARC",
	.reserved1_rw0 = 0,
	.reserved2_rw0 = 0,
	.announce = 1,
	.version = 0x01000000,
	.tcp_ip_port = 0,
};

// Mqtt json data variable
unsigned int JsonStrLen;
unsigned char JsonBuf[1024];
unsigned int JsonFlagUpdate;

// linux thread variable
// pthread_t th_PollUpdate;    // polling update thread
// pthread_t th_SSDPSender;    // network SSDP serch msg sending thread
// pthread_t th_SSDPReceiver;  // network SSDP serch msg receiver thread

// Socket variable
char message[BUF_SIZE];
int ssdp_skip_flag = 0;
int Usock = NULL, sock = NULL;  // UDP, TCP socket
struct sockaddr_in serv_addr,from_adr;
socklen_t adr_sz;

// ------------------------------------------------
// API Functions
// ------------------------------------------------
void Thread_SetUpdateFlag(unsigned int update)
{
  JsonFlagUpdate = update;
}

unsigned int Thread_GetUpdateFlag(void)
{
  return JsonFlagUpdate;
}

void CopyJsonBuffer(const char *pBuf, unsigned int Length)
{
  memset(JsonBuf, 0, sizeof(JsonBuf));

  memcpy(JsonBuf, pBuf, Length);  
}

// ------------------------------------------------
// thread task
// ------------------------------------------------
void thread_PollingUpdate(void* ptr)
{
  // unsigned int BufCnt = 0;
  // unsigned int cnt = 0;
  ESP_LOGI(TAG, "thread_PollingUpdate init...\n");

  while(1)
  {    
    //while( (sock < 0) || ssdp_skip_flag == 0) // If TCP socket is not ready, polling update should be not called
    if( (sock < 0) || ssdp_skip_flag == 0)
    {
      continue;
    }

    if(Thread_GetUpdateFlag())
    {
      JsonFlagUpdate = 0;
      ESP_LOGI(TAG, "JsonFlagUpdate requsted \n");
      ESP_LOGI(TAG, "Copyed buffer:%s \n", JsonBuf);

      // if data sending is avilable, send tcp/ip command to MRX
      msg_parser_mqtt_data(JsonBuf, (size_t)strlen((char *)JsonBuf));
    }

    /* escape routine is needed to close TCP socket     
    if(escape condition)
    {
      if(ssdp_skip_flag)
      {
        close(sock);
      }      
      break;
    }
    */
    vTaskDelay(10 / portTICK_RATE_MS);
  }
  
  // return 0;
}


// ------------------------------------------------
// TEST Code for TCP Connection with MRX
// ------------------------------------------------
#define BUF_SIZE 1024
void error_handling(char *message);

//int main(int argc, char *argv[])
int TCP_Connection(struct sockaddr_in *from_addr)
{
	// int sock;
	// char message[BUF_SIZE];
	// int str_len;
	struct sockaddr_in serv_adr;

  char *TDP_PORT = "14999";

	sock=socket(PF_INET, SOCK_STREAM, 0);   
	if(sock==-1)
		error_handling("socket() error");
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=from_addr->sin_addr.s_addr; //inet_addr(argv[1]);
	serv_adr.sin_port=htons(atoi(TDP_PORT));
	
	if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
  {
    error_handling("connect() error!");
  }		
	else
  {
    ESP_LOGI(TAG, "Connected...........");
    ESP_LOGI(TAG, "TCP Server IP : %X\n",serv_adr.sin_addr.s_addr);
    // xTaskCreatePinnedToCore(&thread_PollingUpdate, "thread_PollingUpdate", 1024*2, NULL, 1, NULL, 1);
  }
		
	//close(sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

// ------------------------------------------------
// SSDP Sender function
// ------------------------------------------------
static int MSearchMessageDataSend(void)
{    
  Usock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);	
  if(Usock==-1) 
  {
		ESP_LOGI(TAG, "socket() error\n");
    return -1;
	}

  // It is very important to create broadcast message on UDP
  int broadcast = 1;
	if( setsockopt(Usock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) != 0 )
	{
	    perror("setsockopt : ");
	    close(Usock);
	    return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
  memset(&st_packet, 0, sizeof(st_packet));

  //st_packet.unique_header = "PARC";
	st_packet.announce = 0x01;
	st_packet.version = 0x01000000;
	st_packet.tcp_ip_port = 0;

	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr("255.255.255.255");
	serv_addr.sin_port=htons(atoi("14999"));
 
  sendto(Usock, (unsigned char*)&st_packet, sizeof(st_packet), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

  return 0;
  //ssdp_skip_flag = 0;
}

void thread_SSDPSender(void* ptr)
{
  //unsigned int cnt = 0;
  ESP_LOGI(TAG, "thread_SSDPSender init...\n");

  while(1)
  {
#if 0
    while(ssdp_skip_flag)  // If have tcp connection with MRX, then this flag is set to 1
    {
      ;
    }
#endif
    // Send broadcast ping message
    ESP_LOGI(TAG, "send SSDP msg... \n");        

    MSearchMessageDataSend();

    vTaskDelay(2000 / portTICK_RATE_MS);
    //vTaskDelay(200);

    if(ssdp_skip_flag)
    {
      break;
    }
  }

  vTaskDelete(NULL);
  // return;
}

void thread_SSDPReceiver(void* ptr)
{
  // unsigned int cnt = 0;  
  // int count1;

  ESP_LOGI(TAG, "thread_SSDPReceiver init...\n");

  while(1)
  {
    int ret = 0;
    // If TCP connection is estbalished, then it should be skipped.
    //while(ssdp_skip_flag)  // If have tcp connection with MRX, then this flag is set to 1
    adr_sz = sizeof(from_adr);
  
    memset(&from_adr, 0, sizeof(from_adr));

    //ret = recvfrom(Usock, message, BUF_SIZE, 0,(struct sockaddr*)&from_adr, &adr_sz);
    ret = recvfrom(Usock, &st_packet, sizeof(st_packet), 0,(struct sockaddr*)&from_adr, &adr_sz);
      
    if(ret > 0)
    {
      #if 1 // debug
      printf("rev size : %d\n", ret);    
      printf("unique_header:%s\n", st_packet.unique_header);
      printf("tcp_ip_port:%d\n", st_packet.tcp_ip_port);
      printf("device_name:%s\n", st_packet.device_name);
      printf("model_name:%s\n", st_packet.model_name);
      printf("serial_number:%s\n", st_packet.serial_number);
      printf("from_adr.sin_addr:%X\n", from_adr.sin_addr.s_addr);
      #endif

      if(from_adr.sin_addr.s_addr != 0)  // add more conditions
      {      
        if(TCP_Connection(&from_adr) == 0)
        {
          ssdp_skip_flag = 1;
          break;
        }
      }

      // Clear temp buffer data
      memset(message, 0, BUF_SIZE);
    }
    adr_sz = 0;

    //sleep(2);
    //vTaskDelay(200);
    vTaskDelay(50 / portTICK_RATE_MS);
  }
  
  vTaskDelete(NULL);
  // return 0;
}


int Thread_Create(void)
{    
  // vSemaphoreCreateBinary( xSemaphore );

  // pthread_create( &th_PollUpdate, NULL, thread_PollingUpdate, NULL);
  // pthread_create( &th_SSDPSender, NULL, thread_SSDPSender, NULL);
  // pthread_create( &th_SSDPReceiver, NULL, thread_SSDPReceiver, NULL);
  // xTaskCreate(&thread_PollingUpdate, "PollingUpdate", 1024*4, NULL, 5, NULL);

  // -3, -2, -1, 0, 1, 2, 3

  xTaskCreate(thread_SSDPSender, "SSDPSender", 1024*3, NULL, -3, NULL);
  xTaskCreate(thread_SSDPReceiver, "SSDPReceiver", 1024*3, NULL, 3, NULL);
  xTaskCreate(thread_PollingUpdate, "PollingUpdate", 1024*3, NULL, 0, NULL);

  // xTaskCreatePinnedToCore(thread_SSDPSender, "SSDPSender", 1024*3, NULL, 5, NULL, 1);
  // xTaskCreatePinnedToCore(thread_SSDPReceiver, "SSDPReceiver", 1024*3, NULL, 5, NULL, 1);
  // xTaskCreatePinnedToCore(thread_PollingUpdate, "PollingUpdate", 1024*3, NULL, 5, NULL, 1);
  // synchronize threads:
  //pthread_join(th_PollUpdate, NULL);                // pauses until first finishes
  //pthread_join(th_NetServer, NULL);               // pauses until second finishes
  
  ESP_LOGI(TAG, "Create thread completed.\n");

  return 0;
}
