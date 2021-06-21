#include <string>
#include <unistd.h>
#include <string.h>
#include <memory>
#include <algorithm>
#include <thread>

#include "Deck.cpp"
#include "Socket.h"
#include "Message.cpp"

class PokerServer
{
public:
    PokerServer(const char * s, const char * p): socket(s, p, true) {};
    ~PokerServer() {};

    void do_messages() {
        Message msg;
        Socket* s;
        while (true) {
            std::cout << "Número de clientes: " << clients.size() << std::endl;
            s = socket.accept();
            std::cout << "Ha aceptado un cliente" << std::endl;
            socket.recv(msg, *s);
            std::cout << "Recibe mensaje hilo principal" << std::endl;
            int ind = checkLogin(msg, s);
            PokerServer* server = this;
            if (ind >= 0) {
                std::thread input_thr([server, s, ind](){ server->input_thread(server, s, ind); });
                input_thr.detach();
            }

            //Si no se tienen suficientes jugadores se sigue esperando conexiones
            if (clients.size() < NUM_PLAYERS) continue;

            //Se informa a cada jugador del resto
            for (int i = 0; i < NUM_PLAYERS; i++) {
                Message msg(clients[i].second, Message::LOGIN_INFO);
                sendPlayers(msg);
            }

            //Empieza la partida y se suceden rondas hasta que alguien se desconecte
            while (clients.size() == NUM_PLAYERS) {
                game();
            }
            std::cout << "Se acaba la partida\n";
        }
        delete s;
    }

    void input_thread(PokerServer* server, Socket* s, int ind) {
        Message msg;
        while(true) {
            socket.recv(msg, *s);
            if (server->checkLogout(msg)) {
                server->exit = true;
                return;
            }

            //Solo se lee input del jugador que tiene el turno
            //Esto procesaria todos los mensajes del juego, pero en este caso solo hay uno
            if (server->turn != ind) continue;
            if (msg.type == Message::DISCARD) {
                //Se mira cuantos inputs tiene la instruccion
                int count = 0;
                if (msg.message1 != 0) {
                    count++;
                    if (msg.message2 != 0) count++;
                }
                //Se envian mensajes con las nuevas cartas y las que han sustituido
                for (int j = 0; j < count; j++) {
                    int c = ((j == 0) ? (int)msg.message1 : (int)msg.message2) - 1;
                    Message msg("Server", Message::DISCARD, c, deck->draw());
                    socket.send(msg, *s);
                    hands[ind][c] = msg.message2;
                }
                //Se avisa a los jugadores de cuantas cartas ha descartado este cliente
                msg.type = Message::DISCARD_INFO; msg.message1 = count;
                sendPlayers(msg);
                server->turn++;
            }
            //...
        }
    }

private:
    //Posibles manos en el poker
    enum Hands {High_Card, Pair, Two_Pair, Three_of_a_kind, Straight, Flush, Full_House, Four_of_a_kind, Straight_Flush, Royal_Flush};

    int turn; //De quien es el turno para leer input
    bool exit = true;
    Socket socket;
    Deck* deck; //Baraja para la partida
    int hands[NUM_PLAYERS][2]; //Manos de cada jugador
    std::vector<int> cardsTable; //Cartas en la mesa
    std::vector<std::pair<std::unique_ptr<Socket>, std::string>> clients; //Lista de clientes con su socket y su nick

    void game() {
        deck = new Deck();
        Message m;
        exit = false;

        std::cout << "Se reparten 2 cartas a cada jugador\n";
        for (int i = 0; i < clients.size(); i++) {
            m = Message(clients[i].second, Message::CARDS, deck->draw(), deck->draw());
            socket.send(m, *clients[i].first.get());
            hands[i][0] = m.message1, hands[i][1] = m.message2;
        }

        std::cout << "Se ponen 2 cartas en la mesa\n";
        for (int i = 0; i < 2; i++) {
            m = Message("Server", Message::CARD_TABLE, deck->draw());
            cardsTable.push_back(m.message1);
            sendPlayers(m);
        }

        std::cout << "Se mira cuantas cartas cada jugador quiere descartar\n";
        turn = 0;
        while (turn < NUM_PLAYERS) if (exit) return;

        std::cout << "Se pone otra carta en la mesa\n";
        m = Message("Server", Message::CARD_TABLE, deck->draw());
        cardsTable.push_back(m.message1);
        sendPlayers(m);

        std::cout << "Se mira cuantas cartas cada jugador quiere descartar\n";
        turn = 0;
        while (turn < NUM_PLAYERS) if (exit) return;

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
        std::cout << m.nick << " " << (int)m.type << " " << (int)m.message1 << " " << (int)m.message2 << std::endl;

        //5 segundos entre ronda y ronda
        sleep(5);

        std::cout << "Se resetea el juego\n";
        m = Message("Server", Message::END_ROUND);
        sendPlayers(m);
        cardsTable.clear();
    }

