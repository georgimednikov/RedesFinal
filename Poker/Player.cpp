#include <thread>
#include <vector>
#include "Serializable.h"
#include "Socket.h"
#include "Message.cpp"
#include "Texture.cpp"
#include "Font.cpp"
#include "Deck.cpp"

//Tamaños de la ventana
const int WIN_WIDTH = 1200;
const int WIN_HEIGHT = 900;

//Tamaños de las cartas
const int CARD_WIDTH = 125;
const int CARD_HEIGHT = 200;
const int CARD_OFFSET = 10;

//Archivo fuente del texto
const std::string FONT_SOURCE = "font.ttf";

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

    ~Player() { closeRender(); };

    //Informa al servidor de que se ha unido
    void login() {
        Message em(nicks[0], Message::LOGIN);
        socket.send(em, socket);
    }

    //Informa al servidor de que se ha desconectado
    void logout() {
        Message em(nicks[0], Message::LOGOUT);
        socket.send(em, socket);
    }

    //Hilo que procesa los inputs de este jugador
    void input_thread() {    
        while (true) {
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
            //else if (inst == "PASS") em.type = Message::PASS;
            //else if (inst == "BET") em.type = Message::BET;
            else if (inst == "DISCARD") {
                //Si se descarta hay que ver cuantas cartas se descartan y cuales para decirselo al servidor
                //DISCARD -> 0 descartes; DISCARD 1 -> Se descarta la de la izquierda
                //DISCARD 2 -> Se descarta la de la derecha; DISCARD 1 2 -> Se descartan ambas
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

    //Hilo que renderiza el estado del juego
    void render_thread() {
        while(true)
            //Solo renderiza cuando se está jugando
            if (state != UNSTARTED) {
                initRender();
                while (state != UNSTARTED) render();
            }
    }

    //Hilo que recibe y procesa los mensajes del servidor
    void net_thread() {
        while(true) {
            Message msg;
            socket.recv(msg, socket);
            std::cout << msg.nick << " " << (int)msg.type << " " << (int)msg.message1 << " " << (int)msg.message2 << std::endl;
            switch (msg.type)
            {
                case Message::LOGIN_INFO:
                {
                    //Se reciben los nicks de los jugadores en conjunto.
                    //Se guardan y se ordenan siguiendo el orden de turnos.
                    if (msg.nick == nicks[0]) continue;
                    nicks[usedNicks] = msg.nick; //Se entiende que el servidor no permite que se conecten mas de NUM_PLAYERS jugadores
                    usedNicks++;
                    //Cuando están todos comienza la partida.
                    if (usedNicks == NUM_PLAYERS) {
                        sortNicks();
                        state = PLAYING;
                    }
                    break;
                }
                case Message::DISCARD:
                {
                    //Message1 -> La carta descartada; Message2 -> La nueva carta
                    hands[0][msg.message1] = msg.message2;
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
                    hands[pos][0] = msg.message1, hands[pos][1] = msg.message2;
                    break;
                }
                case Message::CARD_TABLE:
                {    
                    cardsTable[cardNum] = msg.message1;
                    cardNum++;
                    break;
                }
                /*case Message::PASS:
                {                
                    int pos = searchNick(msg.nick);
                    if (pos == -1) std::cerr << "Invalid Nick: " << msg.nick << "\n";
                    for (int i = 0; i < 2; i++) hands[pos][i] = -1;
                    break;
                }*/
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
        UNSTARTED, //No ha empezado la partida / Se esperan jugadores
        PLAYING, //Se está jugando
        LOSE, //Has perdido
        WIN, //Has ganado
        DRAW //Has empatado
    };

    Socket socket;
    State state;

    //Arrays y contadores
    int usedNicks = 1;
    std::string nicks[NUM_PLAYERS];
    int cardNum = 0;
    int cardsTable[3];

    int hands[NUM_PLAYERS][2]; // -1 significa que no se conoce la carta
    int discarded[NUM_PLAYERS]; //Cuantas cartas ha descartado cada jugador

    SDL_Window* window;
    SDL_Renderer* renderer;
    Texture* texture;
    Font* font;

    //Dado un nick encuentra su índice en la lista
    int searchNick(std::string nc) {
        for (int i = 0; i < NUM_PLAYERS; ++i) {
            if (nicks[i] == nc) return i;
        }
        return -1;
    }

    //Ordena los nicks en base al orden de turnos
    void sortNicks() {
        std::string temp[NUM_PLAYERS];
        std::copy(nicks, nicks + NUM_PLAYERS, temp);
        int pos = searchNick(nicks[0]);
        for (int i = 0; i < NUM_PLAYERS; pos = ++pos % NUM_PLAYERS, cont++)
            nicks[i] = temp[pos];
    }

    //Se acaba la partida; se resetea todo el juego
    void resetGame() {
        resetState();
        usedNicks = 1;
        cardNum = 0;
        state = UNSTARTED;
        closeRender();
    }

    //Se acaba la ronda; se resetea el estado del juego
    void resetState() {
        for (int i = 0; i < NUM_PLAYERS; i++) {
            for (int j = 0; j < 2; j++) {
                hands[i][j] = -1;
            }
            discarded[i] = 0;
        }
        for (int i = 0; i < 3; i++)
            cardsTable[i] = 0;
        cardNum = 0;
        state = PLAYING;
    }

    //Inicializa SDL
    void initRender() {
        int winX, winY; // PosiciOn de la ventana
        winX = winY = SDL_WINDOWPOS_CENTERED;
        // InicializaciOn del sistema, ventana y renderer
        SDL_Init(SDL_INIT_EVERYTHING);
        window = SDL_CreateWindow("Poker De-lux", winX, winY, WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        
        TTF_Init();
        texture = new Texture(renderer, DECK_SOURCE);
        font = new Font(FONT_SOURCE, 30);
    }

    //Destruye SDL si se ha llegado a inicializar
    void closeRender() {
        if (state != UNSTARTED) {
            delete texture; 
            delete font;
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            TTF_Quit();
            SDL_Quit();
        }
    }

    //Se renderiza el estado del juego
    void render() {
        //Nueva iteracion nuevo frame
        SDL_SetRenderDrawColor(renderer, 48, 132, 70, 255);
        SDL_RenderClear(renderer); //Clear

        Texture* text;

        //Se colocan las cartas de cada jugador siguiendo una elipse
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
                texture->render(dest, angle * (180 / M_PI), source);
            }

            //Se colocan los textos de cada jugador
            int xPos = ax;// + (CARD_WIDTH + CARD_OFFSET * 2) * ( cos(angle));
            int yPos = ay;// + (CARD_WIDTH + CARD_OFFSET * 2) * (-sin(angle));

            text = new Texture(renderer, "Discards: " + std::to_string(discarded[i]), font);
            switch (i)
            {
            case 0:
                xPos += CARD_WIDTH + CARD_OFFSET * 2;
                break;
            case 1:
                xPos -= CARD_HEIGHT / 2 + CARD_OFFSET, yPos -= CARD_WIDTH + CARD_OFFSET * 2 + text->height_;
                break;
            case 2:
                xPos -= CARD_WIDTH + CARD_OFFSET * 2 + text->width_;
                break;
            case 3:
                xPos -= CARD_HEIGHT / 2, yPos += CARD_WIDTH + CARD_OFFSET * 2 + text->height_;
                break;
            }
            text->render(xPos, yPos);
            text = new Texture(renderer, nicks[i], font);
            text->render(xPos, yPos - text->height_);
        }

        //Cartas sobre la mesa
        int ax = WIN_WIDTH / 2 - (CARD_WIDTH + CARD_OFFSET) *  (cardNum / 2.0);
        dest.y = WIN_HEIGHT / 2 - CARD_HEIGHT / 2, dest.w = CARD_WIDTH, dest.h = CARD_HEIGHT;
        for (int j = 0; j < cardNum; j++) {
            dest.x = ax + (CARD_WIDTH + CARD_OFFSET * 2) * j;
            Deck::getCardCoor(cardsTable[j], source.x, source.y, source.w, source.h); 
            texture->render(dest, source);
        }

        delete text;
        SDL_RenderPresent(renderer); //Se dibuja el nuevo frame
    }
};

int main(int argc, char **argv) {
    Player ec(argv[1], argv[2], argv[3]);
    std::thread net_thread([&ec](){ ec.net_thread(); });
    std::thread render_thread([&ec](){ ec.render_thread(); });
    ec.login();
    ec.input_thread();
}