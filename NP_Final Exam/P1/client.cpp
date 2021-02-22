#include <bits/stdc++.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>

#define INPUT_BUFFER_SIZE 1024

using namespace std;

bool mute = false;

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

  if (words[0] == "yell") {
    if (words.size() <= 1) {
      cout << "Command format: register yell <message>" << endl;
      return false;
    }
    return true;
  }

  if (words[0] == "tell") {
    if (words.size() <= 2) {
      cout << "Command format: register yell <receiver> <message>" << endl;
      return false;
    }
    return true;
  }

  if (words[0] == "unmute") {
    if (words.size() != 1) {
      cout << "Command format: unmute" << endl;
      return false;
    }else{
      if(!mute){
        cout << "You are already in unmute mode." << endl;
        return false;
      }
    }
    mute = false;
    cout << "Unmute mode." << endl;
    return false;
  }

  if (words[0] == "mute") {
    if (words.size() != 1) {
      cout << "Command format: mute" << endl;
      return false;
    }else{
      if(mute){
        cout << "You are already in mute mode." << endl;
        return false;
      }
    }
    mute = true;
    cout << "Mute mode." << endl;
    return false;
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

  pthread_t recv_echo_thread_id;

  vector<pollfd> pollfds;
  char *input_buffer;

  void sendMessage(string msg) {
    send(c_tcpfd, msg.c_str(), strlen(msg.c_str()), 0);
  }

  static void *recv_echo_thread(void *arg) {
    Client *client = (Client *) arg;
    char thread_input_buffer[INPUT_BUFFER_SIZE];
    int length;
    while ((length = recv(client->c_tcpfd, thread_input_buffer, INPUT_BUFFER_SIZE, 0))) {
      thread_input_buffer[length] = '\0';
      if (!mute) {
        cout << thread_input_buffer;
      }
    }

    shutdown(client->c_tcpfd, SHUT_RDWR);
    close(client->c_tcpfd);
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

    // receive welcome message from server
    input_buffer = new char[INPUT_BUFFER_SIZE];
    int length = recv(c_tcpfd, input_buffer, INPUT_BUFFER_SIZE, 0);
    input_buffer[length] = '\0';
    cout << input_buffer;
    pthread_create(&recv_echo_thread_id, nullptr, Client::recv_echo_thread, this);
    delete[] input_buffer;

    string input;
    while (true) {
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
    }
  }
};

int main(int argc, char **argv) {
  if (argc == 3) {
    Client client(string(argv[1]), atoi(argv[2]));
    client.service_loop();
  }
}