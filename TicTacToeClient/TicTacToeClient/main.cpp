#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <string>
#include<thread>
#include<Windows.h>
#define IP "127.0.0.1"
#define PORT 2222
#define WINDOW_NAME "TicTacToe"



using namespace sf;
using namespace std;
bool close = false;
int id = -1;
string name = "";
int opponent_id;
string opponent_name;
bool turn = true;
bool changeState = false;

class State {
protected:
	Font font;
	TcpSocket * socket;
public:
	virtual void runThread() = 0;
	virtual void processEvent(Event &event) = 0;
	virtual void draw(RenderWindow * window) = 0;
};

class Intro : public State {
private:
	bool finding;
	string strPlayer;
	Socket::Status status;
	string strStatus;
	string statusRec;
	bool isConnected;
	bool nameSeted;
	void connectToServer() {
		
		if(!isConnected && socket->connect(IP, PORT) != Socket::Done) strStatus = "Cannot connect to server!";
		else {
			if (isConnected) {
				Packet packet;
				packet << "players";
				socket->send(packet);
			}
			isConnected = true;
			strStatus = "Connected! ";
			do {
				Packet packet;
				if (socket->receive(packet) != Socket::Done) {
					strStatus = "Disconnected!";
					isConnected = false;
					nameSeted = false;
					break;
				}
				string str;
				packet >> str;
				if (str == "id") {
					packet >> id;
					strStatus += "Your id: " + to_string(id);
				}
				else if (str == "players") {
					int players;
					int matches;
					packet >> players >> matches;
					strPlayer = "Players: " + to_string(players) + " | Matches: " + to_string(matches);
				}
				else if (str == "your_opponent") {
					packet >> opponent_id >> opponent_name >> turn;
					changeState = true;
					finding = false;
					statusRec = "Random Match";
					break;
				}
			} while (true);
		}
	}
public:
	Intro(TcpSocket & socket) {
		this->font.loadFromFile("Font/BrushScriptStd.otf");
		this->strStatus = "Connecting to server...";
		this->isConnected = false;
		this->nameSeted = false;
		this->statusRec = "";
		this->socket = &socket;
		this->finding = false;
	}
	void runThread() {
		connectToServer();
	}
	void draw(RenderWindow * window) {
		window->clear(Color(249, 204, 118));
		RectangleShape recOutline(Vector2f(312, 42));
		recOutline.setPosition(69, 259);
		recOutline.setFillColor(Color::Red);
		window->draw(recOutline);

		RectangleShape rec(Vector2f(300, 30));
		rec.setPosition(75, 265);
		if (nameSeted) rec.setFillColor(Color::Blue);
		else rec.setFillColor(Color::White);
		window->draw(rec);
		// ten game
		Text text;
		text.setFont(font);
		text.setCharacterSize(40);
		text.setFillColor(Color::Black);
		text.setString("TicTacToe");
		text.setPosition(225 - text.getLocalBounds().width / 2, 10);
		window->draw(text);
		int height = text.getLocalBounds().height;
		// ten tac gia
		text.setCharacterSize(24);
		text.setString("Code by Nguyen Huy Dinh");
		text.setPosition(225 - text.getLocalBounds().width / 2, height + 45);
		window->draw(text);
		height += text.getLocalBounds().height;
		// status
		text.setString(strStatus);
		text.setPosition(225 - text.getLocalBounds().width / 2, height + 80);
		window->draw(text);
		height += text.getLocalBounds().height;
		// so nguoi choi, so tran
		text.setString(strPlayer);
		text.setPosition(225 - text.getLocalBounds().width / 2, height + 100);
		window->draw(text);
		// nickname
		if (!nameSeted) text.setString("Your Nickname:");
		else text.setString("Your Nickname: " + name);
		text.setPosition(225 - text.getLocalBounds().width / 2, height + 150);
		window->draw(text);
		// name
		if (isConnected) {
			if (!nameSeted) text.setString(name + '|');
			else text.setString(statusRec);
			text.setPosition(225 - text.getLocalBounds().width / 2, 263);
			window->draw(text);
		}
		window->display();
	}
	void processEvent(Event &event) {
		if (event.type == Event::KeyPressed) {
			if (isConnected && !nameSeted) {
				if (event.key.code >= Keyboard::A && event.key.code <= Keyboard::Z && name.length() <= 15)
					name += 'a' + event.key.code;
				else if (event.key.code == Keyboard::BackSpace && name.length() > 0)
					name.pop_back();
				else if (event.key.code == Keyboard::Enter && name.length() > 0) {
					// dang nhap thanh cong
					Packet packet;
					packet << "set_name" << name;
					socket->send(packet);
					nameSeted = true;
					statusRec = "Random Match";
				}
			}
		}
		else if (event.type == Event::MouseButtonPressed) {
			if (event.mouseButton.button == Mouse::Left && nameSeted && !finding) {
				int x = event.mouseButton.x;
				int y = event.mouseButton.y;
				if (x >= 75 && x <= 375 && y >= 265 && y <= 295) {
					Packet packet;
					packet << "find_opponent";
					socket->send(packet);
					statusRec = "Finding opponent...";
					finding = true;
				}
			}
		}
	}	
};

