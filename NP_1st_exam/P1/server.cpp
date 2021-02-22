#include <bits/stdc++.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>

#define INPUT_BUFFER_SIZE 1024

using namespace std;

vector<string> files_to_transfer;

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
      cout << msg << endl;
      delete[] input_buffer;

      stringstream ss(msg);
      string word;
      vector<string> words;
      while (getline(ss, word, ' ')) {
        words.emplace_back(word);
      }

      if (words[0] == "get-file-list") {
        stringstream result;

        result << "Files: ";

        struct dirent **namelist;
        int n;
        n = scandir(".", &namelist, 0, alphasort);
        while (n--) {
          if (namelist[n]->d_type == DT_REG) {
            result << namelist[n]->d_name << ' ';
          }
          free(namelist[n]);
        }
        free(namelist);

        result << endl;
        sendto(c_fd, result.str().c_str(), strlen(result.str().c_str()), 0, (sockaddr * ) & src_addr, sizeof(src_addr));
        return;
      }

      if (words[0] == "get-file") {
        for (int i = 1; i < words.size(); i++) {
          files_to_transfer.emplace_back(words[i]);
        }
      }

      while (!files_to_transfer.empty()) {
        string file_name = files_to_transfer.back();
        int file_fd = open(file_name.c_str(), O_CREAT | O_RDONLY, S_IWUSR | S_IRUSR);
        int read_length;
        char *read_buffer = new char[INPUT_BUFFER_SIZE];
        while ((read_length = read(file_fd, read_buffer, 512))) {
          char *transfer_data = new char[file_name.size() + 1 + read_length];
          memcpy(transfer_data, file_name.c_str(), file_name.size());
          memset(transfer_data + file_name.size(), ' ', 1);
          memcpy(transfer_data + file_name.size() + 1, read_buffer, read_length);
          sendto(c_fd, transfer_data, file_name.size() + 1 + read_length, 0, (sockaddr * ) & src_addr, sizeof(src_addr));
          delete[] transfer_data;
        }
        sendto(c_fd, file_name.c_str(), strlen(file_name.c_str()), 0, (sockaddr * ) & src_addr, sizeof(src_addr));
        delete[] read_buffer;
        close(file_fd);
        files_to_transfer.pop_back();
      }

    } else {
      shutdown(c_fd, SHUT_RDWR);
      close(c_fd);
    }
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