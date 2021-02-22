#include <bits/stdc++.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>

#define INPUT_BUFFER_SIZE 1024

using namespace std;

sem_t operation_lock;

int account1 = 0;
int account2 = 0;

int user_count = 0;

unordered_map<string, char> tcp_username;

string server_service(string line) {
  stringstream ss(line);
  string word;
  vector<string> words;
  while (getline(ss, word, ' ')) {
    words.emplace_back(word);
  }

  stringstream result;

  if (words[0] == "show-accounts") {
    result << "ACCOUNT1 " << account1 << endl;
    result << "ACCOUNT2 " << account2 << endl;
  }

  if (words[0] == "deposit") {
    if(words[1] == "ACCOUNT1"){
      sem_wait(&operation_lock);
      account1 += stoi(words[2]);
      result << "Successfully deposits "  << words[2] << " into ACCOUNT1." << endl;
      sem_post(&operation_lock);
    }

    if(words[1] == "ACCOUNT2"){
      sem_wait(&operation_lock);
      account2 += stoi(words[2]);
      result << "Successfully deposits "  << words[2] << " into ACCOUNT2." << endl;
      sem_post(&operation_lock);
    }
  }

  if(words[0] == "withdraw"){
    if(words[1] == "ACCOUNT1"){
      if(stoi(words[2]) > account1){
        result << "Withdraw excess money from accounts." << endl;
      }else{
        sem_wait(&operation_lock);
        account1 -= stoi(words[2]);
        result << "Successfully withdraws " << words[2] << " from ACCOUNT1." << endl;
        sem_post(&operation_lock);
      }
      
    }

    if(words[1] == "ACCOUNT2"){
      if(stoi(words[2]) > account2){
        result << "Withdraw excess money from accounts." << endl;
      }else{
        sem_wait(&operation_lock);
        account2 -= stoi(words[2]);
        result << "Successfully withdraws " << words[2] << " from ACCOUNT2." << endl;
        sem_post(&operation_lock);
      }
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
    char name = 'A' + user_count++;
    tcp_username[tcp_ip_port] = name;
    cout << "New connection from " << tcp_ip_port << " user" << name << endl;
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
      string msg(input_buffer);
      string result = server_service(msg);
      send(c_fd, result.c_str(), strlen(result.c_str()), 0);
    } else { //client graceful disconnect
      cout << "user" << tcp_username[ip_port] << " " << ip_port << " disconnected" << endl;
      tcp_username.erase(ip_port);
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

    sem_init(&operation_lock, 0 ,0);
    sem_post(&operation_lock);
  }
  ~Server() {
    for (pollfd p_fd : pollfds) {
      shutdown(p_fd.fd, SHUT_RDWR);
      close(p_fd.fd);
    }
    sem_destroy(&operation_lock);
    cout << "[Server] shutdown" << endl;
  }

  string sockaddr_in_to_string(sockaddr_in *src_addr) {
    stringstream ss;
    ss << inet_ntoa(src_addr->sin_addr) << ":" << ntohs(src_addr->sin_port);
    return ss.str();
  }

  void listen_for_message() {
    listening = true;
    listen(s_tcpfd, 4);

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