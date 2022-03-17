#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <cstdlib>
#include <ctime>
#include <list>
#include <cstring>
#include <unistd.h>
#include <set>

using std::string;
using std::cin;
using std::cout;
using std::endl;
using std::set;

enum
{
    SERVER_PORT = 7009,
    NICK_LEN = 64,
    QUEUE_LEN = 5,
    STD_IN = 0,
    MSG_SIZE = 2048,
};

int OPTVAL = 1;

int users[QUEUE_LEN];

class Client
{
public:
    string Nickname;
    set<string> Privates;

} Clients[QUEUE_LEN];

char msg_buf[MSG_SIZE];
int sListen;

int
firstFreeID(const int connections[])
{
    for(int i = 0; i < QUEUE_LEN; i++)
        if (connections[i] == -1)
            return i;
    return -1;
}

void
initialization(int connections[])
{
    for (int i = 0; i < QUEUE_LEN; i++)
        connections[i] = -1;
}

void
nullptrForCstr(char *str)
{
    for (int _ = 0; _ < strlen(str); _++) {
        if (str[_] == '\n' || str[_] == '\r') {
            str[_] = 0;
            break;
        }
    }
}
int
spacesInNames(const char buf[])
{
    for (int i = 0; (buf[i] != '\0') && (buf[i] != '\n') && (buf[i] != '\r'); i++)
        if (buf[i] == ' ')
            return i;
    return 0;
}

int
isCorrectName(const char buf[])
{
    if ((strlen(buf) > NICK_LEN) || (spacesInNames(buf) > 0) ||
        (buf[0] == '\0') || (buf[0] == '\n') || (buf[0] == '\r') ||
        (buf[0] == '\\') || (buf[0] == '/') || (buf[0] == '*'))
        return 0;
    string nick(buf);
    for(const auto& client : Clients) {
        if (client.Nickname == nick) {
            return 0;
        }
    }

    return 1;
}

void
atAll(const char buf[], int usr_d)
{
    for (auto usr : users) {
        if (usr == usr_d)
            continue;
        send(usr, buf, MSG_SIZE, 0);
    }
}

void
SigHandler(int s)
{
    shutdown(sListen, 2);
    close(sListen);
    string gb =  "Server is offed. Good Bye!";
    atAll(gb.c_str(), sListen);
    exit(0);
}

void
nickHandler(char *buf, int usr_d, int id)
{
    cout << "I am here because nickname field is empty" << endl;   // FLAG
    if (isCorrectName(buf)) {
        cout << "I am here because name is correct" << endl;   // FLAG
        string std_str(buf);
        Clients[id].Nickname = std_str;
        cout << "His name is " << Clients[id].Nickname << endl; // report for admin

        string acquaintance = "                              *SERVER: There is a new user on our server \"Server\": " + Clients[id].Nickname; // report for users
        atAll(acquaintance.c_str(), usr_d);
    }
    else {
        string err = "                              "
                     "*SERVER: Error! Your nickname is incorrect or is taken by another user.                                   \n"
                     "Try again.";
        send(usr_d, err.c_str(), MSG_SIZE, 0);
    }
}

