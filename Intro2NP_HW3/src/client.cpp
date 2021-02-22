#include <bits/stdc++.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>

#define INPUT_BUFFER_SIZE 1024

using namespace std;

sem_t chat_room_server_boot_lock;
sem_t chat_room_server_connect_lock;
sem_t chat_room_server_end_lock;
int server_port = -1;
string server_owner;
map<string, string> ip_port_user_name;
list<string> chat_record;

class ChatRoomServer {
 private:
  int s_tcpfd;
  sockaddr_in s_addr;

  bool listening = false;
  vector<pollfd> pollfds;
  char *input_buffer;

  void handleConnection() {
    pollfd pfd;
    pfd.fd = accept(s_tcpfd, nullptr, nullptr);
    pfd.events = POLL_IN;
    pfd.revents = 0;
    pollfds.emplace_back(pfd);
  }

  void handleInput(int c_fd) {
    time_t current = time(nullptr);
    tm *time_struct = localtime(&current);
    string time_info = '[' + to_string(time_struct->tm_hour) + ':' + to_string(time_struct->tm_min) + "] : ";

    sockaddr src_addr;
    socklen_t src_len = sizeof(sockaddr);
    getpeername(c_fd, &src_addr, &src_len); //tcp
    string ip_port = sockaddr_in_to_string((sockaddr_in * ) & src_addr);

    input_buffer = new char[INPUT_BUFFER_SIZE];
    int length = recv(c_fd, input_buffer, INPUT_BUFFER_SIZE, 0);
    if (length) {
      input_buffer[length] = '\0';
      string msg(input_buffer);

      if (ip_port_user_name.count(ip_port) == 0) { // client would first send their name before their first message
        ip_port_user_name[ip_port] = msg;
        if (server_owner.empty()) { //owner would always be the first one to login
          server_owner = msg;
        }
        string login_message = "*****************************\n";
        login_message += "** Welcome to the chatroom **\n";
        login_message += "*****************************\n";
        for (list<string>::reverse_iterator rit = chat_record.rbegin(); rit != chat_record.rend(); rit++) {
          login_message += *rit;
        }
        send(c_fd, login_message.c_str(), strlen(login_message.c_str()), 0);

        if (ip_port_user_name[ip_port] != server_owner) {
          string sys = "sys" + time_info + msg + " join us.\n";
          for (int i = 0; i < pollfds.size(); i++) {
            if (pollfds[i].fd != s_tcpfd && pollfds[i].fd != c_fd) {
              send(pollfds[i].fd, sys.c_str(), strlen(sys.c_str()), 0);
            }
          }
        }
      } else { // handle chat message
        msg = ip_port_user_name[ip_port] + time_info + msg + '\n';
        if (chat_record.size() == 3) {
          chat_record.pop_back();
        }
        chat_record.push_front(msg);
        for (int i = 0; i < pollfds.size(); i++) {
          if (pollfds[i].fd != s_tcpfd && pollfds[i].fd != c_fd) {
            send(pollfds[i].fd, msg.c_str(), strlen(msg.c_str()), 0);
          }
        }
      }
    } else { // client disconnected
      if (ip_port_user_name[ip_port] != server_owner) {
        string sys = "sys" + time_info + ip_port_user_name[ip_port] + " leave us.\n";
        for (int i = 0; i < pollfds.size(); i++) {
          if (pollfds[i].fd != s_tcpfd && pollfds[i].fd != c_fd) {
            send(pollfds[i].fd, sys.c_str(), strlen(sys.c_str()), 0);
          }
        }
      }

      ip_port_user_name.erase(ip_port);
      shutdown(c_fd, SHUT_RDWR);
      close(c_fd);
    }

    delete[] input_buffer;
  }

