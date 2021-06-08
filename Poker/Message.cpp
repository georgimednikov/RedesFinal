#include <string>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <memory>
#include "Serializable.h"

class Message: public Serializable
{
public:
    static const size_t MESSAGE_SIZE = 9 * sizeof(char) + 2 * sizeof(uint8_t) + sizeof(uint16_t);

    enum MessageType
    {
        LOGIN, //Se conecta un jugador
        PASS, //Se pasa la ronda
        BET, //Se apuesta una cantidad
        DISCARD, //Se descarta 0, 1 o 2 cartas
        LOGOUT, //Se desconecta un jugador
        CARDS, //Que cartas tiene un jugador
        CARD_TABLE, //Nueva carta sobre la mesa
        LOGIN_INFO, //La información que se le da a un jugador cuando se une
        DISCARD_INFO, //Cuantas cartas ha descartado un jugador
        END_ROUND //Se resetea el estado del juego
    };

    Message(){};

    Message(const std::string& n, const uint8_t& t, const uint16_t& m1 = -1, const uint8_t& m2 = -1) : nick(n), type(t), message1(m1), message2(m2) {};

    void to_bin()
    {
        alloc_data(MESSAGE_SIZE);

        memset(_data, 0, MESSAGE_SIZE);

        char* tmp = _data;

        memcpy(tmp, nick.c_str(), 8);
        tmp += 8;

        memcpy(tmp, '\0', sizeof(char));
        tmp++;

        memcpy(tmp, &type, sizeof(uint8_t));
        tmp += sizeof(uint8_t);
            
        memcpy(tmp, &message1, sizeof(uint16_t));
        tmp += sizeof(uint16_t);

        memcpy(tmp, &message2, sizeof(uint8_t));
    }

    int from_bin(char * bobj)
    {
        alloc_data(MESSAGE_SIZE);

        memcpy(static_cast<void *>(_data), bobj, MESSAGE_SIZE);

        char* tmp = _data;

        nick = tmp;
        tmp += 9;

        memcpy(&type, tmp, sizeof(uint8_t));
        tmp += sizeof(uint8_t);
            
        memcpy(&message1, tmp, sizeof(uint16_t));
        tmp += sizeof(uint16_t);

        memcpy(&message2, tmp, sizeof(uint8_t));

        return 0;
    }

    std::string nick;
    uint8_t type;
    uint16_t message1;
    uint8_t message2;
};