void
cmdHandler(char buf[], int usr_d, int id)
{
    string message(buf);
    string cmd = message.substr(1);
    string cmd_first;
    for (int i = 1; buf[i] != '\n' && buf[i] != '\0' && buf[i] != '\r' && buf[i] != ' '; i++) {
        cmd_first.push_back(buf[i]);

    }
    cout << cmd_first << cmd << endl;
    if (cmd_first != "help" && cmd_first != "users" && cmd_first != "privates" && cmd_first != "private" && cmd_first != "quit") {
        string warning = "                              *SERVER: Unknown command.";
        send(usr_d, warning.c_str(), MSG_SIZE, 0);
    }
    else {
        if (cmd == "help") {
            string help_msg = "The list of actual commands: ";
            send(usr_d, help_msg.c_str(), MSG_SIZE, 0);
            string help_list = "*********************************\n"
                               "* \\help                         *\n"
                               "* \\users                        *\n"
                               "* \\privates                     *\n"
                               "* \\private <nickname> <message> *\n"
                               "* \\quit <message>               *\n"
                               "*********************************";

            send(usr_d, help_list.c_str(), MSG_SIZE, 0);

        } else if (cmd == "users") {
            string intro = "The list of registrated users: ";
            send(usr_d, intro.c_str(), MSG_SIZE, 0);
            for (const auto &client: Clients) {
                string name = client.Nickname;
                string star_name;
                if (!name.empty()) {
                    star_name = "* " + name + "\0";
                    send(usr_d, star_name.c_str(), MSG_SIZE, 0);
                }
            }
        } else if (cmd == "privates") {
            if (Clients[id].Privates.empty()) {
                string warning = "You don't have private conversations yet.\nPlease, use '\\private <nickaname> <message>' to start chatting personally.";
                send(usr_d, warning.c_str(), MSG_SIZE, 0);
            } else {
                for (const auto &name: Clients[id].Privates) {
                    string star_name = "* " + name;
                    send(usr_d, star_name.c_str(), MSG_SIZE, 0);
                }
            }
        }

        string cmd_quit = message.substr(1, 4);
        if (cmd_quit == "quit") {
            string msg_quit = message.substr(6);
            string full_msg = "<" + Clients[id].Nickname + ">$ " + msg_quit +
                              "\n                              *SERVER: This is " + Clients[id].Nickname +
                              "'s last message.";
            cout << full_msg << endl;
            cout << "hi!" << endl; // FLAG
            atAll(full_msg.c_str(), usr_d);

            shutdown(usr_d, 2);
            close(usr_d);

            string good_bye;
            if (Clients[id].Nickname.empty()) {
                good_bye = "The mysterious stranger left the chat.";
            } else {
                good_bye = Clients[id].Nickname + " left the chat.";
            }
            cout << good_bye << endl;
            string gb_for_users = "                              *SERVER: " + good_bye;
            atAll(gb_for_users.c_str(), users[id]);

            Clients[id].Nickname.clear();
            Clients[id].Privates.clear();
            users[id] = -1;

        }
        if (message.size() > 9) {
            string cmd_private = message.substr(1, 7);
            if (cmd_private == "private") {
                string nick_and_msg_private = message.substr(9);
                int k = spacesInNames(nick_and_msg_private.c_str());
                string nick_private = nick_and_msg_private.substr(0, k);
                string msg_private = nick_and_msg_private.substr(k + 1);
                int flag = 0;
                string msg_to_send;
                if (nick_private == Clients[id].Nickname) {
                    msg_to_send = "                              *SERVER: What are you doing?";
                    send(users[id], msg_to_send.c_str(), MSG_SIZE, 0);
                } else {
                    for (int i = 0; i < QUEUE_LEN; i++) {
                        if (nick_private == Clients[i].Nickname) {
                            flag = 1;
                            msg_to_send = "<" + Clients[id].Nickname + "(pers)>$ " + msg_private;
                            send(users[i], msg_to_send.c_str(), MSG_SIZE, 0);
                            Clients[id].Privates.insert(Clients[i].Nickname);
                            Clients[i].Privates.insert(Clients[id].Nickname);
                        }
                    }
                    if (flag == 0) {
                        string warning = "                              *SERVER: There is no user with this nickname.\n"
                                         "                              Use \\users to get the list of users.";
                        send(usr_d, warning.c_str(), MSG_SIZE, 0);
                    }
                }

            }
        }
    }
}

void
msgHandler(char *buf, int usr_d, int id, Client* clients)
{
    nullptrForCstr(buf);
    cout << "I am in the msgHandler" << endl;   // FLAG
    if (buf == nullptr) {
        string err = "                              *SERVER: Error! Empty string!\n";
        send(usr_d, err.c_str(), MSG_SIZE, 0);
        return;
    }

    if (buf[0] == '\\') { // command
        cout << "I am here because command arrived" << endl; // FLAG
        cmdHandler(buf, usr_d, id);
    } else if (clients[id].Nickname.empty()) { // nickname
        nickHandler(buf, usr_d, id);
    } else { // common message
        cout << "I am here because the message is not a nickname" << endl;   // FLAG
        string msg_for_sending = "<" + Clients[id].Nickname + ">$ " + buf;
        atAll(msg_for_sending.c_str(), usr_d);
        cout << buf << endl;
    }
}

