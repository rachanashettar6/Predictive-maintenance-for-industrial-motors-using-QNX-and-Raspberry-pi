#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <curl/curl.h>
#include <pthread.h>
#include <sched.h>
#include <sys/neutrino.h>
typedef struct {
    float temp;
    int rpm;
    char vibration[10];
    char risk[20];
} sensor_data_t;

int chid;
void set_priority()
{
    struct sched_param param;
    param.sched_priority = 20;
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0)
    {
        perror("Failed to set priority");
    }
    else
    {
        printf("[INFO] Real-time priority enabled\n");
    }
}
void* processing_thread(void* arg)
{
    int rcvid;
    sensor_data_t data;
    while (1)
    {
        rcvid = MsgReceive(chid, &data, sizeof(data), NULL);
        if (rcvid <= 0) continue;
        printf("[PROCESS] Temp: %.1f | RPM: %d | Vib: %s | Risk: %s\n",
               data.temp, data.rpm, data.vibration, data.risk);
        char json[256];
        sprintf(json,
            "{ \"temp\": \"%.1f\", \"rpm\": \"%d\", \"vibration\": \"%s\", \"risk\": \"%s\" }",
            data.temp, data.rpm, data.vibration, data.risk);
        CURL *curl = curl_easy_init();
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL,
                "https://industrial-motor-default-rtdb.firebaseio.com/.json");
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            if (curl_easy_perform(curl) == CURLE_OK)
            {
                printf("[INFO] Firebase updated\n");
            }
            else
            {
                printf("[ERROR] Firebase update failed\n");
            }

            curl_easy_cleanup(curl);
        }

        MsgReply(rcvid, 0, NULL, 0);
    }
}
void* handle_client(void* arg)
{
    int client_fd = *(int*)arg;
    free(arg);
    char buffer[4096];
    int total = 0;
    while (1)
    {
        int n = read(client_fd, buffer + total, sizeof(buffer) - total - 1);
        if (n <= 0) break;
        total += n;
    }
    buffer[total] = '\0';
    sensor_data_t data;
    char *json_start = strstr(buffer, "{");
    if (json_start)
    {
        sscanf(json_start,
            "{\"temp\":%f,\"rpm\":%d,\"vibration\":\"%[^\"]\",\"risk\":\"%[^\"]\"}",
            &data.temp, &data.rpm, data.vibration, data.risk);
        printf("[RECV] Temp: %.1f | RPM: %d\n", data.temp, data.rpm);
        int coid = ConnectAttach(0, 0, chid, 0, 0);
        if (coid != -1)
        {
            MsgSend(coid, &data, sizeof(data), NULL, 0);
            ConnectDetach(coid);
        }
    }
    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
    write(client_fd, response, strlen(response));

    close(client_fd);
    return NULL;
}
int main()
{
    set_priority();
    chid = ChannelCreate(0);
    if (chid == -1)
    {
        perror("Channel creation failed");
        return -1;
    }
    pthread_t proc_thread;
    pthread_create(&proc_thread, NULL, processing_thread, NULL);
    int server_fd;
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("Socket creation failed");
        return -1;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("Bind failed");
        return -1;
    }
    listen(server_fd, 5);
    printf("[INFO] Server started on port 8080...\n");
    while (1)
    {
        int *client_fd = (int*) malloc(sizeof(int));
        *client_fd = accept(server_fd,
            (struct sockaddr *)&addr,
            (socklen_t*)&addrlen);
        if (*client_fd < 0) {
            perror("Accept failed");
            free(client_fd);
            continue;
        }
        printf("[INFO] ESP32 connected\n");
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, client_fd);
        pthread_detach(tid);
    }
    close(server_fd);
    return 0;
}
