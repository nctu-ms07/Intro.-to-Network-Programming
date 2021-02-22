#include <bits/stdc++.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define INPUT_BUFFER_SIZE 1024

using namespace std;

unordered_map<string, string> tcp_upd;
unordered_map<string, pair<string, string>> user_data;
unordered_map<string, string> login_users;

string server_service(string ip_port, string line) {
  stringstream ss(line);
  string word;
  vector<string> words;
  while (getline(ss, word, ' ')) {
    words.emplace_back(word);
  }

  stringstream result;

  if (words[0] == "register") {
    if (user_data.find(words[1]) == user_data.end()) {
      user_data[words[1]] = make_pair(words[2], words[3]);
      result << "Register successfully." << endl;
    } else {
      result << "The user name has been used" << endl;
    }
  }

  if (words[0] == "login") {
    auto it = login_users.find(ip_port);
    if (it != login_users.end()) {
      result << "Please logout first." << endl;
    } else if (user_data.count(words[1]) == 1 && user_data[words[1]].second == words[2]) {
      result << "Welcome, " << words[1] << "." << endl;
      login_users[ip_port] = words[1];
    } else {
      result << "Login failed." << endl;
    }
  }

  if (words[0] == "logout") {
    auto it = login_users.find(ip_port);
    if (it != login_users.end()) {
      result << "Bye, " << it->second << "." << endl;
      login_users.erase(it);
    } else {
      result << "Please login first." << endl;
    }
  }

  if (words[0] == "whoami") {
    auto it = login_users.find(ip_port);
    if (it != login_users.end()) {
      result << it->second << endl;
    } else {
      result << "Please login first." << endl;
    }
  }

  if (words[0] == "list-user") {
    result << "Name    Email" << endl;
    for (auto it = user_data.begin(); it != user_data.end(); it++) {
      result << it->first << "   " << (it->second).first << endl;
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
    tcp_upd[tcp_ip_port] = "unknown";
    //sending tcp_ip_port to client
    sendto(pfd.fd, tcp_ip_port.c_str(), strlen(tcp_ip_port.c_str()), 0, &src_addr, src_len);
  }

  void handleInput(int c_fd) {
    sockaddr src_addr;
    socklen_t src_len = sizeof(sockaddr);
    getpeername(c_fd, &src_addr, &src_len); //tcp

    input_buffer = new char[INPUT_BUFFER_SIZE];
    int length = recvfrom(c_fd, input_buffer, INPUT_BUFFER_SIZE, 0, &src_addr, &src_len); //udp
    string ip_port = sockaddr_in_to_string((sockaddr_in * ) & src_addr);
    if (length) {
      input_buffer[length] = '\0';
      string msg(input_buffer);
      if (tcp_upd.count(msg)) { //linking tcp and udp
        tcp_upd[msg] = ip_port;
        cout << "[Server] client " << ip_port << " connected using UDP" << endl;
        cout << "[Server] linking " << msg << " and " << ip_port << " as same client" << endl;
      } else { // handle command sent from client
        cout << "[" << ip_port << "] " << msg << endl;
        if (tcp_upd.count(ip_port)) { //changing tcp to udp if possible
          ip_port = tcp_upd[ip_port];
        }
        string result = server_service(ip_port, msg);
        sendto(c_fd, result.c_str(), strlen(result.c_str()), 0, &src_addr, src_len);
      }
    } else { //client graceful disconnect
      cout << "[Server] client " << ip_port << " disconnected as TCP" << endl;
      cout << "[Server] client " << tcp_upd[ip_port] << " disconnected as UDP" << endl;
      login_users.erase(tcp_upd[ip_port]);
      tcp_upd.erase(ip_port);
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