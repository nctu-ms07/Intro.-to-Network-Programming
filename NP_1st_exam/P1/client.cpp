#include <bits/stdc++.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define INPUT_BUFFER_SIZE 1024

using namespace std;

int file_fd = -1;
int request_amount = 0;
bool receive_mode = false;

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

  if (words[0] == "get-file-list") {
    if (words.size() != 1) {
      cout << "Command format: get-file-list" << endl;
      return false;
    }
    return true;
  }

  if (words[0] == "get-file") {
    if (words.size() == 1) {
      cout << "Command format: get-file <file-name1> <file-name2>" << endl;
      return false;
    }
    request_amount = words.size() - 1;
    receive_mode = true;
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

  void listen_for_message() {
    poll(pollfds.data(), pollfds.size(), -1);

    for (int i = 0; i < pollfds.size(); i++) {
      if ((pollfds[i].revents & POLL_IN) == POLL_IN) {
        input_buffer = new char[INPUT_BUFFER_SIZE];
        int length = recv(pollfds[i].fd, input_buffer, INPUT_BUFFER_SIZE, 0);
        input_buffer[length] = '\0';

        if (!receive_mode) {
          cout << input_buffer;
        } else {
          while (receive_mode) {
            stringstream ss((string(input_buffer)));
            string file_name;
            getline(ss, file_name, ' ');

            if (file_fd == -1) {
              file_fd = open(file_name.c_str(), O_CREAT | O_TRUNC | O_WRONLY, S_IWUSR | S_IRUSR);
            }

            if (file_name.size() == length) {
              close(file_fd);
              file_fd = -1;
              request_amount--;
              if (request_amount == 0) {
                receive_mode = false;
                break;
              }
            } else {
              char *write_buffer = new char [length - file_name.size() - 1];
              memcpy(write_buffer, input_buffer + file_name.size() + 1, length - file_name.size() -1);
              write(file_fd, write_buffer, length - file_name.size() - 1);
              delete[] write_buffer;
            }

            length = recv(pollfds[i].fd, input_buffer, INPUT_BUFFER_SIZE, 0);
          }
        }

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
    cout << "********************************" << endl;
    cout << "** Welcome to the BBS server. **" << endl;
    cout << "********************************" << endl;

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