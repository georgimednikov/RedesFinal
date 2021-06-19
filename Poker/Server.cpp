#include <string>
#include <unistd.h>
#include <string.h>
#include <memory>
#include <algorithm>

#include "Deck.cpp"
#include "Socket.h"
#include "Message.cpp"

const int NUM_PLAYERS = 2;

/**
 *  Clase para el servidor de poker
 */
class PokerServer
{
public:
    PokerServer(const char * s, const char * p): socket(s, p, true) {};

    /**
     *  Thread principal del servidor recive mensajes en el socket y
     *  lo distribuye a los clientes. Mantiene actualizada la lista de clientes
     */
    void do_messages() {
        Message msg;
        Socket* s;
        while (true) {
            std::cout << "Clientes: " << clients.size() << std::endl;
            // for (int i = 0; i < clients.size(); i++) {
            //     socket.recv(msg, *clients[i].first.get());
            //     checkLogout(msg, clients[i].first.get());
            // }
            // std::cout << "Se han mirado los mensajes de los clientes anteriores" << std::endl;
            s = socket.accept();
            //std::cout << "Se acepta" << std::endl;
            socket.recv(msg, *s);
            checkLogin(msg, s);
            //std::cout << "Se han mirado los mensajes del cliente nuevo" << std::endl;

            if (clients.size() < NUM_PLAYERS) continue;
            //std::cout << "Se empieza el juego" << std::endl;

            for (int i = 0; i < NUM_PLAYERS; i++) {
                Message msg(clients[i].second, Message::LOGIN_INFO);
                sendPlayers(msg);
            }

            while (clients.size() == NUM_PLAYERS) {
                game();
            }
            Message m = Message("Server", Message::LOGOUT);
            sendPlayers(m);
        }
    }

private:
    enum Hands {High_Card, Pair, Two_Pair, Three_of_a_kind, Straight, Flush, Full_House, Four_of_a_kind, Straight_Flush, Royal_Flush};

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

        char c;
        std::cout << "Se reparten 2 cartas a cada jugador\n";
        for (int i = 0; i < clients.size(); i++) {
            m = Message(clients[i].second, Message::CARDS, deck->draw(), deck->draw());
            socket.send(m, *clients[i].first.get());
        }

        std::cout << "Se ponen 2 cartas en la mesa\n";
        m = Message("Server", Message::CARD_TABLE, deck->draw(), deck->draw());
        cardsTable.push_back(m.message1); cardsTable.push_back(m.message2);
        sendPlayers(m);
        std::cin >> c;

        std::cout << "Se mira cuantas cartas cada jugador quiere descartar\n";
        for (int i = 0; i < NUM_PLAYERS; i++) {
            std::cout << "Pre get\n";
            do {
                socket.recv(m, *(clients[j].first.get()));
                if (checkLogout(m, s)) return;
            }
            while (s != clients[i].first.get() && m.type != Message::DISCARD);
            std::cout << "Post get\n";

            int count = 0; if (m.message1 != 0) {
                count++;
                if (m.message2 != 0) count++;
            }
            m.type = Message::DISCARD_INFO; m.message1 = count;
            sendPlayers(m);
        }
        std::cin >> c;

        std::cout << "Se mira quien se retira\n";
        for (int i = 0; i < NUM_PLAYERS; i++) {
            do {
                socket.recv(m, *(clients[i].first.get()));
                if (checkLogout(m, s)) return;
            }
            while (s != clients[i].first.get() && m.type != Message::PASS && m.type != Message::BET);
            sendPlayers(m);
        }

        std::cout << "Se pone otra carta en la mesa\n";
        m = Message("Server", Message::CARD_TABLE, deck->draw());
        cardsTable.push_back(m.message1);
        sendPlayers(m);

