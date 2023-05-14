/** @file
 * @author Воронин Н.А.
 * @date 01.05.2023
 * @copyright ИБСТ ПГУ
 * @brief Главный модуль проекта
 * @param opt переменная для работы с параметрами командной строки
 * @param optarg переменная для получения парметров командной строки
 */
 
#include "ServKurs.h"

using namespace std;

int main(int argc, char *argv[])
{
    try {
        bool start = false;
        int port = 33333;
        string basa = "/etc/vcalc.conf";
        string jur = "/var/log/vcalc.log";
        static struct option long_options[] = {
            {"start", 0, 0, 's'},
            {"help", 0, NULL, 'h'},
            {"base", 1, NULL, 'b'},
            {"journal", 1, NULL, 'j'},
            {"port", 1, NULL, 'p'},
            {NULL, 0, NULL, 0}
        };
        int opt;
        opt = getopt_long(argc, argv, ":hsb:j:p:", long_options, NULL);
        do {
            if (opt == -1 or (char)opt == 'h') {
                cout << "Программа-сервер для вычисления произведения вектора может использовать следующие параметры:\n\n";
                cout << "\"h\", \"help\" или отсутствие параметров – вызывают данное сообщение-справку\n\n";
                cout << "\"s\" или \"start\" – непосредственно запускают работу программы-сервера, обязательный.\n";
                cout << "                   Данный параметр необходим поскольку у программы-сервера нет обязательных параметров, а запуск без параметров должен выдавать справку\n";
                cout << "\"b\" или \"base\" – передает программе имя файла с базой клиентов, необязательный.\n";
                cout << "                   Значение по умолчанию: \"~/etc/vcalc.conf\"\n";
                cout << "\"j\" или \"journal\" – передает программе имя файла с журналом работы, необязательный.\n";
                cout << "                   Значение по умолчанию: \"~/var/log/vcalc.log\"\n";
                cout << "\"p\" или \"port\" – передает программе порта сервера, необязательный.\n";
                cout << "                   Значение по умолчанию: \"33333\"\n";
                return 0;
            } else if ((char)opt == 's') {
                start = true;
            } else if ((char)opt == 'p') {
                port = stoi(optarg);
            }  else if ((char)opt == 'b') {
                basa = string(optarg);
            } else if ((char)opt == 'j') {
                jur = string(optarg);
            } else if ((char)opt == ':') {
                cout << "Введенный параметр требует аргумент. Для вызова справки воспользуйтесь параметрами \"h\" или \"help\"\n";
                return 0;
            } else {
                cout << "Неизвестный параметр. Для вызова справки воспользуйтесь параметрами \"h\" или \"help\"\n";
                return 0;
            }
        } while((opt = getopt_long(argc, argv, ":hsb:j:p:", long_options, NULL))!= -1);
        if (start) {
            WorkWithClient vork(basa, jur, port);
            struct sockaddr_in {
                short sin_family;
                unsigned short sin_port;
                struct in_addr sin_addr;
                char sin_zero[8];
            };
            struct in_addr {
                unsigned long s_addr;
            };

            int s = socket(AF_INET, SOCK_STREAM, 0);

            sockaddr_in * self_addr = new (sockaddr_in);
            self_addr->sin_family = AF_INET;
            self_addr->sin_port = htons(port);
            self_addr->sin_addr.s_addr = inet_addr("127.0.0.1");

            int b = bind(s,(const sockaddr*) self_addr,sizeof(sockaddr_in));
            if(b == -1) {
                vork.errrecord("Критическая","Ошибка привязки к адресу");
                throw string("Ошибка привязки к адресу");
            }
            int rl = listen(s, 5);
            if(rl == -1) {
                vork.errrecord("Критическая","Ошибка установки в режим ожидания");
                throw string("Ошибка установки в режим ожидания");
            }
            while(true) {
                sockaddr_in * client_addr = new sockaddr_in;
                socklen_t len = sizeof (sockaddr_in);
                int work_sock = accept(s, (sockaddr*)(client_addr), &len);
                if(work_sock == -1) {
                    vork.errrecord("Некритическая","Ошибка установки соединения с клиентом");
                    continue;
                } else {
                    char* id = new char[400];
                    int rc;
                    rc = recv(work_sock, id, sizeof(id), 0);
                    if (rc == -1) {
                        vork.errrecord("Некритическая","Ошибка получения идентификатора ID пользователя");
                        close(work_sock);
                        continue;
                    }
                    if (!vork.checID(string(id))) {
                        vork.errrecord("Некритическая","Ошибка идентификации");
                        char errmsg[] = "ERR";
                        send(work_sock, errmsg, 3, 0);
                        delete[] id;
                        close(work_sock);
                        continue;
                    }
                    delete[] id;
                    string sol = vork.makesalt();
                    char salt[sol.length() + 1];
                    sol.copy(salt, sol.length() + 1);
                    rc = send(work_sock, salt, 16, 0);
                    if (rc == -1) {
                        vork.errrecord("Некритическая","Ошибка отправки случайного числа SALT");
                        close(work_sock);
                        continue;
                    }
                    char* parol = new char[300];
                    rc = recv(work_sock, parol, 300, 0);
                    if (rc == -1) {
                        vork.errrecord("Некритическая","Ошибка получения пароля пользователя");
                        close(work_sock);
                        continue;
                    }
                    if (!vork.checparol(string(parol))) {
                        vork.errrecord("Некритическая","Ошибка аутентификации");
                        char errmsg[] = "ERR";
                        send(work_sock, errmsg, sizeof(errmsg), 0);
                        close(work_sock);
                        continue;
                    }
                    delete[] parol;
                    rc = send(work_sock, "OK", 2, 0);
                    if (rc == -1) {
                        vork.errrecord("Некритическая","Ошибка отправки ответа");
                        close(work_sock);
                        continue;
                    }
                    uint32_t num_v, v_len;
                    rc = recv(work_sock, (void *) &num_v, sizeof num_v, 0);
                    if (rc == -1) {
                        vork.errrecord("Некритическая","Ошибка получения числа векторов");
                        close(work_sock);
                        continue;
                    }
                    for (uint32_t i = 0; i < num_v; ++i) {
                        rc = recv(work_sock, (void *) &v_len, sizeof v_len, 0);
                        if (rc == -1) {
                            vork.errrecord("Некритическая","Ошибка получения длины вектора");
                            close(work_sock);
                            continue;
                        }
                        int32_t* data = new int32_t[v_len];
                        rc = recv(work_sock, (void *) data, sizeof(int32_t) * v_len, 0);
                        if (rc == -1) {
                            vork.errrecord("Некритическая","Ошибка получения значений вектора");
                            close(work_sock);
                            continue;
                        }
                        int32_t sum = vork.count(v_len,data);
                        delete[] data;
                        rc = send(work_sock, (void*)&sum, sizeof(int32_t), 0);
                        if (rc == -1) {
                            vork.errrecord("Некритическая","Ошибка отправки результата");
                            close(work_sock);
                            continue;
                        }
                    }
                    close(work_sock);
                }
            }
            close(s);
        } else (cout << "Отсутствует параметр запуска. Для вызова справки воспользуйтесь параметрами \"h\" или \"help\"\n");
    } catch (const string & exs) {
        cerr << exs << endl;
    } catch (const exception & exs) {
        cerr << "Необработанная ошибка: " << exs.what() << endl;
    }
    return 0;

}
