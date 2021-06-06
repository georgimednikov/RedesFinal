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
            std::string msg;
            std::getline(std::cin, msg);
            Message em(nicks[0], msg);

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
        }
    }


private:

    /**
     * Socket para comunicar con el servidor
     */
    Socket socket;

    /**
     * Nick del usuario
     */

    std::string nicks[NUM_PLAYERS];
    int hands[4];
};