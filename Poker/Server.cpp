#include <string>
#include <unistd.h>
#include <string.h>
#include <memory>

#include "Deck.cpp"
#include "Socket.h"
#include "Message.cpp"

const int NUM_PLAYERS = 4;

/**
 *  Clase para el servidor de poker
 */
class PokerServer
{
public:
    PokerServer(const char * s, const char * p): socket(s, p)
    {
        socket.bind();
    };

    /**
     *  Thread principal del servidor recive mensajes en el socket y
     *  lo distribuye a los clientes. Mantiene actualizada la lista de clientes
     */
    void do_messages() {
        while (true) {
            Message msg;
            Socket* s;
            socket.recv(msg, s);
            s->bind();

            checkLogin(msg, s);
            checkLogout(msg, s);

            if (clients.size() < NUM_PLAYERS) continue;
            while (clients.size() == NUM_PLAYERS) {
                game();
            }
            Message m = Message("Server", Message::LOGOUT);
            sendPlayers(m);
            
        }
    }

private:
    Deck* deck;

    /**
     * Manos de los jugadores
     */
    int hands[NUM_PLAYERS][2];

    /**
     * Cartas en la mesa
     */
    std::vector<int> cardsTable;

    /**
     *  Lista de clientes conectados al servidor de Chat, representados por
     *  su socket
     */
    std::vector<std::pair<std::unique_ptr<Socket>, std::string>> clients;

    /**
     * Socket del servidor
     */
    Socket socket;

    void game() {
        deck = new Deck();
        Message m;
        Socket* s;

        //Se reparten 2 cartas a cada jugador
        for (int i = 0; i < clients.size(); i++) {
            m = Message(clients[i].second, Message::CARDS, deck->draw(), deck->draw());
            socket.send(m, *clients[i].first.get());
        }

        //Se ponen 2 cartas en la mesa
        for (int i = 0; i < 2; i++) {
            m = Message("Server", Message::CARD_TABLE, deck->draw());
            cardsTable.push_back(m.message1);
            sendPlayers(m);
        }

        //Se mira cuantas cartas cada jugador quiere descartar
        for (int i = 0; i < NUM_PLAYERS; i++) {
            do {
                socket.recv(m, s);
                if (checkLogout(m, s)) return;
            }
            while (s != clients[i].first.get() && m.type != Message::DISCARD);
            int count = 0; if (m.message1 != -1) {
                count++;
                if (m.message2 != -1) count++;
            }
            m.type = Message::DISCARD_INFO; m.message1 = count;
            sendPlayers(m);
        }

        //Se mira quien se retira
        for (int i = 0; i < NUM_PLAYERS; i++) {
            do {
                socket.recv(m, s);
                if (checkLogout(m, s)) return;
            }
            while (s != clients[i].first.get() && m.type != Message::PASS && m.type != Message::BET);
            sendPlayers(m);
        }

        //Se pone otra carta en la mesa
        m = Message("Server", Message::CARD_TABLE, deck->draw());
        cardsTable.push_back(m.message1);
        sendPlayers(m);

        //Se mira cuantas cartas cada jugador quiere descartar
        Socket* s;
        for (int i = 0; i < NUM_PLAYERS; i++) {
            do {
                socket.recv(m, s);
                if (checkLogout(m, s)) return;
            }
            while (s != clients[i].first.get() && m.type != Message::DISCARD);
            int count = 0; if (m.message1 != -1) {
                count++;
                if (m.message2 != -1) count++;
            }
            m.type = Message::DISCARD_INFO; m.message1 = count;
            sendPlayers(m);
        }

        //Se mira quien se retira
        for (int i = 0; i < NUM_PLAYERS; i++) {
            do {
                socket.recv(m, s);
                if (checkLogout(m, s)) return;
            }
            while (s != clients[i].first.get() && m.type != Message::PASS && m.type != Message::BET);
            sendPlayers(m);
        }

        //Se muestran las cartas
        for (int i = 0; i < NUM_PLAYERS; i++) {
            m = Message(clients[i].second, Message::CARDS, hands[i][0], hands[i][1]);
            sendPlayers(m);
        }

        //Se comprueba quien gana
        checkWinner();

        //Se resetea el juego
        m = Message("Server", Message::END_ROUND);
        sendPlayers(m);
        cardsTable.clear();
    }

    void sendPlayers(Message m) {
        for (int j = 0; j < NUM_PLAYERS; j++) {
            socket.send(m, *clients[j].first.get());
        }
    }

    bool checkLogin(Message m, Socket* s) {
        if (m.type != Message::LOGIN) return false;
        std::cout << "Conectado: " << m.nick << std::endl;
        clients.push_back(std::pair<std::unique_ptr<Socket>, std::string>(std::move(std::make_unique<Socket>(*s)), m.nick));
        return true;
    }

    bool checkLogout(Message m, Socket* s) {
        if (m.type != Message::LOGOUT) return false;
        auto it = clients.begin();
        while (it != clients.end() && it->first.get() != s) it++;
        if(it != clients.end()) {
            std::cout << "Desconectado a: " << m.nick << std::endl;
            clients.erase(it);
            it->first.release();
        }
        return true;
    }

    void checkWinner() {
        
    }
};


int main(int arg, char *argv[])
{
    PokerServer es(argv[1], argv[2]);
    es.do_messages();
    return 0;
}