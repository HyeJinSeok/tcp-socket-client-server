// EOF/EOT 프로토콜을 사용하는 파일 수신 서버
// 빌드: gcc server.c -o server
// 실행 권장: cd ~/server-side && ./server

// ⚠️ 바이너리 파일에서 "EOF" 바이트 패턴이 우연히 나오면 오탐 가능

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include <sys/stat.h>       // mkdir
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT 2030
#define MAX_BUFFER_SIZE 1024
#define END_OF_FILE "EOF"
#define END_OF_TRANSFER "EOT"

// handle_client 함수
static void handle_client(int client_socket) {
    char buffer[MAX_BUFFER_SIZE];
    char directory[MAX_BUFFER_SIZE];

  
    // 1) 디렉토리 이름 받기
    ssize_t n = recv(client_socket, directory, sizeof(directory) - 1, 0);
    if (n <= 0) {
        perror("recv directory");
        close(client_socket);
        exit(1);
    }
    directory[n] = '\0';  // 안전용
    printf("[+] Received directory name: %s\n", directory);

  
    // 2) 디렉토리 생성 (있으면 통과)
    if (mkdir(directory, 0777) == -1 && errno != EEXIST) {
        perror("Error creating directory");
        close(client_socket);
        exit(1);
    }

  
    // 3) 해당 디렉토리로 이동(서버의 CWD가 바뀜)
    if (chdir(directory) == -1) {
        perror("Error changing directory");
        close(client_socket);
        exit(1);
    }

  
    // 4) 디렉토리명 ACK
    send(client_socket, "ACK", 3, 0);

  
    // 5) 파일 수신 루프
    while (1) {
        // 파일명 수신
        memset(buffer, 0, sizeof(buffer));
        ssize_t fn_bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (fn_bytes <= 0) {
            break;
        }
        buffer[fn_bytes] = '\0';  // 안전용

        // 모든 전송 종료 신호면 빠져나감
        if (strcmp(buffer, END_OF_TRANSFER) == 0) {
            printf("[*] End of transmission from client.\n");
            break;
        }

        printf("[+] Receiving file: %s\n", buffer);

        // 파일명 ACK
        send(client_socket, "ACK", 3, 0);

        // 기존 파일 있으면 삭제(덮어쓰기 목적)
        if (access(buffer, F_OK) != -1) {
            remove(buffer);
        }

        // 파일 내용 수신
        FILE* file = fopen(buffer, "wb");
        if (!file) {
            perror("Error opening file");
            continue;
        }

        while (1) {
            ssize_t bytes = recv(client_socket, buffer, sizeof(buffer), 0);
            if (bytes <= 0) {
                break;
            }

            // 버퍼의 앞부분이 EOF로 시작하면 파일 끝으로 간주

            if (bytes >= (ssize_t)strlen(END_OF_FILE) &&
                strncmp(buffer, END_OF_FILE, strlen(END_OF_FILE)) == 0) {
                break;
            }

            // 받은 바이트 그대로 파일에 기록
            fwrite(buffer, 1, bytes, file);
        }

        fclose(file);

        // 파일 내용 ACK
        send(client_socket, "ACK", 3, 0);
        printf("[+] File received successfully.\n");
    } 
  // 파일 수신 루프 끝

    close(client_socket);
    exit(0); // 자식 프로세스 종료
}
// handle_client 함수 끝


// 메인 함수
int main(void) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;

    // 좀비 프로세스 방지 (자식 종료 시 자동 회수)
    signal(SIGCHLD, SIG_IGN);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 재시작 편의(옵션)
    int yes = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("[*] Server listening on port %d...\n", PORT);

  
    // 접속 수락 루프
    while (1) {
        client_len = sizeof(client_addr); // 매번 초기화 권장
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            perror("Accept failed");
            continue;
        }

        // 멀티프로세싱: 자식에게 처리 맡김
        if (fork() == 0) {
            close(server_fd);       // 자식은 리스닝 소켓 불필요
            handle_client(client_fd);
        }

        close(client_fd);           // 부모는 연결 소켓 닫고 다음 손님 대기
    }

    return 0;
}
