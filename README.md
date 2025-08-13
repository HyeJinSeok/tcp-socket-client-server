# ◈ TCP Socket 기반 디렉토리 전송 클라이언트·서버

<br>

이 프로젝트는 C 언어와 POSIX 소켓 API를 이용해 구현한 **TCP 기반 파일 전송 프로그램**입니다. <br>

클라이언트는 지정한 디렉토리 내의 모든 파일을 읽어 서버에 전송하고, <br>

서버는 해당 디렉토리를 생성한 뒤 파일들을 저장합니다.

<br>

## 🔹실습 목표

<br>

- TCP 소켓 프로그래밍 구조(socket → bind → listen → accept → send/recv)를 이해한다.
  
- 클라이언트-서버 간 **데이터 송수신** 절차를 직접 구현한다.
  
- 파일 I/O와 네트워크 전송을 연계하는 프로그램을 작성한다.
  
- **저수준 네트워크 프로그래밍**을 통해 고수준 프레임워크(Spring, Tomcat 등) 내부 동작 기반을 이해한다.

<br>

## 🔹학습 시기


📆 2023년 10월~12월

<br><br>

## 🔹프로젝트 개요

<br>

### ① TCP 소켓이란?


• 네트워크 상에서 연결형(신뢰성 보장) 통신을 제공하는 **통로** <br>
• TCP는 데이터의 순서 보장, 오류 검출 및 재전송 기능을 제공함 <br>
• 소켓을 통해 프로그램 간 데이터를 안전하게 주고받을 수 있음

<img src="https://github.com/HyeJinSeok/tcp-socket-client-server/blob/main/assets/tcp_socket.png" alt="TCP 소켓" width="700">

<br>

### ② 소켓의 원리


• 소켓은 OS가 제공하는 네트워크 입출력 종단점(endpoint) 으로, <br>
• 프로세스가 네트워크를 통해 **바이트 데이터를 송수신**할 수 있도록 하는 표준 인터페이스 <br>

<img src="https://github.com/HyeJinSeok/tcp-socket-client-server/blob/main/assets/socket_step.png" alt="소켓 과정" width="500">


**< 서버-클라이언트 공통 >** <br>

- **socket( )** : 소켓(네트워크 통로)을 생성하고 소켓 식별자(FD)를 반환 
- **close( )** : 소켓을 닫음. TCP라면 FIN 전송로 연결 종료 <br><br>

**< 서버 전용 >** <br>

- **bind( )** : 소켓에 로컬 IP:PORT를 붙임 -> 주소와 결합 
- **listen( )** : 클라이언트의 연결 요청을 대기
- **accept( )** : 대기 중인 연결 요청을 수락하고, 새로운 연결용 소켓(새 FD)을 반환 <br><br>

**< 클라이언트 전용 >** <br>

- **connect( )** : 서버의 IP:PORT로 연결 요청을 보냄 (성공하면 이후 send/recv 가능) <br><br>

**< 데이터 송수신 >** <br>

- TCP(스트림, SOCK_STREAM) 
- **send( ) / recv( )** : 연결된 상대와 바이트 스트림을 주고받음 
- **write( ) / read( )** 도 쓸 수 있으나, 플래그 등 옵션 사용 제한적 <br><br>

> [!NOTE]
> **소켓 식별자 (File Descriptor, FD)** 는
> 프로세스 안에서 어느 소켓을 가리키는지 지정할 때 쓰임 <br>
> 정수 번호(예: 3, 4, 5…) 형태로, 로컬 **커널** 내부 식별자 역할

<br>

### ③ 이 프로젝트에서의 적용 방식


− TCP 소켓을 사용한 디렉토리 단위 파일 전송 <br>
− 경계 표시는 간단한 신호 (**ACK / EOF / EOT**)로 구분

<br>

**< 서버 측 >** <br>

- socket( ) → bind( ) → listen( ) → accept( )으로 클라이언트 연결을 수락
- 수락한 연결을 현재 프로세스에서 직접 처리 (단일 프로세스로 구현)
- 첫 메시지로 디렉토리 이름을 recv( )로 수신 → mkdir( ) 후 chdir( )하여 저장 위치(CWD)를 설정
- ACK 송신

- 이후 디렉토리 속 파일마다 다음을 반복:
  - recv( )로 파일 이름 수신 → ACK 송신
  - recv( )로 파일 내용을 계속 받으며 저장
  - 버퍼 앞부분이 "EOF" 로 들어오면 해당 파일 수신 종료 → ACK 송신
- 모든 파일이 끝나면 클라이언트가 보내는 "EOT" 를 받아 세션 종료로 처리하고 소켓을 닫음

<br>

**< 클라이언트 측 >** <br>

- socket( ) → connect( )로 서버에 연결
- 먼저 디렉토리 이름을 send( )로 전송
- 서버의 ACK 수신

- 디렉토리 내 파일을 순회하며, 각 파일에 대해:
  - 파일 이름 전송 → ACK 수신
  - 파일 내용 전송(끝까지)
  - "EOF" 전송 → ACK 수신
- 모든 파일 전송이 끝나면 "EOT" 를 전송하여 전송 종료를 알림

<br>

> [!IMPORTANT]
> Key information users need to know to achieve their goal.
