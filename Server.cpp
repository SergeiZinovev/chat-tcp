
#include <iostream>
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#define NUMBER_OF_THREADS 100

// thread identifiers
pthread_t tid[NUMBER_OF_THREADS];

int Connections[100];
int Counter = 0;
std::vector <std::string> users;

//the current thread descriptor
struct thread_args {
    int index;
};

//receive and send out messages
void* ClientHandler(void* args) {
    thread_args* arg = (thread_args*)args;
    int msg_size;

    while (true) {
        //receive message size
        int connected = recv(Connections[arg->index], (char*)&msg_size, sizeof(int), 0);

        if(!connected) {
            std::cout << "Client Disconnected\n";
            users[arg->index] = "DISC";
            close(Connections[arg->index]);
            return args;
        }

        char* msg = new char[msg_size + 1];
        msg[msg_size] = '\0';

        //receive a user message
        recv(Connections[arg->index], msg, msg_size, 0);

        for (int i = 0; i < Counter; i++) {
            if (i == arg->index) continue;

            //send the sender's name
            int name_size  = sizeof(users[arg->index]);
            send(Connections[i], (char*)&name_size, sizeof(int), 0);           
            send(Connections[i], users[arg->index].c_str(), name_size, 0);

            //send a message to other users
            send(Connections[i], (char*)&msg_size, sizeof(int), 0);
            send(Connections[i], msg, msg_size, 0);
        }

        delete[] msg;
    }
}

int main()
{

    //add information about the socket address
    sockaddr_in addr;
    socklen_t sizeofaddr = sizeof(addr);
    addr.sin_addr.s_addr = inet_addr("127.0.6.1"); 
    addr.sin_port = htons(1234); 
    addr.sin_family = AF_INET;

    //Create a socket
    int sListen = socket(AF_INET, SOCK_STREAM, 0);

    //bind address to socket
    bind(sListen, (sockaddr*)&addr, sizeof(addr));

    //listen to the port
    listen(sListen, SOMAXCONN); //SOMAXCONN - defines the maximum queue size for sockets in fully and not fully established connection states

    //create a new socket to hold the connection
    int newConnection;

    //waiting for new connections
    for (int i = 0; i < 100; i++) {

        //accept a new connection
        newConnection = accept(sListen, (sockaddr*)&addr, &sizeofaddr);

        if (newConnection == 0) {
            std::cout << "Error " << std::endl;
        }
        else {

            std::cout << "Client Connected!" << std::endl;
            char hello_msg[] = "Welcome to the chat room. \nEnter your name: ";
            
            //send a welcome message
            send(newConnection, hello_msg, sizeof(hello_msg), 0);

            //receive the username
            int size_name;
            recv(newConnection, (char*)&size_name, sizeof(int), 0);

            char* user_name = new char[size_name + 1];
            user_name[size_name] = '\0';
            recv(newConnection, user_name, size_name, 0);
            
            //a line containing all online users at the moment
            std::string all_users;

            if (users.empty()) {
                all_users = "No active users at the moment\n";
            }
            else {
                all_users = "Online users: ";
                for (auto name : users) {
                    if (name == "DISC") continue;
                    all_users += name + " ";
                }
            }

            //send a line with current users
            int size_all_users = all_users.length();
            send(newConnection, (char*)&size_all_users, sizeof(int), 0);
            send(newConnection, all_users.c_str(), size_all_users, 0);

            //add a new user to the list
            users.push_back(user_name);

            //memorize the connection
            Connections[i] = newConnection;
            Counter++;

            //notify other users about the new member
            for (int i = 0; i < Counter; i++) {
                if (i == Counter - 1) continue;

                //send the sender's name
                int name_size = sizeof(user_name);
                send(Connections[i], (char*)&size_name, sizeof(int), 0);
                send(Connections[i], user_name, size_name, 0);

                //send a welcome message
                char msg_new_user[] = "Let's start the conversation!";
                int msg_new_user_size = sizeof(msg_new_user);
                send(Connections[i], (char*)&msg_new_user_size, sizeof(int), 0);
                send(Connections[i], msg_new_user, sizeof(msg_new_user), 0);

            }


            //prepare arguments for creating a flow
            thread_args* arg = new thread_args;
            arg->index = i;
            
            //create a thread to receive and send messages from the current user
            int err = pthread_create(&tid[Counter], NULL, ClientHandler, arg);
            if (err !=0 ) {
                std::cerr << "Can't create thread" << std::endl;
            }

        }
    }

    system("pause");
    return 0;
}

