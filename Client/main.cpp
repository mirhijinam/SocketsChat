#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <cstdlib>
#include <ctime>
#include <list>
#include <cstring>
#include <unistd.h>

using std::cout;
using std::endl;
using std::cin;
using std::string;

enum
{
    SERVER_PORT = 7009,
    MSG_SIZE = 2048,
    STD_IN = 0
};

char msg_buf[MSG_SIZE];
int usr_sock;

void
SigHandler(int a)
{
    shutdown(usr_sock, 2);
    close(usr_sock);
    cout << "Disconnecting" << endl;
    sleep(1);
    cout << "..." << endl;
    sleep(1);
    cout << "..." << endl;
    sleep(1);
    cout << "..." << endl;
    exit(0);
}

int
main()
{
    struct sockaddr_in server_address = sockaddr_in{};
    int server_address_size = sizeof(server_address);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;


    if ((usr_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Error! Failed to create socket." << endl;
    }

    if (connect(usr_sock,
                (struct sockaddr *) &server_address,
                server_address_size) != 0) {
        std::cerr << "Error! Failed to connect." << endl;
        return 1;
    }
    sleep(1);
    cout << "..." << endl;
    sleep(1);
    cout << "..." << endl;
    sleep(1);
    cout << "..." << endl;
    sleep(1);
    cout << "Connected!" << endl;
    sleep(1);

    signal(SIGINT, SigHandler);
    signal(SIGTERM, SigHandler);

    for(;;) {
        fd_set readfds;

        int max_d = usr_sock;

        FD_ZERO(&readfds);
        FD_SET(usr_sock, &readfds);

        FD_SET(STD_IN, &readfds);

        int res =  select(max_d+1, &readfds, 0, 0, 0);
        if (res == -1) {
            std::cerr << "Failed to select()." << endl;
        }

        if (FD_ISSET(usr_sock, &readfds)) {
            if (recv(usr_sock, msg_buf, MSG_SIZE, 0) <= 0) {
                shutdown(usr_sock, 2);
                close(usr_sock);
                cout << "Disconnecting" << endl;
                sleep(1);
                cout << "..." << endl;
                sleep(1);
                cout << "..." << endl;
                sleep(1);
                cout << "..." << endl;
                exit(0);
            }
            cout << msg_buf << endl;
            msg_buf[0] = 0;
        }

        if (FD_ISSET(STD_IN, &readfds)) {
            msg_buf[0] = 0;
            read(STD_IN, msg_buf, MSG_SIZE);
            send(usr_sock, msg_buf, MSG_SIZE, 0);
        }
    }

    return 0;
}