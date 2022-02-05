/* Socket client Example

   Edit:   Starmark LN
   e-mail: ln.starmark@gmail.com   
		   ln.starmark@ekatra.io  	
   date:   04.02.22
   
   cd examples\protocols\http_request_starmark
   idf.py menuconfig
   idf.py build
   idf.py –p COM41 flash
   idf.py –p COM41 monitor  
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>  

#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"

//--- Для работы с консолью ------
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"
#include "driver/uart.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "addr_from_stdin.h"
#include "lwip/err.h"
#include "lwip/sockets.h"


#if defined(CONFIG_EXAMPLE_IPV4)
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR
#elif defined(CONFIG_EXAMPLE_IPV6)
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV6_ADDR
#else
#define HOST_IP_ADDR ""
#endif

#define DEBUG_EN

#define PORT CONFIG_EXAMPLE_PORT

#define NUMBER_OF_MESSAGES	6

#ifndef	DEBUG_EN
static void menu(void);
static int input_int(void);
static int cmd_to_work(int val);
#endif 

static const char *TAG = "example";
char *payload[NUMBER_OF_MESSAGES] = {"No CMD", "Start", "Engine on", "Engine off", "TEST_MESSAGE", "Abracadabra"};
static const char *MENU_TEXT = 	"\r\n List commands\r\n"
	"================\r\n"
	"No CMD       - 0\r\n"
	"Start        - 1\r\n"
	"Engine on    - 2\r\n"
	"Engine off   - 3\r\n"
	"TEST_MESSAGE - 4\r\n"
	"Abracadabra  - 5\r\n"
	"\r\n";
	
int sock;	

static void tcp_client_task(void *pvParameters)
{
    char rx_buffer[128];

    char host_ip[] = HOST_IP_ADDR;
    int addr_family = 0;
    int ip_protocol = 0;
	
	char *pay;

    while (1) {
		
#if defined(CONFIG_EXAMPLE_IPV4)
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(host_ip);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
#elif defined(CONFIG_EXAMPLE_IPV6)
        struct sockaddr_in6 dest_addr = { 0 };
        inet6_aton(host_ip, &dest_addr.sin6_addr);
        dest_addr.sin6_family = AF_INET6;
        dest_addr.sin6_port = htons(PORT);
        dest_addr.sin6_scope_id = esp_netif_get_netif_impl_index(EXAMPLE_INTERFACE);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
#elif defined(CONFIG_EXAMPLE_SOCKET_IP_INPUT_STDIN)
        struct sockaddr_storage dest_addr = { 0 };
        ESP_ERROR_CHECK(get_addr_from_stdin(PORT, SOCK_STREAM, &ip_protocol, &addr_family, &dest_addr));
#endif

        int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, PORT);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Successfully connected");
    		
        while (1) {
			
#ifndef	DEBUG_EN
			//--- input CMD  
			menu();			
			pay = payload[cmd_to_work(input_int())];
#else	
			//--- Auto CMD (debug)
			static int  cnt_pay = 0;
			pay = payload[cnt_pay++];
#endif		
			
            int err = send(sock, pay, strlen(pay), 0);
            if (err < 0) {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                break;
            }

			//--- прием ответа с сервера
            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            if (len < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            }
            else {
                rx_buffer[len] = 0; // Null-terminate string
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
                ESP_LOGI(TAG, "%s", rx_buffer);
            }

#ifdef	DEBUG_EN
			//--- Для тестирования можно орг цикл char *pay = payload[cnt_pay++];
			if(cnt_pay==NUMBER_OF_MESSAGES) {
				cnt_pay = 0;
			}
#endif
			
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
		
    }
	
    vTaskDelete(NULL);
}

//===============================================================================

#ifndef	DEBUG_EN
static void menu(void)
{
	printf(MENU_TEXT);
}

static int input_int(void)
{
	char ch[17];
	for(bool ok = false;!ok; ) {
		ok = true;
		printf("Enter number: ");
		fgets(ch,16,stdin);

		if (ch[strlen(ch)-1] == '\n') 
			ch[strlen(ch)-1] = 0;
		
		for(int i = 0; ch[i]; ++i) {
			if (!isdigit(ch[i]))
			{
				puts("Error input. Retry");
				ok = false;
				break;
			}
		}	
	}
			
	return atoi(ch);	
}

static int cmd_to_work(int val)
{
	int cnt_pay = 0;
	
	if((val < NUMBER_OF_MESSAGES) && (val >= 0)){
		cnt_pay=val;
	}

	printf("cnt_pay %d\r\n", cnt_pay);
	
	return cnt_pay;
}
#endif

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
	
	//--- Для работы консоли
	setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0));
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);
	
    ESP_ERROR_CHECK(example_connect());

    xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);
}
