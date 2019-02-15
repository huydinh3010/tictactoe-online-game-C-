#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <thread>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include <iterator>
#include <Windows.h>
#define PORT 2222

using namespace sf;
using namespace std;	

bool id[1000] = {};
mutex m;
class Game {
public:
	/*TcpSocket * socket1;
	TcpSocket * socket2;*/
	int matrix[3][3] = {};
	void set(int x, int y, int value) {
		matrix[y][x] = value;
	}
	void clear() {
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				matrix[i][j] = 0;
	}
	int checkWin(int value) {
		if (matrix[0][0] == value && matrix[0][1] == value && matrix[0][2] == value) return 1;
		if (matrix[1][0] == value && matrix[1][1] == value && matrix[1][2] == value) return 1;
		if (matrix[2][0] == value && matrix[2][1] == value && matrix[2][2] == value) return 1;
		if (matrix[0][0] == value && matrix[1][0] == value && matrix[2][0] == value) return 1;
		if (matrix[0][1] == value && matrix[1][1] == value && matrix[2][1] == value) return 1;
		if (matrix[0][2] == value && matrix[1][2] == value && matrix[2][2] == value) return 1;
		if (matrix[0][0] == value && matrix[1][1] == value && matrix[2][2] == value) return 1;
		if (matrix[2][0] == value && matrix[1][1] == value && matrix[0][2] == value) return 1;
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				if (matrix[i][j] == 0) return -1; // chưa kết thúc
			}
		}
		return 0; // hòa
	}
}; 
vector<Game*> listGames;

class Player {
public:
	Game * game;
	TcpSocket * socket;
	Player * opponent;
	int id;
	string name;
	bool inMatch;
	bool rematch;
	Player(TcpSocket * socket, int id) {
		this->socket = socket;
		this->id = id;
		this->inMatch = false;
		this->rematch = false;
	}
	~Player() {
		if (game != NULL) {
			m.lock();
			vector<Game *>::iterator it = find(listGames.begin(), listGames.end(), game);
			if (it != listGames.end()) listGames.erase(it);
			m.unlock();
			delete game;
			cout << "A game is delete" << endl;
		}
		delete socket;
		if (opponent != NULL) {
			opponent->opponent = NULL;
			opponent->game = NULL;
		}
	}
};

vector<Player *> listPlayers; // danh sách người kết nối với server
vector<Player *> players; // danh sách những người đang tìm đối thủ



void listenFromSocket(Player * player) {
	do {
		Packet packet;
		if (player->socket->receive(packet) != Socket::Done) {
			if (player->inMatch && player->opponent != NULL) {
				Packet p;
				p << "opponent_out";
				player->opponent->socket->send(p);
				player->opponent->opponent = NULL;
				player->opponent->game = NULL;
			}
			m.lock();
			vector<Player*>::iterator it;
			it = find(listPlayers.begin(), listPlayers.end(), player);
			if (it != listPlayers.end()) listPlayers.erase(it);
			it = find(players.begin(), players.end(), player);
			if (it != players.end()) players.erase(it);
			Packet packet;
			packet << "players" << listPlayers.size() << listGames.size();
			for (int i = 0; i < listPlayers.size(); i++) {
				if (!listPlayers[i]->inMatch) listPlayers[i]->socket->send(packet);
			}
			m.unlock();
			id[player->id] = false;
			delete player;
			//player = NULL;
			cout << "A player is disconnect" << endl;
			break;
		}
		string str;
		packet >> str;
		if (str == "set_name") packet >> player->name;
		else if (str == "player") {
			Packet packet;
			m.lock();
			int list_size = listPlayers.size();
			int games = listGames.size();
			m.unlock();
			packet << "players" << list_size << games;
			player->socket->send(packet);
		}
		else if (str == "find_opponent") {
			m.lock();
			players.push_back(player);
			m.unlock();
		}
		else if (str == "players") {
			m.lock();
			Packet p;
			p << listPlayers.size() << listGames.size();
			player->socket->send(p);
			m.unlock();
		}
		else if (str == "set") {
			int x, y, value;
			packet >> x >> y >> value;
			player->game->set(x, y, value);
			Packet pk;
			pk << "set" << x << y << value;
			if(player->opponent != NULL) player->opponent->socket->send(pk);
			int k = player->game->checkWin(value);
			if (k == 0) {
				Packet p;
				p << "draw";
				player->socket->send(p);
				if (player->opponent != NULL) player->opponent->socket->send(p);
			}
			else if (k == 1) {
				Packet p1;
				p1 << "you_win";
				player->socket->send(p1);
				Packet p2;
				p2 << "you_lose";
				if (player->opponent != NULL) player->opponent->socket->send(p2);
			}
		}
		else if (str == "rematch") {
			player->rematch = true;
			if (player->opponent != NULL && player->opponent->rematch) {
				Packet p;
				p << "rematch";
				player->socket->send(p);
				player->opponent->socket->send(p);
				player->rematch = false;
				player->opponent->rematch = false;
				player->game->clear();
			}
		}
		else if (str == "exit_game") {
			Packet p1;
			p1 << "exit";
			player->socket->send(p1);
			player->inMatch = false;
			// del game
			if (player->game != NULL) {
				m.lock();
				vector<Game *>::iterator it = find(listGames.begin(), listGames.end(), player->game);
				if (it != listGames.end()) listGames.erase(it);
				m.unlock();
				delete player->game;
				player->game = NULL;
				cout << "A game is delete" << endl;
				
			}
			
			//player->game = NULL;
			if (player->opponent != NULL) {
				Packet p2;
				p2 << "opponent_out";
				player->opponent->socket->send(p2);
				player->opponent->opponent = NULL;
				player->opponent->game = NULL;
				player->opponent = NULL;
			}
		}
	} while (true);
}

