#include <bits/stdc++.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define INPUT_BUFFER_SIZE 1024

using namespace std;

bool isValid(string cmd) {
  if (cmd.size() == 0) {
    return false;
  }
  stringstream ss(cmd);
  string word;
  vector<string> words;
  while (getline(ss, word, ' ')) {
    words.emplace_back(word);
  }

  if (words[0] == "show-accounts") {
    if (words.size() != 1) {
      cout << "Command format: show-accounts" << endl;
      return false;
    }
    return true;
  }

  if (words[0] == "deposit") {
    if (words.size() != 3) {
      cout << "Command format: deposit <account> <money>" << endl;
      return false;
    }else{
      if(stoi(words[2]) <= 0){
        cout << "Deposit a non-positive number into accounts." << endl;
        return false;
      }
    }
    return true;
  }

  if (words[0] == "withdraw") {
    if (words.size() != 3) {
      cout << "Command format: withdraw <account> <money>" << endl;
      return false;
    }else{
      if(stoi(words[2]) <= 0){
        cout << "withdraw a non-positive number into accounts." << endl;
        return false;
      }
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

  cout << "Command not found." << endl;
  return false;
}

class Client {
 private:
  int c_tcpfd, c_udpfd;
  sockaddr_in s_addr;

  vector<pollfd> pollfds;
  char *input_buffer;

  void listen_for_message() {
    poll(pollfds.data(), pollfds.size(), -1);

    for (int i = 0; i < pollfds.size(); i++) {
      if ((pollfds[i].revents & POLL_IN) == POLL_IN) {
        input_buffer = new char[INPUT_BUFFER_SIZE];
        int length = recv(pollfds[i].fd, input_buffer, INPUT_BUFFER_SIZE, 0);
        input_buffer[length] = '\0';
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
    send(c_tcpfd, msg.c_str(), strlen(msg.c_str()), 0);
  }

 public:
  Client(string ip, uint16_t port) {
    c_tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    c_udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    s_addr.sin_family = AF_INET;
    inet_aton(ip.c_str(), &(s_addr.sin_addr));
    s_addr.sin_port = htons(port);

    connect(c_tcpfd, (sockaddr * ) & s_addr, sizeof(s_addr));

    pollfd pfd;
    pfd.fd = c_tcpfd;
    pfd.events = POLL_IN;
    pfd.revents = 0;
    pollfds.push_back(pfd);
    pfd.fd = c_udpfd;
    pollfds.push_back(pfd);
  }

  ~Client() {
    shutdown(c_tcpfd, SHUT_RDWR);
    shutdown(c_udpfd, SHUT_RDWR);
    close(c_tcpfd);
    close(c_udpfd);
  }

  void service_loop() {
    cout << "********************************" << endl;
    cout << "** Welcome to the TCP server. **" << endl;
    cout << "********************************" << endl;

    string input;
    while (true) {
      cout << "% ";
      getline(cin, input);

      if (!isValid(input)) {
        continue;
      }

      if (input == "exit") {
        shutdown(c_tcpfd, SHUT_RDWR);
        shutdown(c_udpfd, SHUT_RDWR);
        close(c_tcpfd);
        close(c_udpfd);
        break;
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