int
main()
{
    struct sockaddr_in server_address = sockaddr_in{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;
    int server_address_size = sizeof(server_address);

    if ((sListen = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Error! Failed to create socket." << endl;
    }
    if (setsockopt(sListen, SOL_SOCKET, SO_REUSEADDR, &OPTVAL, sizeof(OPTVAL)) < 0) {
        std::cerr << "Error! Failed to setsockopt()." << endl;
    }
    if ((bind(sListen, (struct sockaddr *) &server_address, sizeof(server_address))) < 0) {
        std::cerr << "Error! Failed to bind socket to address." << endl;
    }

    if (listen(sListen, QUEUE_LEN) < 0) {
        std::cerr << "Error! Failed to listen." << endl;
    }

    cout << "Server started." << endl;

    initialization(users);

    signal(SIGINT, SigHandler);
    signal(SIGTERM, SigHandler);
    signal(SIGSTOP, SigHandler);

    for (;;) {
        fd_set readfds;
        int max_d = sListen; // Suppose, the maximum is the No. of the listening socket

        FD_ZERO(&readfds); // Clearing the set
        FD_SET(sListen, &readfds); // Adding the descriptor of the listening socket to the set

        FD_SET(STD_IN, &readfds);

        // Creating a loop through client sockets
        for (auto user : users) {
            if (user != -1) {
                FD_SET(user, &readfds); // Adding client's descriptor in the set
                if (user > max_d)
                    max_d = user;
            }
        }

        int res = select(max_d+1,
                         &readfds,
                         nullptr, nullptr, nullptr);
        if (res < 1) {
            std::cerr << "Error! Failed to select()." << endl;
            exit(1);
        }

        if (FD_ISSET(sListen, &readfds)) {
            int new_connection = accept(sListen,
                                        (struct sockaddr *) (&server_address),
                                        (socklen_t *) &server_address_size);

            (new_connection != 0 ?
                cout << "Client connected!" << endl :
                std::cerr << "Error! Failed to accept().") << endl;

            string greet = "             ________________________________________________\n"
                           "            /                                                \\\n"
                           "           |    _________________________________________     |\n"
                           "           |   |                                         |    |\n"
                           "           |   |                                         |    |\n"
                           "           |   |                                         |    |\n"
                           "           |   |  *SERVER: Hello, Client!                |    |\n"
                           "           |   |   This is my server \"Server\"!           |    |\n"
                           "           |   |   Type \\help for list of commands.      |    |\n"
                           "           |   |   Your first message without \\          |    |\n"
                           "           |   |    will be your name on the server.     |    |\n"
                           "           |   |                                         |    |\n"
                           "           |   |                                         |    |\n"
                           "           |   |_________________________________________|    |\n"
                           "           |                                                  |\n"
                           "            \\_________________________________________________/\n"
                           "                   \\___________________________________/\n"
                           "                ___________________________________________\n"
                           "             _-'    .-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.  --- `-_\n"
                           "          _-'.-.-. .---.-.-.-.-.-.-.-.-.-.-.-.-.-.-.--.  .-.-.`-_\n"
                           "       _-'.-.-.-. .---.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-`__`. .-.-.-.`-_\n"
                           "    _-'.-.-.-.-. .-----.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-----. .-.-.-.-.`-_\n"
                           " _-'.-.-.-.-.-. .---.-. .-------------------------. .-.---. .---.-.-.-.`-_\n"
                           ":-------------------------------------------------------------------------:\n"
                           "`---._.-------------------------------------------------------------._.---'";

            send(new_connection, greet.c_str(), MSG_SIZE, 0);

            int k = firstFreeID(users);
            if(k >= 0) {
                users[k] = new_connection;
            } else {
                cout << "Queue is full!" << endl;
            }
        }

        if (FD_ISSET(STD_IN, &readfds)) { // Users will see other's messages
            msg_buf[0] = 0;
            read(STD_IN, msg_buf, MSG_SIZE);
            string msg_buf_std(msg_buf);
            msg_buf_std.substr(0, 5);
            string msg_buf_first;
            for (int i = 0; i < 5; i++) {
                msg_buf_first.push_back(msg_buf[i]);
            }

            cout << msg_buf_first << endl; // FLAG
            if (msg_buf_first == "\\exit\n" || msg_buf_first == "\\exit") {
                shutdown(sListen, 2);
                close(sListen);
                string gb =  "Server is offed. GB!";
                atAll(gb.c_str(), sListen);
                exit(0);
            }
            //cout << msg_buf << endl;
        }

        for (int i = 0; i < QUEUE_LEN; i++) { // Catching user's message
            if (users[i] != -1) {
                if (FD_ISSET(users[i], &readfds)) {
                    msg_buf[0] = 0;
                    int id = i;

                    if (recv(users[id], msg_buf, MSG_SIZE, 0) > 0) {
                        msgHandler(msg_buf, users[id], id, Clients);
                    } else {
                        string error = "                              *SERVER: Something goes wrong...";
                        send(users[id], error.c_str(), MSG_SIZE, 0);
                        shutdown(users[id], 2);
                        close(users[id]);

                        string good_bye;
                        if (Clients[id].Nickname.empty()) {
                            good_bye = "The mysterious stranger left the chat.";
                        } else {
                            good_bye = Clients[id].Nickname + " left the chat.";
                        }
                        cout << good_bye << endl;
                        string gb_for_users  = "                              *SERVER: " + good_bye;
                        atAll(gb_for_users.c_str(), users[id]);

                        Clients[id].Nickname.clear();
                        Clients[id].Privates.clear();
                        users[id] = -1;

                        continue;
                    }

                }
            }
        }

    }
    return 0;
}