 public:
  ChatRoomServer(uint16_t port) {
    cout << "start to create chatroom…" << endl;
    sem_init(&chat_room_server_boot_lock, 0, 0);

    s_tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = INADDR_ANY;
    s_addr.sin_port = htons(port);

    int enable = 1;
    setsockopt(s_tcpfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    bind(s_tcpfd, (sockaddr * ) & s_addr, sizeof(s_addr));

    pollfd pfd;
    pfd.fd = s_tcpfd;
    pfd.events = POLL_IN;
    pfd.revents = 0;
    pollfds.emplace_back(pfd);
  }

  ~ChatRoomServer() {
    for (int i = 0; i < pollfds.size(); i++) {
      shutdown(pollfds[i].fd, SHUT_RDWR);
      close(pollfds[i].fd);
    }

    server_port = -1;
    server_owner.clear();
    ip_port_user_name.clear();
  }

  static string sockaddr_in_to_string(sockaddr_in *src_addr) {
    stringstream ss;
    ss << inet_ntoa(src_addr->sin_addr) << ":" << ntohs(src_addr->sin_port);
    return ss.str();
  }

  void stop_listening() {
    listening = false;
  }

  void listen_for_message() {
    listening = true;
    listen(s_tcpfd, 10);
    sem_post(&chat_room_server_boot_lock);

    while (listening) {
      poll(pollfds.data(), pollfds.size(), -1);

      for (int i = 0; i < pollfds.size(); i++) {
        if ((pollfds[i].revents & POLL_IN) == POLL_IN) {
          if (pollfds[i].fd == s_tcpfd) { // new tcp connection
            handleConnection();
            if (server_port == -1) {
              sem_post(&chat_room_server_connect_lock);
              server_port = ntohs(s_addr.sin_port);
            }
          } else { // client message from tcp protocol
            handleInput(pollfds[i].fd);
          }
        } else if (pollfds[i].revents != 0) { // other event happened, we tend to disconnect
          shutdown(pollfds[i].fd, SHUT_RDWR);
          close(pollfds[i].fd);
          pollfds.erase(pollfds.begin() + i);
        }
      }
    }

    time_t current = time(nullptr);
    tm *time_struct = localtime(&current);
    string time_info = '[' + to_string(time_struct->tm_hour) + ':' + to_string(time_struct->tm_min) + "] : ";

    string sys = "sys" + time_info + "the chatroom is close.\n";

    for (int i = 0; i < pollfds.size(); i++) {
      if (pollfds[i].fd != s_tcpfd) {
        send(pollfds[i].fd, sys.c_str(), strlen(sys.c_str()), 0);
      }
    }

    sem_post(&chat_room_server_end_lock);
  }

};

bool isValid(string cmd) {
  if (cmd.empty()) {
    return false;
  }
  stringstream ss(cmd);
  string word;
  vector<string> words;
  while (getline(ss, word, ' ')) {
    words.emplace_back(word);
  }

  if (words[0] == "register") {
    if (words.size() != 4) {
      cout << "Command format: register <username> <email> <password>" << endl;
      return false;
    }
    return true;
  }

  if (words[0] == "login") {
    if (words.size() != 3) {
      cout << "Command format: login <username> <password>>" << endl;
      return false;
    }
    return true;
  }

  if (words[0] == "logout") {
    if (words.size() != 1) {
      cout << "Command format: logout" << endl;
      return false;
    }
    return true;
  }

  if (words[0] == "whoami") {
    if (words.size() != 1) {
      cout << "Command format: whoami" << endl;
      return false;
    }
    return true;
  }

  if (words[0] == "list-user") {
    if (words.size() != 1) {
      cout << "Command format: list-user" << endl;
      return false;
    }
    return true;
  }

  if (words[0] == "exit") {
    if (words.size() != 1) {
      cout << "Command format: exit" << endl;
      return false;
    }
    return true;
  }

  if (words[0] == "create-board") {
    if (words.size() != 2) {
      cout << "Command format: create-board <name>" << endl;
      return false;
    }
    return true;
  }

  if (words[0] == "create-post") {
    if (words.size() < 6) {
      cout << "Command format: create-post <board-name> --title <title> --content <content>" << endl;
      return false;
    }

    string title;
    string content;

    smatch match;
    if (regex_search(cmd, match, regex("(--title )(.*)( --content)"))
        || regex_search(cmd, match, regex("(--title )(.*)"))) {
      title = match.str(2);
    }

    if (regex_search(cmd, match, regex("(--content )(.*)( --title)"))
        || regex_search(cmd, match, regex("(--content )(.*)"))) {
      content = match.str(2);
    }

    if (title.empty() || content.empty()) {
      cout << "Command format: create-post <board-name> --title <title> --content <content>" << endl;
      return false;
    }

    return true;
  }

  if (words[0] == "list-board") {
    if (words.size() != 1) {
      cout << "Command format: list-board" << endl;
      return false;
    }
    return true;
  }

  if (words[0] == "list-post") {
    if (words.size() != 2) {
      cout << "Command format: list-post <board-name>" << endl;
      return false;
    }
    return true;
  }

  if (words[0] == "read") {
    if (words.size() != 2) {
      cout << "Command format: read <post-S/N>" << endl;
      return false;
    }
    return true;
  }

  if (words[0] == "delete-post") {
    if (words.size() != 2) {
      cout << "Command format: delete-post <post-S/N>" << endl;
      return false;
    }
    return true;
  }

  if (words[0] == "update-post") {
    if (words.size() < 4) {
      cout << "Command format: update-post <post-S/N> --title/content <new>" << endl;
      return false;
    }

    string title;
    string content;

    smatch match;
    if (regex_search(cmd, match, regex("(--title )(.*)( --content)"))
        || regex_search(cmd, match, regex("(--title )(.*)"))) {
      title = match.str(2);
    }

    if (regex_search(cmd, match, regex("(--content )(.*)( --title)"))
        || regex_search(cmd, match, regex("(--content )(.*)"))) {
      content = match.str(2);
    }

    if (title.empty() && content.empty()) {
      cout << "Command format: update-post <post-S/N> --title/content <new>" << endl;
      return false;
    }

    return true;
  }

  if (words[0] == "comment") {
    if (words.size() < 3) {
      cout << "Command format: comment <post-S/N> <comment>" << endl;
      return false;
    }
    return true;
  }

  if (words[0] == "create-chatroom") {
    if (words.size() != 2) {
      cout << "Command format: create-chatroom <port>" << endl;
      return false;
    }
    return true;
  }

  if (words[0] == "list-chatroom") {
    if (words.size() != 1) {
      cout << "Command format: list-chatroom" << endl;
      return false;
    }
    return true;
  }

  if (words[0] == "join-chatroom") {
    if (words.size() != 2) {
      cout << "Command format: join-chatroom <chatroom_name>" << endl;
      return false;
    }
    return true;
  }

  if (words[0] == "attach") {
    if (words.size() != 1) {
      cout << "Command format: attach" << endl;
      return false;
    }
    return true;
  }

  if (words[0] == "restart-chatroom") {
    if (words.size() != 1) {
      cout << "Command format: restart-chatroom" << endl;
      return false;
    }
    return true;
  }

  cout << "Command not found." << endl;
  return false;
}

class Client {
 private:
  int c_tcpfd, c_udpfd;
  sockaddr_in s_addr;

