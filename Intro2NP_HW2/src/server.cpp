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

unordered_map<string, string> tcp_upd;
unordered_map<string, pair<string, string>> user_data;
unordered_map<string, string> login_users;

int post_serial_number = 1;
map<string, string> board_owner;
map<int, string> post_board;
map<int, pair<int, char * >> post_map;

string server_service(string ip_port, string line) {
  stringstream ss(line);
  string word;
  vector<string> words;
  while (getline(ss, word, ' ')) {
    words.emplace_back(word);
  }

  stringstream result;

  if (words[0] == "register") {
    if (user_data.count(words[1]) == 1) {
      result << "The user name has been used" << endl;
    } else {
      user_data[words[1]] = make_pair(words[2], words[3]);
      result << "Register successfully." << endl;
    }
  }

  if (words[0] == "login") {
    if (login_users.count(ip_port) == 0) {
      if (user_data.count(words[1]) == 1 && user_data[words[1]].second == words[2]) {
        login_users[ip_port] = words[1];
        result << "Welcome, " << words[1] << "." << endl;
      } else {
        result << "Login failed." << endl;
      }
    } else {
      result << "Please logout first." << endl;
    }
  }

  if (words[0] == "logout") {
    if (login_users.count(ip_port) == 1) {
      result << "Bye, " << login_users[ip_port] << "." << endl;
      login_users.erase(ip_port);
    } else {
      result << "Please login first." << endl;
    }
  }

  if (words[0] == "whoami") {
    if (login_users.count(ip_port) == 1) {
      result << login_users[ip_port] << endl;
    } else {
      result << "Please login first." << endl;
    }
  }

  if (words[0] == "list-user") {
    result << left << setw(10) << "Name" << "Email" << endl;
    for (auto it = user_data.begin(); it != user_data.end(); it++) {
      result << setw(10) << it->first << (it->second).first << endl;
    }
  }

  if (words[0] == "create-board") {
    if (login_users.count(ip_port) == 1) {
      if (board_owner.count(words[1]) == 0) {
        board_owner[words[1]] = login_users[ip_port];
        result << "Create board successfully." << endl;
      } else {
        result << "Board already exists." << endl;
      }
    } else {
      result << "Please login first." << endl;
    }
  }

  if (words[0] == "create-post") {

    if (login_users.count(ip_port) == 1) {
      if (board_owner.count(words[1]) == 1) {
        string title;
        string content;

        smatch match;
        if (regex_search(line, match, regex("(--title )(.*)( --content)"))
            || regex_search(line, match, regex("(--title )(.*)"))) {
          title = match.str(2);
        }

        if (regex_search(line, match, regex("(--content )(.*)( --title)"))
            || regex_search(line, match, regex("(--content )(.*)"))) {
          content = match.str(2);
        }

        time_t current = time(nullptr);
        tm *time_struct = localtime(&current);
        int month = time_struct->tm_mon + 1;
        int day = time_struct->tm_mday;
        string date = to_string(month) + "/" + to_string(day);

        string post = login_users[ip_port] + "\n" + title + "\n" + date + "\n" + content + "\n";

        int shmid = shmget(0, strlen(post.c_str()), IPC_CREAT | 0660);
        char *shmchar = (char *) shmat(shmid, NULL, 0);
        post.copy(shmchar, strlen(post.c_str()));

        post_board[post_serial_number] = words[1];
        post_map[post_serial_number] = make_pair(shmid, shmchar);

        post_serial_number++;

        result << "Create post successfully." << endl;
      } else {
        result << "Board does not exist." << endl;
      }
    } else {
      result << "Please login first." << endl;
    }
  }

  if (words[0] == "list-board") {
    result << left << setw(7) << "Index" << setw(16) << "Name" << "Moderator" << endl;
    int index = 1;
    for (auto it = board_owner.begin(); it != board_owner.end(); it++) {
      result << setw(7) << index++ << setw(16) << it->first << it->second << endl;
    }
  }

  if (words[0] == "list-post") {
    if (board_owner.count(words[1]) == 1) {
      result << left << setw(5) << "S/N" << setw(30) << "Title" << setw(12) << "Author" << "Date" << endl;
      for (auto it = post_board.begin(); it != post_board.end(); it++) {
        if (it->second != words[1]) {
          continue;
        }
        int post_id = it->first;
        stringstream post;
        post << post_map[post_id].second;
        string author;
        string title;
        string date;
        getline(post, author, '\n');
        getline(post, title, '\n');
        getline(post, date, '\n');
        result << setw(5) << post_id << setw(30) << title << setw(12) << author << date << endl;
      }
    } else {
      result << "Board does not exist." << endl;
    }
  }

  if (words[0] == "read") {
    if (post_map.count(stoi(words[1])) == 1) {
      stringstream post;
      post << post_map[stoi(words[1])].second;
      string author;
      string title;
      string date;
      string content;
      string comment;
      getline(post, author, '\n');
      getline(post, title, '\n');
      getline(post, date, '\n');
      getline(post, content, '\n');
      getline(post, comment, '\n');
      result << "Author: " << author << endl;
      result << "Title: " << title << endl;
      result << "Date: " << date << endl;
      result << "--" << endl;
      result << regex_replace(content, regex("<br>"), "\n") << endl;
      result << "--" << endl;
      if (!comment.empty()) {
        result << regex_replace(comment, regex(" <br> "), "\n") << endl;
      }
    } else {
      result << "Post does not exist." << endl;
    }
  }

  if (words[0] == "delete-post") {
    if (login_users.count(ip_port) == 1) {
      if (post_map.count(stoi(words[1])) == 1) {
        stringstream post;
        post << post_map[stoi(words[1])].second;
        string author;
        getline(post, author, '\n');

        if (author == login_users[ip_port]) {
          shmctl(post_map[stoi(words[1])].first, IPC_RMID, nullptr);
          shmdt(post_map[stoi(words[1])].second);
          post_board.erase(stoi(words[1]));
          post_map.erase(stoi(words[1]));
          result << "Delete successfully." << endl;
        } else {
          result << "Not the post owner." << endl;
        }
      } else {
        result << "Post does not exist." << endl;
      }
    } else {
      result << "Please login first." << endl;
    }
  }

  if (words[0] == "update-post") {
    if (login_users.count(ip_port) == 1) {
      if (post_map.count(stoi(words[1])) == 1) {
        stringstream post;
        post << post_map[stoi(words[1])].second;
        string author;
        string title;
        string date;
        string content;
        string comment;
        getline(post, author, '\n');
        getline(post, title, '\n');
        getline(post, date, '\n');
        getline(post, content, '\n');
        getline(post, comment, '\n');

        if (author == login_users[ip_port]) {
          shmctl(post_map[stoi(words[1])].first, IPC_RMID, nullptr);
          shmdt(post_map[stoi(words[1])].second);
          post_map.erase(stoi(words[1]));

          smatch match;
          if (regex_search(line, match, regex("(--title )(.*)( --content)"))
              || regex_search(line, match, regex("(--title )(.*)"))) {
            title = match.str(2);
          }

          if (regex_search(line, match, regex("(--content )(.*)( --title)"))
              || regex_search(line, match, regex("(--content )(.*)"))) {
            content = match.str(2);
          }

          string updated_post = author + "\n" + title + "\n" + date + "\n" + content + "\n" + comment;

          int shmid = shmget(0, strlen(updated_post.c_str()), IPC_CREAT | 0660);
          char *shmchar = (char *) shmat(shmid, NULL, 0);
          updated_post.copy(shmchar, strlen(updated_post.c_str()));

          post_map[stoi(words[1])] = make_pair(shmid, shmchar);

          result << "Update successfully." << endl;
        } else {
          result << "Not the post owner." << endl;
        }
      } else {
        result << "Post does not exist." << endl;
      }
    } else {
      result << "Please login first." << endl;
    }
  }

  if (words[0] == "comment") {
    if (login_users.count(ip_port) == 1) {
      if (post_map.count(stoi(words[1])) == 1) {
        stringstream post;
        post << post_map[stoi(words[1])].second;
        string author;
        string title;
        string date;
        string content;
        string comment;
        getline(post, author, '\n');
        getline(post, title, '\n');
        getline(post, date, '\n');
        getline(post, content, '\n');
        getline(post, comment, '\n');

        if (!comment.empty()) {
          comment += " <br> ";
        }

        smatch match;
        if (regex_search(line, match, regex("(comment \\d+ )(.*)"))) {
          comment += login_users[ip_port] + ":" + match.str(2);
        }

        string updated_post = author + "\n" + title + "\n" + date + "\n" + content + "\n" + comment;

        shmctl(post_map[stoi(words[1])].first, IPC_RMID, nullptr);
        shmdt(post_map[stoi(words[1])].second);
        post_map.erase(stoi(words[1]));

        int shmid = shmget(0, strlen(updated_post.c_str()), IPC_CREAT | 0660);
        char *shmchar = (char *) shmat(shmid, NULL, 0);
        updated_post.copy(shmchar, strlen(updated_post.c_str()));

        post_map[stoi(words[1])] = make_pair(shmid, shmchar);

        result << "Comment successfully." << endl;
      } else {
        result << "Post does not exist." << endl;
      }
    } else {
      result << "Please login first." << endl;
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
    cout << "[Server] client " << tcp_ip_port << " connected using TCP" << endl;
    tcp_upd[tcp_ip_port] = "unknown";
    //sending tcp_ip_port to client
    send(*c_fd_p, tcp_ip_port.c_str(), strlen(tcp_ip_port.c_str()), 0);

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

      cout << "[" << ip_port << "] " << msg << endl;

      string result = server_service(tcp_upd[ip_port], msg); //changing tcp to udp if possible
      send(c_fd, result.c_str(), strlen(result.c_str()), 0);
    }

    //client graceful disconnect
    cout << "[Server] client " << ip_port << " disconnected as TCP" << endl;
    cout << "[Server] client " << tcp_upd[ip_port] << " disconnected as UDP" << endl;
    login_users.erase(tcp_upd[ip_port]);
    tcp_upd.erase(ip_port);
    shutdown(c_fd, SHUT_RDWR);
    close(c_fd);

    cout << "[Server] Exit tcp service thread for client " << ip_port << endl;
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
      } else { // handle command sent from client using udp
        cout << "[" << ip_port << "] " << msg << endl;
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