#include "Serializable.h"
#include "Socket.h"
#include "Message.cpp"

const int NUM_PLAYERS = 4;

class Player
{
public:
    /**
     * @param s dirección del servidor
     * @param p puerto del servidor
     * @param n nick del usuario
     */
    Player(const char * s, const char * p, const char * n): socket(s, p) {
        nicks[0] = n;
        resetState();
    };

    /**
     *  Envía el mensaje de login al servidor
     */
    void login()
    {
        Message em(nicks[0], Message::LOGIN);
        socket.send(em, socket);
    }

    /**
     *  Envía el mensaje de logout al servidor
     */
    void logout()
    {
        Message em(nicks[0], Message::LOGOUT);
        socket.send(em, socket);
    }

    /**
     *  Rutina principal para el Thread de E/S. Lee datos de STDIN (std::getline)
     *  y los envía por red vía el Socket.
     */
    void input_thread()
    {    
        while (true)
        {
            // Leer stdin con std::getline
            std::string inp;
            std::getline(std::cin, inp);
            size_t space_pos = inp.find(" ");
            std::string inst; 
            if (space_pos != std::string::npos) {
                inst = inp.substr(0, space_pos);
                inp = inp.substr(space_pos + 1);
            }

            Message em = Message();
            em.nick = nicks[0];
            if (inst == "LOGOUT") em.type = Message::LOGOUT;
            else if (inst == "PASS") em.type = Message::PASS;
            /*else if (inst == "BET") {
                em.type = Message::BET;
                em.message1 = std::atoi(inp);
            }*/
            else if (inst == "DISCARD") {
                em.type = Message::DISCARD;
                space_pos = s.find(" ");
                if (space_pos == std::string::npos) {
                    em.message1 = std::atoi(inp);
                }
                else {
                    em.message1 = std::atoi(s.substr(0, space_pos));
                    inp = inp.substr(space_pos + 1);
                    em.message2 = std::atoi(inp);
                }
            }
            else continue;

            // Enviar al servidor usando socket
            socket.send(em, socket);
        }
    }

    /**
     *  Rutina del thread de Red. Recibe datos de la red y los "renderiza"
     *  en STDOUT
     */
    void net_thread()
    {
        while(true)
        {
            Message msg;
            socket.recv(msg);

            switch (msg.type)
            {
            case Message::LOGIN_INFO:
                /* code */
                break;
            case Message::DISCARD_INFO:
                int pos = searchNick(msg.nick);
                if (pos == -1) std::cerr << "Invalid Nick: " << msg.nick << "\n";
                for (int i = 0; i < msg.message1; i++) discarded[pos][i] = true;
                break;
            case Message::CARDS:
                if (msg.nick == nicks[0]) continue;
                int pos = searchNick(msg.nick);
                if (pos == -1) std::cerr << "Invalid Nick: " << msg.nick << "\n";
                for (int i = 0; i < msg.message1; i++) hands[pos][i] = true;
                break;
            case Message::PASS:
                int pos = searchNick(msg.nick);
                if (pos == -1) std::cerr << "Invalid Nick: " << msg.nick << "\n";
                for (int i = 0; i < 2; i++) hand[pos][i] = -1;
                break;
            }
        }
    }


private:

    /**
     * Socket para comunicar con el servidor
     */
    Socket socket;

    /**
     * Nicks de los jugadores
     */
    std::string nicks[NUM_PLAYERS];

    /**
     * Manos de los jugadores
     */
    int hands[NUM_PLAYERS][2]; // 0 significa que no se conoce la carta, -1 que se ha retirado

    /**
     * Descartes de los jugadores
     */
    bool discarded[NUM_PLAYERS][2];

    int searchNick(std::string nc) {
        for (int i = 0; i < NUM_PLAYERS; ++i) {
            if (nicks[i] == nc) return i;
        }
        return -1;
    }

    void resetState() {
        for (int i = 0; i < NUM_PLAYERS; i++)
            for (int j = 0; j < 2; j++) {
                hands[i][j] = 0;
                discarded[i][j] = false;
            }
    }
};