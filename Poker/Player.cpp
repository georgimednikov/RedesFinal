#include <thread>
#include "Serializable.h"
#include "Socket.h"
#include "Message.cpp"
#include "Texture.cpp"
#include "Font.cpp"
#include "Deck.cpp"

const int NUM_PLAYERS = 4;

const int WIN_WIDTH = 1200;
const int WIN_HEIGHT = 900;

const int CARD_WIDTH = 125;
const int CARD_HEIGHT = 200;
const int CARD_OFFSET = 10;

const std::string FONT_SOURCE = "./RetroGaming.ttf";

class Player
{
public:
    /**
     * @param s dirección del servidor
     * @param p puerto del servidor
     * @param n nick del usuario
     */
    Player(const char * s, const char * p, char * n)  {
        socket = Socket(s, p, false);
        nicks[0] = n;
        resetGame();
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
            std::string inp;
            std::getline(std::cin, inp);
            size_t space_pos = inp.find(" ");
            std::string inst; 
            if (space_pos != std::string::npos) {
                inst = inp.substr(0, space_pos);
                inp = inp.substr(space_pos + 1);
            }
            else inst = inp;

            Message em = Message();
            em.nick = nicks[0];
            if (inst == "LOGOUT") em.type = Message::LOGOUT;
            else if (inst == "PASS") em.type = Message::PASS;
            else if (inst == "BET") em.type = Message::BET;
            else if (inst == "login") em.type = Message::LOGIN;
            else if (inst == "DISCARD") {
                em.type = Message::DISCARD;
                space_pos = inp.find(" ");
                if (inst == inp);
                else if (space_pos == std::string::npos) {
                    em.message1 = std::stoi(inp);
                }
                else {
                    em.message1 = std::stoi(inp.substr(0, space_pos));
                    inp = inp.substr(space_pos + 1);
                    em.message2 = std::stoi(inp);
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
        //initRender();
        while(true)
        {
            //render();
            Message msg;
            socket.recv(msg, socket);
            std::cout << msg.nick << " " << (int)msg.type << " " << (int)msg.message1 << " " << (int)msg.message2 << std::endl;
            switch (msg.type)
            {
                case Message::LOGIN_INFO:
                {
                    if (msg.nick == nicks[0]) continue;
                    nicks[usedNicks] = msg.nick; //Se entiende que el servidor no permite que se conecten mas de NUM_PLAYERS jugadores
                    usedNicks++;
                    break;
                }
                case Message::DISCARD_INFO:
                {                
                    int pos = searchNick(msg.nick);
                    if (pos == -1) std::cerr << "Invalid Nick: " << msg.nick << "\n";
                    discarded[pos] = msg.message1;
                    break;
                }
                case Message::CARDS:
                {
                    int pos = searchNick(msg.nick);
                    if (pos == -1) std::cerr << "Invalid Nick: " << msg.nick << "\n";
                    for (int i = 0; i < msg.message1; i++) hands[pos][i] = true;
                    break;
                }
                case Message::CARD_TABLE:
                {    cardsTable.push_back(msg.message1);
                    break;
                }
                case Message::PASS:
                {                
                    int pos = searchNick(msg.nick);
                    if (pos == -1) std::cerr << "Invalid Nick: " << msg.nick << "\n";
                    for (int i = 0; i < 2; i++) hands[pos][i] = -1;
                    break;
                }
                case Message::END_ROUND:
                {    
                    resetState();
                    break;
                }
                case Message::LOGOUT:
                {
                    resetGame();
                    break;
                }
                case Message::WINNER:
                {
                    if (msg.nick == "Server") state = DRAW;
                    if (msg.nick == nicks[0]) state = WIN;
                    else state = LOSE;
                }
            }
        }
    }


private:

    enum State {
        PLAYING,
        LOSE,
        WIN,
        DRAW
    };

    /**
     * Estado del juego
     */
    State state;

    /**
     * Socket para comunicar con el servidor
     */
    Socket socket;

    /**
     * Nicks de los jugadores
     */
    int usedNicks = 1;
    std::string nicks[NUM_PLAYERS];

    /**
     * Manos de los jugadores
     */
    int hands[NUM_PLAYERS][2]; // -1 significa que no se conoce la carta, -2 que se ha retirado

    /**
     * Descartes de los jugadores
     */
    int discarded[NUM_PLAYERS];

    /**
     * Cartas en la mesa
     */
    std::vector<int> cardsTable;

    int searchNick(std::string nc) {
        for (int i = 0; i < NUM_PLAYERS; ++i) {
            if (nicks[i] == nc) return i;
        }
        return -1;
    }

    void resetGame() {
        resetState();
        usedNicks = 1;
    }

    void resetState() {
        for (int i = 0; i < NUM_PLAYERS; i++) {
            for (int j = 0; j < 2; j++) {
                hands[i][j] = -1;
            }
            discarded[i] = 0;
        }
        cardsTable.clear();
        state = PLAYING;
    }

    void initRender() {
        int winX, winY; // PosiciOn de la ventana
        winX = winY = SDL_WINDOWPOS_CENTERED;
        // InicializaciOn del sistema, ventana y renderer
        SDL_Init(SDL_INIT_EVERYTHING);
        window = SDL_CreateWindow("Poker De-lux", winX, winY, WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        
        texture = new Texture(renderer, DECK_SOURCE);
        font = new Font(FONT_SOURCE, 10);
    }

    void render() {
        SDL_SetRenderDrawColor(renderer, 48, 132, 70, 255);
        SDL_RenderClear(renderer); //Clear

        double angle = 0;
        SDL_Rect dest, source; dest.w = CARD_WIDTH; dest.h = CARD_HEIGHT;
        for (int i = 0; i < NUM_PLAYERS; i++) {
            angle = i * (360 / NUM_PLAYERS);
            angle *= (M_PI / 180);

            int ax = ( WIN_WIDTH / 2 - CARD_HEIGHT/2 - CARD_OFFSET) * sin(angle) + (WIN_WIDTH / 2), 
                ay = ( WIN_HEIGHT / 2 - CARD_HEIGHT/2 - CARD_OFFSET) * cos(angle) + (WIN_HEIGHT / 2);
            for (int j = 0; j < 2; j++) {
                if (hands[i][j] < -1) break;
                dest.x = ax + (((- 1 + j * 2) * (CARD_OFFSET + (CARD_WIDTH) * ( 1 - j )) + CARD_WIDTH / 2) * (cos(angle)));
                dest.y = ay + (((- 1 + j * 2) * (CARD_OFFSET + (CARD_WIDTH) * ( 1 - j )) + CARD_WIDTH / 2) * (-sin(angle)));

                if(cos(angle) == 0) dest.w = CARD_HEIGHT, dest.h = CARD_WIDTH;

                dest.x -= dest.w / 2;
                dest.y -= dest.h / 2;

                Deck::getCardCoor(hands[i][j], source.x, source.y, source.w, source.h); 
                //texture->render(0, 0);
            }

            Texture* nick = new Texture(renderer, nicks[i], font);
            dest.x = 0, dest.y = 0, dest.w = 0, dest.h = 0;
            texture->render(dest, source);
        }

        int ax = WIN_WIDTH / 2 - (CARD_WIDTH + CARD_OFFSET) *  (cardsTable.size() / 2.0);
        dest.y = WIN_HEIGHT / 2 - CARD_HEIGHT / 2, dest.w = CARD_WIDTH, dest.h = CARD_HEIGHT;
        for (int j = 0; j < cardsTable.size(); j++) {
            dest.x = ax + (CARD_WIDTH + CARD_OFFSET * 2) * j;
            Deck::getCardCoor(cardsTable[j], source.x, source.y, source.w, source.h); 
            texture->render(dest, source);
        }

        SDL_RenderPresent(renderer); //Draw
    }

    SDL_Window* window;
    SDL_Renderer* renderer;
    Texture* texture;
    Font* font;
};

int main(int argc, char **argv)
{
    Player ec(argv[1], argv[2], argv[3]);
    std::thread net_thread([&ec](){ ec.net_thread(); });
    ec.login();
    ec.input_thread();
}