  ChatRoomServer *chat_room_server;

  pthread_t chat_room_server_thread_id;
  pthread_t recv_echo_thread_id;

  bool chatroom_mode = false;
  int chatroom_tcpfd;
  sockaddr_in chatroom_addr;
  string chat_input;

  bool create_chatroom = false;
  bool connect_chatroom = false;
  vector<pollfd> pollfds;
  char *input_buffer;

  void listen_for_message() {
    poll(pollfds.data(), pollfds.size(), -1);

    for (int i = 0; i < pollfds.size(); i++) {
      if ((pollfds[i].revents & POLL_IN) == POLL_IN) {
        input_buffer = new char[INPUT_BUFFER_SIZE];
        int length = recv(pollfds[i].fd, input_buffer, INPUT_BUFFER_SIZE, 0);
        input_buffer[length] = '\0';

        if (create_chatroom) {
          stringstream parser(input_buffer);
          string user_name;
          getline(parser, user_name, ' ');
          string port;
          getline(parser, port, ' ');

          for (char c : port) {
            if (!isdigit(c)) {
              create_chatroom = false;
              break;
            }
          }

          if (create_chatroom) {
            create_chatroom = false;
            createChatRoom("127.0.0.1", stoul(port), user_name);
            length = recv(pollfds[i].fd, input_buffer, INPUT_BUFFER_SIZE, 0);
            input_buffer[length] = '\0';
          }
        }

        if (connect_chatroom) {
          stringstream parser(input_buffer);
          string user_name;
          getline(parser, user_name, ' ');
          string port;
          getline(parser, port, ' ');

          for (char c : port) {
            if (!isdigit(c)) {
              connect_chatroom = false;
              break;
            }
          }

          if (connect_chatroom) {
            connect_chatroom = false;
            connectChatRoom("127.0.0.1", stoul(port), user_name);
            length = recv(pollfds[i].fd, input_buffer, INPUT_BUFFER_SIZE, 0);
            input_buffer[length] = '\0';
          }
        }

        cout << input_buffer;
        delete[] input_buffer;
      } else if (pollfds[i].revents != 0) { // other event happened, we tend to disconnect
        cout << "[Client] disconnected from server" << endl;
        shutdown(pollfds[i].fd, SHUT_RDWR);
        close(pollfds[i].fd);
        pollfds.erase(pollfds.begin() + i);
      }
    }
  }

