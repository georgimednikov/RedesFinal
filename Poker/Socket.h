#ifndef SOCKET_H_
#define SOCKET_H_

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include <iostream>
#include <stdexcept>

#include <ostream>
#include <string.h>

#include "Serializable.h"


// -----------------------------------------------------------------------------
// Definiciones adelantadas
// -----------------------------------------------------------------------------
class Socket;
class Serializable;

/**
 *  Esta función compara dos Socks, realizando la comparación de las structuras
 *  sockaddr: familia (INET), dirección y puerto, ver ip(7) para comparar
 *  estructuras sockaddr_in. Deben comparar el tipo (sin_family), dirección
 *  (sin_addr.s_addr) y puerto (sin_port). La comparación de los campos puede
 *  realizarse con el operador == de los tipos básicos asociados.
 */
bool operator== (const Socket &s1, const Socket &s2);

/**
 *  Imprime la dirección y puerto en número con el formato:"dirección_ip:puerto"
 */
std::ostream& operator<<(std::ostream& os, const Socket& dt);
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------


/**
 * Clase base que representa el extremo local de una conexión UDP. Tiene la lógica
 * para inicializar un sockect y la descripción binaria del extremo
 *   - dirección IP
 *   - puerto
 */
class Socket
{
public:
    /**
     * El máximo teórico de un mensaje UDP es 2^16, del que hay que
     * descontar la cabecera UDP e IP (con las posibles opciones). Se debe
     * utilizar esta constante para definir buffers de recepción.
     */
    static const int32_t MAX_MESSAGE_SIZE = 32768;

    /**
     *  Construye el socket UDP con la dirección y puerto dados. Esta función
     *  usara getaddrinfo para obtener la representación binaria de dirección y
     *  puerto.
     *
     *  Además abrirá el canal de comunicación con la llamada socket(2).
     *
     *    @param address cadena que representa la dirección o nombre
     *    @param port cadena que representa el puerto o nombre del servicio
     */
    Socket(const char * address, const char * port, bool passive)
    {
        //Construir un socket de tipo AF_INET y SOCK_DGRAM usando getaddrinfo.
        //Con el resultado inicializar los miembros sd, sa y sa_len de la clase

        struct addrinfo hints;
        struct addrinfo * res;

        memset((void *) & hints, 0, sizeof(struct addrinfo));
        
        if(passive)
        {
            hints.ai_flags = AI_PASSIVE;
        }
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        int rc = getaddrinfo(address, port, &hints, &res);

        if(rc != 0)
        {
            std::cerr << "[getaddrinfo] " << gai_strerror(rc) << std::endl;
        }

        sa     = *res->ai_addr;
        sa_len = res->ai_addrlen;
        
        sd = socket(res->ai_family, res->ai_socktype, 0);
        
        if(sd == -1)
        {
            std::cout << "[socket] " << strerror(errno) << "\n";
        }
        
        if(!passive)
        {
            connect(sd, res->ai_addr, res->ai_addrlen);
        }
        else
        {
            bind();
            listen();
        }

        freeaddrinfo(res);
    }

    Socket() {
        struct sockaddr socka;
        socklen_t socka_len = sizeof(struct sockaddr);
        sd = -1;
        sa = socka;
        sa_len = socka_len;
    }

    //Clientes en el Server
    Socket(int s, sockaddr socka, socklen_t socka_len ):sd(s), sa(socka), sa_len(socka_len) {};

    /**
     *  Inicializa un Socket copiando los parámetros del socket
     */
    Socket(struct sockaddr * _sa, socklen_t _sa_len):sd(-1), sa(*_sa),
        sa_len(_sa_len){};

    virtual ~Socket(){};

    /**
     *  Recibe un mensaje de aplicación
     *
     *    @param obj que recibirá los datos de la red. Se usará para la
     *    reconstrucción del objeto mediante Serializable::from_bin del interfaz.
     *
     *    @param sock que identificará al extremo que envía los datos si es
     *    distinto de 0 se creará un objeto Socket con la dirección y puerto.
     *
     *    @return 0 en caso de éxito o -1 si error (cerrar conexión)
     */
    int recv(Serializable &obj, Socket& sock)
    {
        char buffer[MAX_MESSAGE_SIZE];

        ssize_t bytes = ::recv(sock.sd, buffer, MAX_MESSAGE_SIZE, 0);

        if ( bytes < 0 )
        {
            std::cerr << strerror(errno) << '\n';
            return -1;
        }

        obj.from_bin(buffer);

        return 0;
    }

    int getSocket(){return sd;}

    /**
     *  Envía un mensaje de aplicación definido por un objeto Serializable.
     *
     *    @param obj en el que se enviará por la red. La función lo serializará
     *
     *    @param sock con la dirección y puerto destino
     *
     *    @return 0 en caso de éxito o -1 si error
     */
    int send(Serializable& obj, Socket sock)
    {
        obj.to_bin();

        if(::send(sock.sd, obj.data(), obj.size(), 0) < 0){
            std::cerr << strerror(errno) << '\n';
            return -1;
        }
        return 0;
    }

    Socket* accept()
    {
        struct sockaddr client;
        socklen_t clientLen = sizeof(struct sockaddr);
        int s = ::accept(sd, &client, &clientLen);

        return new Socket(s, client, clientLen);
    }

    /**
     *  Enlaza el descriptor del socket a la dirección y puerto
     */
    int bind()
    {
        return ::bind(sd, (const struct sockaddr *) &sa, sa_len);
    }

    int listen()
    {
        return ::listen(sd, 0);
    }

    /*int close()
    {
        return ::close(sd);
    }*/

    friend std::ostream& operator<<(std::ostream& os, const Socket& dt)
    {
        char host[NI_MAXHOST];
        char serv[NI_MAXSERV];

        getnameinfo((struct sockaddr *) &(dt.sa), dt.sa_len, host, NI_MAXHOST, serv,
                    NI_MAXSERV, NI_NUMERICHOST);

        os << host << ":" << serv;

        return os;
    };

    friend bool operator== (const Socket &s1, const Socket &s2)
    {
        //Comparar los campos sin_family, sin_addr.s_addr y sin_port
        //de la estructura sockaddr_in de los Sockets s1 y s2
        //Retornar false si alguno difiere

        struct sockaddr_in* nS1 = (sockaddr_in*) &(s1.sa);
        struct sockaddr_in* nS2 = (sockaddr_in*) &(s2.sa);

        return (nS1->sin_family == nS2->sin_family) && (nS1->sin_addr.s_addr == nS2->sin_addr.s_addr) && (nS1->sin_port == nS2->sin_port);

    };

protected:

    /**
     *  Descriptor del socket
     */
    int sd;

    /**
     *  Representación binaria del extremo, usada por servidor y cliente
     */
    struct sockaddr sa;
    socklen_t       sa_len;
};

#endif /* SOCKET_H_ */