        std::cout << "Se mira cuantas cartas cada jugador quiere descartar\n";
        for (int i = 0; i < NUM_PLAYERS; i++) {
            do {
                socket.recv(m, *(clients[i].first.get()));
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

        std::cout << "Se mira quien se retira\n";
        for (int i = 0; i < NUM_PLAYERS; i++) {
            do {
                socket.recv(m, *(clients[i].first.get()));
                if (checkLogout(m, s)) return;
            }
            while (s != clients[i].first.get() && m.type != Message::PASS && m.type != Message::BET);
            sendPlayers(m);
        }

        std::cout << "Se muestran las cartas\n";
        for (int i = 0; i < NUM_PLAYERS; i++) {
            m = Message(clients[i].second, Message::CARDS, hands[i][0], hands[i][1]);
            sendPlayers(m);
        }

        std::cout << "Se comprueba quien gana\n";
        int winner = checkWinner();
        if (winner >= 0) m = Message(clients[winner].second, Message::WINNER);
        else m = Message("Server", Message::WINNER);
        sendPlayers(m);

        std::cout << "Se resetea el juego\n";
        m = Message("Server", Message::END_ROUND);
        sendPlayers(m);
        cardsTable.clear();
    }

    void sendPlayers(Message& m) {
        for (int j = 0; j < clients.size(); j++) {
            std::cout << "Manda a " << clients[j].second << " el mensaje: " << m.nick << " " << (int)m.type << " " << (int)m.message1 << " " << (int)m.message2 << "\n";
            socket.send(m, *clients[j].first.get());
        }
    }

    bool checkLogin(Message& m, Socket* s) {
        if (m.type != Message::LOGIN) return false;
        clients.push_back(std::pair<std::unique_ptr<Socket>, std::string>(std::move(std::make_unique<Socket>(*s)), m.nick));
        std::cout << "Conectado: " << m.nick << std::endl;
        return true;
    }

    bool checkLogout(Message& m, Socket* s) {
        if (m.type != Message::LOGOUT) return false;
        auto it = clients.begin();
        while (it != clients.end() && !(*(it->first.get()) == *s)) it++;
        if(it != clients.end()) {
            std::cout << "Desconectado a: " << m.nick << std::endl;
            clients.erase(it);
            it->first.release();
        }
        else std::cout << "No encuentra" << std::endl;
        return true;
    }

    int checkWinner() 
    {
        int winner = 0; 
        int highest_card[NUM_PLAYERS];
        Hands highest_hand[NUM_PLAYERS];
        bool found = false;
        for (int i = 0; i < NUM_PLAYERS; i++)
        {
            std::vector<int> playerHand;
            for(int j = 0; j < 5; j++) {
                if(j < 2) playerHand.push_back(hands[i][j]); 
                else playerHand.push_back(cardsTable[j - 2]);
            }
            std::sort(playerHand.begin(), playerHand.end(), Deck::HighestCard);

            checkFlush(playerHand, highest_hand[i], highest_card[i]);
            checkPairs(playerHand, highest_hand[i], highest_card[i]);
        }

        for (size_t i = 1; i < NUM_PLAYERS; i++)
        {
            if(highest_hand[i] > highest_hand[winner])
                winner = i;
            else if(highest_hand[i] == highest_hand[winner] && highest_card[i] > highest_card[winner])
                winner = i;
            else if(highest_card[i] == highest_card[winner])
            {
                int better = -1;
                int maxI = Deck::HighestCard(hands[i][0], hands[i][1]), maxWin = Deck::HighestCard(hands[winner][0], hands[winner][1]);
                if( maxI > maxWin)
                    winner = i;
                else if (maxI == maxWin)
                {
                    maxI = (maxI == hands[i][0]) ? hands[i][1]: hands[i][0];
                    maxWin = (maxWin == hands[winner][0]) ? hands[winner][1]: hands[winner][0];
                    if( maxI > maxWin) 
                        winner = i;
                    else if (maxI == maxWin)
                    winner = -1;
                }
            }
        }

        return winner;
    }

    void checkFlush(std::vector<int> playerHand, Hands hand, int card)
    {
        bool flush = true, straight = true;
        for (int i = 1; i < playerHand.size() && (flush || straight); i++)
        {
            flush = Deck::SameSuit(playerHand[i - 1], playerHand[i]);
            straight = (playerHand[i - 1] % NUM_CARDS_RANK == playerHand[i] % NUM_CARDS_RANK + 1);
        }

        if (flush && straight && (playerHand[playerHand.size() - 1] % NUM_CARDS_RANK == NUM_CARDS_RANK - 1)) hand = Hands::Royal_Flush;
        else if (flush && straight) hand = Hands::Straight_Flush;
        else if (flush) hand = Hands::Flush;
        else if (straight) hand = Hands::Straight;
        else hand = Hands::High_Card;
        card = playerHand[playerHand.size() - 1] % NUM_CARDS_RANK ;
    }

    void checkPairs(std::vector<int> playerHand, Hands hand, int card)
    {
        if (hand > Hands::Full_House) return;
        int cont = 1;
        bool pair = false,  double_pair = false, threesome = false, foursome = false;
        for (int i = 1; i < playerHand.size(); i++)
        {
            if (playerHand[i - 1] % NUM_CARDS_RANK == playerHand[i] % NUM_CARDS_RANK)
                cont++;
            else {
                switch (cont)
                {
                case 2:
                    if(!pair) pair = true;
                    else double_pair = true;
                    break;
                case 3:
                    threesome = true;
                    break;
                case 4:
                    foursome = true;
                    hand = Hands::Four_of_a_kind;
                    card = playerHand[i - 1] % NUM_CARDS_RANK;
                    return;
                }
                card = playerHand[i - 1] % NUM_CARDS_RANK;
                cont = 1;
            }
        }

        if(pair && threesome) hand = Hands::Full_House;
        else if (threesome) hand = Hands::Three_of_a_kind;
        else if (double_pair) hand = Hands::Two_Pair;
        else if (pair) hand = Hands::Pair;
    }

};

int main(int arg, char *argv[])
{
    PokerServer es(argv[1], argv[2]);
    es.do_messages();
    return 0;
}