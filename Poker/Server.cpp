#include "Deck.cpp"

const int NUM_PLAYERS = 4;

int main(int arg, char *argv[])
{
    int connected = 0;
    int players[NUM_PLAYERS];
    
    // Esperar a que se conecten 4 jugadores.

    Poker game();

    return 0;
}

class Poker {
private:
    bool quit = false;
    int* players;
    Deck* deck;

    void deal() {
        for (int i = 0; i < NUM_PLAYERS; i++) {
            //players[i]->send(deck->draw());
            //players[i]->send(deck->draw());
        }
    };

public:
    Poker(int* play) {
        players = play;
        deck = new Deck();

        while (!quit) {
            deal();
        }
    }
};