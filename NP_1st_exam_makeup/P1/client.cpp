#include <bits/stdc++.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define INPUT_BUFFER_SIZE 1024

using namespace std;

vector<string> files_to_transfer;

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

  if (words[0] == "send-file") {
    if (words.size() == 1) {
      cout << "Command format: send-file <file-name1> <file-name2>" << endl;
      return false;
    }

    for (int i = 1; i < words.size(); i++) {
      files_to_transfer.emplace_back(words[i]);
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
  int c_udpfd;
  sockaddr_in s_addr;

  vector<pollfd> pollfds;
  char *input_buffer;

  void sendMessage(string msg) {
    sendto(c_udpfd, msg.c_str(), strlen(msg.c_str()), 0, (sockaddr * ) & s_addr, sizeof(s_addr));
  }

 public:
  Client(string ip, uint16_t port) {
    c_udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    s_addr.sin_family = AF_INET;
    inet_aton(ip.c_str(), &(s_addr.sin_addr));
    s_addr.sin_port = htons(port);

    pollfd pfd;
    pfd.fd = c_udpfd;
    pfd.events = POLL_IN;
    pfd.revents = 0;
    pollfds.push_back(pfd);
  }

  ~Client() {
    shutdown(c_udpfd, SHUT_RDWR);
    close(c_udpfd);
  }

  void service_loop() {
    string input;
    while (true) {
      cout << "% ";
      getline(cin, input);

      if (!isValid(input)) {
        continue;
      }

      if (input == "exit") {
        shutdown(c_udpfd, SHUT_RDWR);
        close(c_udpfd);
        break;
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
          sendto(c_udpfd, transfer_data, file_name.size() + 1 + read_length, 0, (sockaddr * ) & s_addr, sizeof(s_addr));
          delete[] transfer_data;
        }
        sendto(c_udpfd, file_name.c_str(), strlen(file_name.c_str()), 0, (sockaddr * ) & s_addr, sizeof(s_addr));
        delete[] read_buffer;
        close(file_fd);
        files_to_transfer.pop_back();
      }
    }
  }
};

int main(int argc, char **argv) {
  if (argc == 3) {
    Client client(string(argv[1]), atoi(argv[2]));
    client.service_loop();
  }
}