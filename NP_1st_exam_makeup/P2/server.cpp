#include <bits/stdc++.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/wait.h>

#define INPUT_BUFFER_SIZE 1024

using namespace std;

unordered_map<string, string> user_name_ip_port;
unordered_map<string, string> ip_port_user_name;

string server_service(string ip_port, string line) {
  stringstream ss(line);
  string word;
  vector<string> words;
  while (getline(ss, word, ' ')) {
    words.emplace_back(word);
  }

  stringstream result;

  if (words[0] == "list-users") {
    for (auto it = user_name_ip_port.begin(); it != user_name_ip_port.end(); it++) {
      result << it->first << " " << it->second << endl;
    }
  }

    if (words[0] == "sort-users") {
    for (auto it = user_name_ip_port.begin(); it != user_name_ip_port.end(); it++) {
      result << it->first << " " << it->second << endl;
    }
  }

  return result.str();
}

class Server {
 private:
  int s_tcpfd, s_udpfd;
  sockaddr_in s_addr;

  bool listening = false;
  vector<pollfd> pollfds;
  char *input_buffer;

  vector<pthread_t> thread_pool;
  vector<int *> thread_arg_pool;

  void handleConnection() {
    sockaddr src_addr;
    socklen_t src_len = sizeof(src_addr);

    int *c_fd_p = new int;
    *c_fd_p = accept(s_tcpfd, &src_addr, &src_len);
    string tcp_ip_port = sockaddr_in_to_string((sockaddr_in * ) & src_addr);
    cout << "New Connection from " << tcp_ip_port << endl;

    pthread_t thread_id;
    pthread_create(&thread_id, nullptr, Server::tcp_service_thread, c_fd_p);
    thread_pool.emplace_back(thread_id);
    thread_arg_pool.emplace_back(c_fd_p);

    cout << "[Server] Create tcp service thread for client " << tcp_ip_port << endl;
  }

  static void *tcp_service_thread(void *c_fd_p) {
    int c_fd = *((int *) c_fd_p);

    sockaddr src_addr;
    socklen_t src_len = sizeof(sockaddr);

    getpeername(c_fd, &src_addr, &src_len); //tcp
    string ip_port = sockaddr_in_to_string((sockaddr_in * ) & src_addr);

    int length;
    char thread_input_buffer[INPUT_BUFFER_SIZE];
    while ((length = recv(c_fd, thread_input_buffer, INPUT_BUFFER_SIZE, 0))) {
      thread_input_buffer[length] = '\0';
      string msg(thread_input_buffer);

      string result;

      if(ip_port_user_name.count(ip_port) == 0){
        if(user_name_ip_port.count(msg) == 1){
          result = "The username is already used!\n";
        }else{
          user_name_ip_port[msg] = ip_port;
          ip_port_user_name[ip_port] = msg;
          result = "Welcome, " + msg + ".\n";
        }
      }else {
        result = server_service(ip_port, msg);
      }

      cout << "[" << ip_port << "] " << msg << endl;
      send(c_fd, result.c_str(), strlen(result.c_str()), 0);
    }

    //client graceful disconnect
    cout << ip_port_user_name[ip_port] << " " << ip_port << " disconnected" << endl;
    user_name_ip_port.erase(ip_port_user_name[ip_port]);
    ip_port_user_name.erase(ip_port);
    shutdown(c_fd, SHUT_RDWR);
    close(c_fd);

    cout << "[Server] Exit tcp service thread for client " << ip_port << endl;
  }

  void handleInput(int c_fd) {
  }

 public:
  Server(uint16_t port) {
    s_tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    s_udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = INADDR_ANY;
    s_addr.sin_port = htons(port);

    bind(s_tcpfd, (sockaddr * ) & s_addr, sizeof(s_addr));
    bind(s_udpfd, (sockaddr * ) & s_addr, sizeof(s_addr));
    cout << "[Server] hosting on " << inet_ntoa(s_addr.sin_addr) << ", port " << ntohs(s_addr.sin_port) << endl;

    pollfd pfd;
    pfd.fd = s_tcpfd;
    pfd.events = POLL_IN;
    pfd.revents = 0;
    pollfds.emplace_back(pfd);
    pfd.fd = s_udpfd;
    pollfds.emplace_back(pfd);
  }
  ~Server() {
    for (int *&element : thread_arg_pool) {
      delete element;
    }

    for (pollfd p_fd : pollfds) {
      shutdown(p_fd.fd, SHUT_RDWR);
      close(p_fd.fd);
    }
    cout << "[Server] shutdown" << endl;
  }

  static string sockaddr_in_to_string(sockaddr_in *src_addr) {
    stringstream ss;
    ss << inet_ntoa(src_addr->sin_addr) << ":" << ntohs(src_addr->sin_port);
    return ss.str();
  }

  void listen_for_message() {
    listening = true;
    listen(s_tcpfd, 10);

    while (listening) {
      poll(pollfds.data(), pollfds.size(), -1);

      for (int i = 0; i < pollfds.size(); i++) {
        if ((pollfds[i].revents & POLL_IN) == POLL_IN) {
          if (pollfds[i].fd == s_tcpfd) { // new tcp connection
            handleConnection();
          } else { // recv from client using udp, shouldn't happened in this case.
            handleInput(pollfds[i].fd);
          }
        } else if (pollfds[i].revents != 0) { // other event happened, we tend to disconnect
          shutdown(pollfds[i].fd, SHUT_RDWR);
          close(pollfds[i].fd);
          pollfds.erase(pollfds.begin() + i);
        }
      }
    }
  }
};

int main(int argc, char *argv[]) {
  if (argc == 2) {
    Server server(atoi(argv[1]));
    server.listen_for_message();
  }
}