  void sendMessage(string msg) {
    if (regex_match(msg, regex("^register.*")) || msg == "whoami" || msg == "list-chatroom") {
      sendto(c_udpfd, msg.c_str(), strlen(msg.c_str()), 0, (sockaddr * ) & s_addr, sizeof(s_addr));
    } else {
      sendto(c_tcpfd, msg.c_str(), strlen(msg.c_str()), 0, (sockaddr * ) & s_addr, sizeof(s_addr));
    }
  }

  static void *chat_room_server_thread(void *arg) {
    ChatRoomServer *server = (ChatRoomServer *) arg;
    server->listen_for_message();
  }

  static void *recv_echo_thread(void *arg) {
    Client *client = (Client *) arg;
    char thread_input_buffer[INPUT_BUFFER_SIZE];
    int length;
    while ((length = recv(client->chatroom_tcpfd, thread_input_buffer, INPUT_BUFFER_SIZE, 0))) {
      thread_input_buffer[length] = '\0';
      if (client->chatroom_mode) {
        cout << thread_input_buffer;
      } else {
        break;
      }
    }

    if (client->chatroom_mode) { // chatroom server closed
      client->chatroom_mode = false;
      shutdown(client->chatroom_tcpfd, SHUT_RDWR);
      close(client->chatroom_tcpfd);
      client->sendMessage("detach");
    }
  }

  void createChatRoom(string ip, uint16_t port, string owner) {
    chatroom_mode = true;
    chat_room_server = new ChatRoomServer(port);
    pthread_create(&chat_room_server_thread_id, nullptr, Client::chat_room_server_thread, chat_room_server);
    sem_wait(&chat_room_server_boot_lock);

    chatroom_tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    chatroom_addr.sin_family = AF_INET;
    inet_aton(ip.c_str(), &(chatroom_addr.sin_addr));
    chatroom_addr.sin_port = htons(port);

    sem_init(&chat_room_server_connect_lock, 0, 0);
    connect(chatroom_tcpfd, (sockaddr * ) & chatroom_addr, sizeof(chatroom_addr));
    sem_wait(&chat_room_server_connect_lock);
    pthread_create(&recv_echo_thread_id, nullptr, Client::recv_echo_thread, this);
    send(chatroom_tcpfd, owner.c_str(), strlen(owner.c_str()), 0);

    sem_destroy(&chat_room_server_boot_lock);
    sem_destroy(&chat_room_server_connect_lock);

    while (chatroom_mode) {
      getline(cin, chat_input);

      if (chatroom_mode) {
        if (chat_input == "detach") {
          chatroom_mode = false;
          shutdown(chatroom_tcpfd, SHUT_RDWR);
          close(chatroom_tcpfd);
          sendMessage("detach");
          chat_input.clear();
          continue;
        }

        if (chat_input == "leave-chatroom") {
          chatroom_mode = false;
          sem_init(&chat_room_server_end_lock, 0, 0);
          chat_room_server->stop_listening();
          shutdown(chatroom_tcpfd, SHUT_RDWR);
          close(chatroom_tcpfd);
          sem_wait(&chat_room_server_end_lock);
          sem_destroy(&chat_room_server_end_lock);
          delete chat_room_server;
          sendMessage("leave-chatroom");
          chat_input.clear();
          continue;
        }

        send(chatroom_tcpfd, chat_input.c_str(), strlen(chat_input.c_str()), 0);
        chat_input.clear();
      }
    }
  }