class Game : public State {
private:
	bool rematchButton;
	bool backButton;
	bool opponent;
	bool showBox;
	string strBox = "Wait for opponent";
	int value;
	bool yourTurn;
	int matrix[3][3] = {};
	void drawBackGround(RenderWindow * window) {
		RectangleShape line1(Vector2f(80, 6));
		line1.setOrigin(Vector2f(40, 3));
		line1.setFillColor(Color::Yellow);
		line1.rotate(90);
		line1.setPosition(222,40);
		window->draw(line1);
		RectangleShape line2(Vector2f(450, 6));
		line2.setOrigin(Vector2f(0, 3));
		line2.setFillColor(Color::Yellow);
		line2.setPosition(0, 80);
		window->draw(line2);
		for (int i = 0; i < 4; i++) {
			RectangleShape line(Vector2f(450, 6));
			line.setOrigin(Vector2f(0, 3));
			line.setPosition(0, i * 150 + 100);
			line.setFillColor(Color(0, 140, 94));
			window->draw(line);
		}
		for (int j = 0; j < 4; j++) {
			RectangleShape line(Vector2f(450, 6));
			line.setOrigin(Vector2f(0, 3));
			line.setPosition(j * 150, 100);
			line.setFillColor(Color(0, 140, 94));
			line.rotate(90);
			window->draw(line);
		}
		Text text;
		// id
		text.setFont(font);
		text.setCharacterSize(24);
		text.setFillColor(Color::Black);
		text.setPosition(10, 0);
		text.setString("id: " + to_string(id));
		window->draw(text);
		// name
		text.setString(name);
		text.setPosition(10, 30);
		window->draw(text);
		// opponent id
		if (opponent) {
			text.setString("id: " + to_string(opponent_id));
			text.setPosition(450 - text.getLocalBounds().width - 10, 0);
			window->draw(text);
			// opponent name
			text.setString(opponent_name);
			text.setPosition(450 - text.getLocalBounds().width - 10, 30);
			window->draw(text);
		}
		// turn
		text.setString("turn");
		text.setFillColor(Color::Red);
		if (yourTurn) text.setPosition(225 - text.getLocalBounds().width - 10, 0);
		else text.setPosition(230, 0);
		window->draw(text);
		// box
	}
	void drawMatrix(RenderWindow * window) {
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				if (matrix[i][j] == 1) drawX(window, j, i);
				else if (matrix[i][j] == -1) drawO(window, j, i);
	}
	void drawX(RenderWindow * window, int x, int y) {
		RectangleShape line1(Vector2f(150, 6));
		line1.setFillColor(Color::Blue);
		line1.rotate(45);
		line1.setOrigin(75, 3);
		line1.setPosition(x * 150 + 75, y * 150 + 175);
		RectangleShape line2(Vector2f(150, 6));
		line2.setFillColor(Color::Blue);
		line2.rotate(-45);
		line2.setOrigin(75, 3);
		line2.setPosition(x * 150 + 75, y * 150 + 175);
		window->draw(line1);
		window->draw(line2);
	}
	void drawO(RenderWindow * window, int x, int y) {
		CircleShape circle(52, 100);
		circle.setFillColor(Color(249, 204, 118));
		circle.setOrigin(52, 52);
		circle.setOutlineThickness(6);
		circle.setOutlineColor(Color::Red);
		circle.setPosition(x * 150 + 75, y * 150 + 175);
		window->draw(circle);
	}
	void drawBox(RenderWindow * window) {
		RectangleShape shape(Vector2f(300, 150));
		shape.setFillColor(Color::White);
		shape.setOrigin(150, 75);
		shape.setPosition(225, 275);
		window->draw(shape);
		Text textButton;
		textButton.setFont(font);
		// strBox
		textButton.setCharacterSize(24);
		textButton.setString(strBox);
		textButton.setFillColor(Color::Black);
		textButton.setPosition(225 - textButton.getLocalBounds().width / 2, 250);
		window->draw(textButton);
		// button
		textButton.setCharacterSize(30);
		if (rematchButton) textButton.setFillColor(Color::Blue);
		else textButton.setFillColor(Color::Cyan);
		textButton.setStyle(Text::Underlined | Text::Bold);
		textButton.setString("Rematch");
		textButton.setPosition(100, 300);
		window->draw(textButton);
		//cout << textButton.getPosition().x << " " << textButton.getPosition().y << endl;
		//cout << textButton.getLocalBounds().width << " " << textButton.getLocalBounds().height << endl;
		//
		if (backButton) textButton.setFillColor(Color::Blue);
		else textButton.setFillColor(Color::Cyan);
		textButton.setString("Back");
		textButton.setPosition(290, 300);
		window->draw(textButton);
		/*cout << textButton.getPosition().x << " " << textButton.getPosition().y << endl;
		cout << textButton.getLocalBounds().width << " " << textButton.getLocalBounds().height << endl;*/
	}
	void listenFromSocket() {
		do {
			Packet packet;
			if (socket->receive(packet) != Socket::Done) {
				changeState = true;
				break;
			}
			string str;
			packet >> str;
			if (str == "set") {
				int x, y;
				packet >> x >> y;
				matrix[y][x] = -value;
				yourTurn = true;
			}
			else if (str == "exit") { // chấp nhận yc 
				changeState = true;
				break;
			}
			else if (str == "you_win") {
				showBox = true;
				strBox = "You Win";
				if (opponent) rematchButton = true;
				else rematchButton = false;
			}
			else if (str == "draw") {
				showBox = true;
				strBox = "Draw";
				if (opponent) rematchButton = true;
				else rematchButton = false;
			}
			else if (str == "you_lose") {
				showBox = true;
				strBox = "You Lose";
				if (opponent) rematchButton = true;
				else rematchButton = false;
			}
			else if (str == "rematch") {
				packet >> yourTurn;
				for (int i = 0; i < 3; i++)
					for (int j = 0; j < 3; j++)
						matrix[i][j] = 0;
				this->showBox = false;
				this->opponent = true;
				this->backButton = true;
				this->rematchButton = true;
				if (yourTurn) this->value = 1;
				else this->value = -1;
			}
			else if (str == "opponent_out") {
				showBox = true;
				strBox = "Your opponent is out!";
				rematchButton = false;
				opponent = false;
			}
		} while (true);
	}
	
public:
	Game(TcpSocket &socket) {
		this->socket = &socket;
		this->font.loadFromFile("Font/BrushScriptStd.otf");
		if (turn) this->value = 1;
		else this->value = -1;
		this->yourTurn = turn;
		this->showBox = false;
		this->opponent = true;
		this->backButton = true;
		this->rematchButton = true;
	}
	void processEvent(Event &event) {
		if (event.type == Event::MouseButtonPressed) {
			int x = event.mouseButton.x;
			int y = event.mouseButton.y;
			if (event.mouseButton.button == Mouse::Left && yourTurn && !showBox) {
				x = x / 150;
				y = (y - 100) / 150;
				if (x >= 0 && x <= 2 && y >= 0 && y <= 2 && matrix[y][x] == 0) {
					matrix[y][x] = value;
					Packet packet;
					packet << "set" << x << y << value;
					socket->send(packet);
					yourTurn = false;
				}
			}
			else if (showBox) {
				// bam vao nut
				// rematch 100 300 - 110 24;
				// back 290 300 - 65 23
				if (rematchButton && x >= 100 && x <= 210 && y >= 300 && y <= 324) {
					// bam vao nut rematch
					Packet packet;
					packet << "rematch";
					socket->send(packet);
					strBox = "Wait for opponent...";
					rematchButton = false;
				}
				else if (backButton && x >= 290 && x <= 355 && y >= 300 && y <= 323) {
					// bam vao nut back
					// gui toi server
					Packet packet;
					packet << "exit_game";
					socket->send(packet);
					backButton = false;
				}
			}
		}
	}
	void draw(RenderWindow * window) {
		window->clear(Color(249, 204, 118));
		drawBackGround(window);
		drawMatrix(window);
		if (showBox) drawBox(window);
		window->display();
	}
	void runThread() {
		listenFromSocket();
	}
};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow){
	RenderWindow window(VideoMode(450, 550), WINDOW_NAME, Style::Close);
	TcpSocket socket;
	State * stateIntro = new Intro(socket);
	State * state = stateIntro;
	thread * t_thread = new thread(&State::runThread, state);
	while (window.isOpen()) {
		Event event;
		while (window.pollEvent(event)) {
			if (event.type == Event::Closed) {
				socket.disconnect();
				window.close();
			}
			state->processEvent(event);
		}
		state->draw(&window);
		if (changeState) {
			if (state == stateIntro) state = new Game(socket);
			else {
				delete state;
				state = stateIntro;
				
			}
			t_thread->join();
			delete t_thread;
			t_thread = new thread(&State::runThread, state);
			changeState = false;
		}
		Sleep(20);
	}
	t_thread->join();
	return 0;
}