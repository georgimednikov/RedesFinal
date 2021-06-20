#include <string>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <memory>
#include "Serializable.h"

class Message: public Serializable
{
public:
    static const size_t MESSAGE_SIZE = 9 * sizeof(char) + 3 * sizeof(uint8_t);

    enum MessageType
    {
        LOGIN, //Se conecta un jugador. Usa Nick
        DISCARD, //Un jugador descarta cartas. Pueden ser 0, 1 para la de la izquierda, 2 para la derecha y 1 2 para ambas.
        LOGOUT, //Se desconecta un jugador
        CARDS, //Que cartas tiene un jugador
        CARD_TABLE, //Nueva carta sobre la mesa
        LOGIN_INFO, //Nick de cada jugador que se da a los clientes
        DISCARD_INFO, //Cuantas cartas ha descartado un jugador
        END_ROUND, //Se resetea el estado del juego
        WINNER //Quien ha ganado la ronda
    };

    Message() : nick(""), type(0), message1(0), message2(0) {};

    Message(const std::string& n, const uint8_t& t, const uint8_t& m1 = 0, const uint8_t& m2 = 0) : nick(n), type(t), message1(m1), message2(m2) {};

    void to_bin()
    {
        alloc_data(MESSAGE_SIZE);

        memset(_data, 0, MESSAGE_SIZE);

        char* tmp = _data;

        memcpy(tmp, nick.c_str(), 9);
        tmp += 8;

        memcpy(tmp, "\0", sizeof(char));
        tmp++;

        memcpy(tmp, &type, sizeof(uint8_t));
        tmp += sizeof(uint8_t);
            
        memcpy(tmp, &message1, sizeof(uint8_t));
        tmp += sizeof(uint8_t);

        memcpy(tmp, &message2, sizeof(uint8_t));
    }

    int from_bin(char * bobj)
    {
        char* tmp = bobj;

        nick = tmp;
        tmp += 9;

        memcpy(&type, tmp, sizeof(uint8_t));
        tmp += sizeof(uint8_t);
            
        memcpy(&message1, tmp, sizeof(uint8_t));
        tmp += sizeof(uint8_t);

        memcpy(&message2, tmp, sizeof(uint8_t));

        return 0;
    }

    std::string nick;
    uint8_t type;
    uint8_t message1;
    uint8_t message2;
};