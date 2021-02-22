#include <bits/stdc++.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define INPUT_BUFFER_SIZE 1024

using namespace std;

int user_name_counter = 0;
unordered_map<string, string> ip_port_username;
unordered_map<string, int> username_fd;

class Server {
 private:
  int s_tcpfd, s_udpfd;
  sockaddr_in s_addr;

  bool listening = false;
  vector<pollfd> pollfds;
  char *input_buffer;

  void handleConnection() {
    sockaddr src_addr;
    socklen_t src_len = sizeof(src_addr);
    pollfd pfd;
    pfd.fd = accept(s_tcpfd, &src_addr, &src_len);
    pfd.events = POLL_IN;
    pfd.revents = 0;
    pollfds.emplace_back(pfd);
    string tcp_ip_port = sockaddr_in_to_string((sockaddr_in * ) & src_addr);
    cout << "[Server] client " << tcp_ip_port << " connected using TCP" << endl;
    string user_name = "user" + to_string(user_name_counter++);
    ip_port_username[tcp_ip_port] = user_name;
    username_fd[user_name] = pfd.fd;
    string welcome = "Welcome, " + user_name + ".\n";
    // sending welcome message to client
    sendto(pfd.fd, welcome.c_str(), strlen(welcome.c_str()), 0, &src_addr, src_len);
  }

  void handleInput(int c_fd) {
    sockaddr src_addr;
    socklen_t src_len = sizeof(sockaddr);
    getpeername(c_fd, &src_addr, &src_len); //tcp

    input_buffer = new char[INPUT_BUFFER_SIZE];
    int length = recv(c_fd, input_buffer, INPUT_BUFFER_SIZE, 0);
    string ip_port = sockaddr_in_to_string((sockaddr_in * ) & src_addr);
    if (length) {
      input_buffer[length] = '\0';
      string tmp(input_buffer);
      stringstream ss(tmp);
      string word;
      vector<string> words;
      while (getline(ss, word, ' ')) {
        words.emplace_back(word);
      }

      string result;

      if(words[0] == "tell"){
        if(username_fd.count(words[1]) == 0){
          result = words[1] + " does not exist\n";
          send(c_fd, result.c_str(), strlen(result.c_str()), 0);
        }else{
          result = words[1] + " told you:";
          for(int i = 2; i < words.size(); i++){
            result += " " + words[i];
          }
          result += '\n';
          send(username_fd[words[1]], result.c_str(), strlen(result.c_str()), 0);
        }
      }

      if(words[0] == "yell"){
        result = ip_port_username[ip_port] + ":";
        for(int i = 1; i < words.size(); i++){
            result += " " + words[i];
        }
        result += '\n';
        for(auto it = username_fd.begin(); it != username_fd.end(); it++){
          if(it->second != c_fd){
            send(it->second, result.c_str(), strlen(result.c_str()), 0);
          }
        }
      }
    } else { //client graceful disconnect
      cout << "[Server] client " << ip_port << " disconnected as TCP" << endl;
      username_fd.erase(ip_port_username[ip_port]);
      ip_port_username.erase(ip_port);
      shutdown(c_fd, SHUT_RDWR);
      close(c_fd);
    }
    delete[] input_buffer;
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
    for (pollfd p_fd : pollfds) {
      shutdown(p_fd.fd, SHUT_RDWR);
      close(p_fd.fd);
    }
    cout << "[Server] shutdown" << endl;
  }

  string sockaddr_in_to_string(sockaddr_in *src_addr) {
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
          } else { // recv from client
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