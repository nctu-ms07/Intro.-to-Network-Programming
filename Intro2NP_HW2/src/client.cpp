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
    if (regex_match(msg, regex("^register.*")) || msg == "whoami") {
      sendto(c_udpfd, msg.c_str(), strlen(msg.c_str()), 0, (sockaddr * ) & s_addr, sizeof(s_addr));
    } else {
      sendto(c_tcpfd, msg.c_str(), strlen(msg.c_str()), 0, (sockaddr * ) & s_addr, sizeof(s_addr));
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