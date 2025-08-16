// 디렉토리 내 파일들을 TCP로 서버에 전송하는 클라이언트

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 2030
#define MAX_BUFFER_SIZE 1024
#define END_OF_FILE "EOF"          
#define END_OF_TRANSFER "EOT"      

// send_file 함수
void send_file(int sock, const char* file_path) {
    char buffer[MAX_BUFFER_SIZE];

    // 전송할 파일 열기 (바이너리 모드)
    FILE* file = fopen(file_path, "rb");
    
  if (!file) {
        perror("Error opening file");
        return;
    }

    // 파일 경로에서 파일명만 추출
    const char* filename = strrchr(file_path, '/');
    filename = filename ? filename + 1 : file_path;

    // (1) 파일 이름 전송
    send(sock, filename, strlen(filename), 0);

    // 서버 ACK 수신
    recv(sock, buffer, sizeof(buffer), 0);

    // (2) 파일 내용 전송
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(sock, buffer, bytes_read, 0);
    }

    // (3) 파일 종료 토큰 전송
    send(sock, END_OF_FILE, strlen(END_OF_FILE), 0);

    // 파일 닫기
    fclose(file);

    // 서버의 파일 수신 완료 ACK 수신
    recv(sock, buffer, sizeof(buffer), 0);
}
// send_file 함수 끝


// send_directory 함수
void send_directory(int sock, const char* dir_path) {
    char buffer[MAX_BUFFER_SIZE];

    const char* dname = strrchr(dir_path, '/');
    dname = dname ? dname + 1 : dir_path;

    // (0) 디렉토리 이름 전송
    send(sock, dname, strlen(dname), 0);

    // 서버 ACK 수신
    recv(sock, buffer, sizeof(buffer), 0);

    // 로컬 디렉토리 열기
    DIR* dir = opendir(dir_path);
    if (!dir) {
        perror("Error opening directory");
        exit(EXIT_FAILURE); // 디렉토리를 열 수 없으면 치명적 에러로 종료
    }

    // 디렉토리 엔트리 순회
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
      
        // 정규 파일만 전송 (DT_REG)
        if (entry->d_type == DT_REG) {
            // 실제 읽을 파일 경로: <dir_path>/<파일명>
            snprintf(buffer, sizeof(buffer), "%s/%s", dir_path, entry->d_name);
            send_file(sock, buffer); // 파일 하나 전송
        }
    }

    closedir(dir);

    // (마지막) 전체 전송 종료 알림
    send(sock, END_OF_TRANSFER, strlen(END_OF_TRANSFER), 0);
}
// send_directory 함수 끝


// 메인 함수
int main(int argc, char* argv[]) {
    // 인자 확인: 서버 IP, 전송할 디렉토리 경로
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char* server_ip = argv[1];
    const char* directory = argv[2];

    // TCP 소켓 생성
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 구성
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),             // 호스트 바이트 오더 → 네트워크 바이트 오더
        .sin_addr.s_addr = inet_addr(server_ip) // 문자열 IP → 32비트 주소(단순 버전)
    };

    // 서버로 연결 시도
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // 디렉토리 전송(디렉토리명 → 파일들 순서대로)
    send_directory(sock, directory);

    // 소켓 종료
    close(sock);
    return 0;
}