  void connectChatRoom(string ip, uint16_t port, string user) {
    chatroom_mode = true;

    chatroom_tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    chatroom_addr.sin_family = AF_INET;
    inet_aton(ip.c_str(), &(chatroom_addr.sin_addr));
    chatroom_addr.sin_port = htons(port);

    connect(chatroom_tcpfd, (sockaddr * ) & chatroom_addr, sizeof(chatroom_addr));
    pthread_create(&recv_echo_thread_id, nullptr, Client::recv_echo_thread, this);
    send(chatroom_tcpfd, user.c_str(), strlen(user.c_str()), 0);

    bool owner = (port == server_port);

    while (chatroom_mode) {
      getline(cin, chat_input);

      if (chatroom_mode) {
        if (chat_input == "leave-chatroom") {
          if (owner) {
            chatroom_mode = false;
            sem_init(&chat_room_server_end_lock, 0, 0);
            chat_room_server->stop_listening();
            shutdown(chatroom_tcpfd, SHUT_RDWR);
            close(chatroom_tcpfd);
            sem_wait(&chat_room_server_end_lock);
            sem_destroy(&chat_room_server_end_lock);
            delete chat_room_server;
            sendMessage("leave-chatroom");
          } else {
            chatroom_mode = false;
            shutdown(chatroom_tcpfd, SHUT_RDWR);
            close(chatroom_tcpfd);
            sendMessage("detach");
          }
          chat_input.clear();
          continue;
        }

        if (chat_input == "detach" && owner) {
          chatroom_mode = false;
          shutdown(chatroom_tcpfd, SHUT_RDWR);
          close(chatroom_tcpfd);
          sendMessage("detach");
          chat_input.clear();
          continue;
        }

        send(chatroom_tcpfd, chat_input.c_str(), strlen(chat_input.c_str()), 0);
        chat_input.clear();
      }
    }
  }

 public:
  Client(string ip, uint16_t port) {
    c_tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    c_udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    s_addr.sin_family = AF_INET;
    inet_aton(ip.c_str(), &(s_addr.sin_addr));
    s_addr.sin_port = htons(port);

    connect(c_tcpfd, (sockaddr * ) & s_addr, sizeof(s_addr));
    //sending tcp_ip_port to server using udp
    input_buffer = new char[INPUT_BUFFER_SIZE];
    int length = recv(c_tcpfd, input_buffer, INPUT_BUFFER_SIZE, 0);
    input_buffer[length] = '\0';
    sendto(c_udpfd, input_buffer, length, 0, (sockaddr * ) & s_addr, sizeof(s_addr));
    delete[] input_buffer;

    pollfd pfd;
    pfd.fd = c_tcpfd;
    pfd.events = POLL_IN;
    pfd.revents = 0;
    pollfds.emplace_back(pfd);
    pfd.fd = c_udpfd;
    pollfds.emplace_back(pfd);
  }

  ~Client() {
    shutdown(c_tcpfd, SHUT_RDWR);
    shutdown(c_udpfd, SHUT_RDWR);
    close(c_tcpfd);
    close(c_udpfd);
  }

  void service_loop() {
    cout << "********************************" << endl;
    cout << "** Welcome to the BBS server. **" << endl;
    cout << "********************************" << endl;

    string input;
    while (true) {
      cout << "% ";

      if (!chat_input.empty()) {
        if (!isValid(chat_input)) {
          chat_input.clear();
          continue;
        }

        if (chat_input == "exit") {
          // TODO chatroom server clean up
          shutdown(c_tcpfd, SHUT_RDWR);
          shutdown(c_udpfd, SHUT_RDWR);
          close(c_tcpfd);
          close(c_udpfd);
          chat_input.clear();
          break;
        }

        if (regex_match(chat_input, regex("^create-chatroom.*")) || chat_input == "restart-chatroom") {
          create_chatroom = true;
        }

        if (regex_match(chat_input, regex("^join-chatroom.*")) || chat_input == "attach") {
          connect_chatroom = true;
        }

        sendMessage(chat_input);
        chat_input.clear();
        listen_for_message();
        continue;
      }

      getline(cin, input);

      if (!isValid(input)) {
        continue;
      }

      if (input == "exit") {
        // TODO chatroom server clean up
        shutdown(c_tcpfd, SHUT_RDWR);
        shutdown(c_udpfd, SHUT_RDWR);
        close(c_tcpfd);
        close(c_udpfd);
        break;
      }

      if (regex_match(input, regex("^create-chatroom.*")) || input == "restart-chatroom") {
        create_chatroom = true;
      }

      if (regex_match(input, regex("^join-chatroom.*")) || input == "attach") {
        connect_chatroom = true;
      }

      sendMessage(input);
      listen_for_message();
    }
  }
};

int main(int argc, char **argv) {
  if (argc == 3) {
    Client client(string(argv[1]), atoi(argv[2]));
    client.service_loop();
  }
}