#include <vector>
#include <random>
#include <ctime>

typedef int card;

const int NUM_CARDS = 52; // Numero de cartas en el mazo
const int NUM_CARDS_RANK = 13; // Numero de cartas por palo

enum Suit {
    hearts,
    diamonds,
    spades,
    clubs
};

class Deck {
private:
    std::vector<card> deck;

public:
    Deck() {
        std::srand(std::time(nullptr));
        for (int i = 0; i < NUM_CARDS; i++)
            deck.push_back(i);
        shuffle();
    }

    void shuffle() {
        card aux;
        for (int i = 0; i < deck.size(); i++) {
            int pos = std::rand() % NUM_CARDS;
            aux = deck[pos];
            deck[pos] = deck[i];
            deck[i] = aux;
        }
    }

    card draw() {
        card top = deck.back();
        deck.pop_back();
        return top;
    }

    static bool SameSuit(card a, card b)
    {
        return (a / NUM_CARDS_RANK == b / NUM_CARDS_RANK );
    }

    static int HighestCard(card a, card b)
    {
        if(a % NUM_CARDS_RANK >= b % NUM_CARDS_RANK) return a;
        else return b;
    }
};