    void closeGame() {
        if (deck != nullptr) delete deck;
        cardsTable.clear();
    }

    //Manda un mensaje a todos los jugadores conectados
    void sendPlayers(Message& m) {
        for (int j = 0; j < clients.size(); j++) {
            socket.send(m, *clients[j].first.get());
        }
    }

    //Se mira si hay un mensaje LOGIN y si lo hay se a�ade la nuevo cliente a la lista
    int checkLogin(Message& m, Socket* s) {
        if (m.type != Message::LOGIN) return -1;
        clients.push_back(std::pair<std::unique_ptr<Socket>, std::string>(std::move(std::make_unique<Socket>(*s)), m.nick));
        std::cout << "Conectado: " << m.nick << std::endl;
        return clients.size() - 1;
    }

    //Se mira si hay un mensaje LOGOUT y si lo hay se desconecta al jugador y se avisa al resto de clientes
    bool checkLogout(Message& m) {
        if (m.type != Message::LOGOUT) return false;
        auto it = clients.begin();
        while (it != clients.end() && !((it->second) == m.nick)) it++;
        if(it != clients.end()) {
            clients.erase(it);
            if (!exit) {
                Message msg("Server", Message::LOGOUT);
                sendPlayers(msg);
                closeGame();
            }
            std::cout << "Desconectado a: " << m.nick << std::endl;
        }
        else {std::cout << "No encontrado a: " << m.nick << std::endl; return false;}
        return true;
    }

    //Decide el ganador en base a las manos y las cartas en la mesa
    int checkWinner() {
        int winner = 0; 
        int highest_card[NUM_PLAYERS];
        Hands highest_hand[NUM_PLAYERS];
        bool found = false;

        //Se guarda la mejor combinaci�n de cada jugador
        for (int i = 0; i < NUM_PLAYERS; i++) {
            std::vector<int> playerHand;
            for(int j = 0; j < 5; j++) {
                if(j < 2) playerHand.push_back(hands[i][j]); 
                else playerHand.push_back(cardsTable[j - 2]);
            }

            std::sort(playerHand.begin(), playerHand.end(), Deck::HighestCardCmp);

            checkFlush(playerHand, highest_hand[i], highest_card[i]);
            checkPairs(playerHand, highest_hand[i], highest_card[i]);
        }

        //Se comparan manos. En caso de empate se miran las siguientes combinaciones m�s valiosas
        for (size_t i = 1; i < NUM_PLAYERS; i++) {
            if(highest_hand[i] > highest_hand[winner]) winner = i;
            else if(highest_hand[i] == highest_hand[winner] && highest_card[i] > highest_card[winner]) winner = i;
            else if(highest_card[i] == highest_card[winner]) {
                int better = -1;
                int maxI = Deck::HighestCard(hands[i][0], hands[i][1]), maxWin = Deck::HighestCard(hands[winner][0], hands[winner][1]);
                if( maxI > maxWin) winner = i;
                else if (maxI == maxWin) {
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

    //Mira la mejor combinaci�n en base a palos y n�meros
    void checkFlush(std::vector<int> playerHand, Hands &hand, int &card) {
        bool flush = true, straight = true;
        for (int i = 1; i < playerHand.size() && (flush || straight); i++) {
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

    //Mira la mejor combinacion de una mano basada en la repetici�n de cartas
    void checkPairs(std::vector<int> playerHand, Hands &hand, int &card) {
        if (hand > Hands::Full_House) return;
        int cont = 1;
        bool pair = false,  double_pair = false, threesome = false, foursome = false;
        for (int i = 1; i < playerHand.size() + 1; i++) {
            if (i < playerHand.size() && playerHand[i - 1] % NUM_CARDS_RANK == playerHand[i] % NUM_CARDS_RANK)
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
                if (cont > 1) {
                    card = playerHand[i - 1] % NUM_CARDS_RANK;
                    cont = 1;
                }
            }
        }

        if(pair && threesome) hand = Hands::Full_House;
        else if (threesome) hand = Hands::Three_of_a_kind;
        else if (double_pair) hand = Hands::Two_Pair;
        else if (pair) hand = Hands::Pair;
    }
};

int main(int arg, char *argv[]) {
    if (arg != 3) return -1;
    PokerServer es(argv[1], argv[2]);
    es.do_messages();
    return 0;
}