void setGame() {
	do {
		Sleep(100);
		m.lock();
		int k = players.size();
		m.unlock();
		if (k > 1) {
			// trong vector players có thể có con trỏ null?
			m.lock();
 			Player * player1 = players.back();
			players.pop_back();
			Player * player2 = players.back();
			players.pop_back();
			m.unlock();
			// tạo game
			player1->inMatch = true;
			player2->inMatch = true;
			Game * game = new Game();
			m.lock();
			listGames.push_back(game);
			m.unlock();
			player1->game = game;
			player2->game = game;
			player1->opponent = player2;
			player2->opponent = player1;
			// gửi thông tin đối thủ cho 2 bên
			bool a = rand() % 2;
			Packet packet1, packet2;
			packet1 << "your_opponent" << player2->id << player2->name << a;
			packet2 << "your_opponent" << player1->id << player1->name << !a;
			player1->socket->send(packet1);
			player2->socket->send(packet2);
			m.lock();
			Packet packet;
			packet << "players" << listPlayers.size() << listGames.size();
			for (int i = 0; i < listPlayers.size(); i++) {
				if (!listPlayers[i]->inMatch) listPlayers[i]->socket->send(packet);
			}
			m.unlock();
			//
		}
	} while (true);
}

int main(){
	srand(time(NULL));
	TcpListener listener;
	if (listener.listen(PORT) != Socket::Done) {
		cout << "Loi khi lang nghe ket noi" << endl;
		system("pause");
	}
	thread th(setGame);
	
	do {
		
		TcpSocket *client = new TcpSocket();
		if (listener.accept(*client) != Socket::Done) {
			cout << "Loi khi nhan ket noi tu client" << endl;
			delete client;
		}
		else {
			int idClient;
			do {
				idClient = rand() % 999 + 1;
			} while (id[idClient]);
			id[idClient] = true;
			Player * player = new Player(client, idClient);
			
			m.lock();
			listPlayers.push_back(player);
			int size_p = listPlayers.size();
			int size_g = listGames.size();
			m.unlock();
			Packet packet;
			packet << "id" << idClient;
			client->send(packet);
			packet.clear();
			packet << "players" << size_p << size_g;
			m.lock();
			for (int i = 0; i < listPlayers.size(); i++) {
				if (!listPlayers[i]->inMatch) listPlayers[i]->socket->send(packet);
			}
			m.unlock();
			cout << "A player connected to server. Id: " << idClient << endl;
			thread * fun = new thread(listenFromSocket, player);
		}
	} while (true);
	system("pause");
	return 0;
}