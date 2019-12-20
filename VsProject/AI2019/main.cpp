// BotDemo.cpp : Defines the entry point for the console application.
//
//#include "stdafx.h"
#include <iostream>
#include <sstream>
#include <time.h>
#include <vector>

//define for using math values like M_PI
#define _USE_MATH_DEFINES
#include <math.h>


#define MAX_GRENADE				3
#define MAX_KIT					3
#define MAX_BULLET				15

#define CHARACTER_RADIUS		150
#define CHARACTER_MOVE_SPEED 	45

//Should define more values here base on GDD
//...
//


using namespace std;

int MAP_WIDTH = 14000;
int MAP_HEIGHT = 9600;


enum Action
{
	WAIT = 0, //action do nothing
	MOVE, //action move on map
	USE, //action use a item you have which have a type
	PICK, //action pick a item on map which have a ID
	DROP //action drop a item on map which have a type
};

enum TypeGameObject
{
	GUN_BASIC,//object items
	GUN_AK,//object items
	GRENADE,//object items
	KIT,//object items
	ARMOR,//object items
	HELMET,//object items
	BULLETS_BOX,//object items
	CHEST,//object items
	CHARACTER,//object player
	BUSH,//object map
	ROCK,//object map
	FLYING_BULLET,//bullet shoot is moving
	FLYING_GRENADE,//grenade object is moving
	NONE_TYPE
};

class Weapon {
	TypeGameObject type;
	int activeRadius;
	int shootSpeed; // how many turn per shoot, minimum mean fast
	int flySpeed;
	int damage;
	int affectRadius;
	int maximumAmount;
	int amount;		// how many bullet

	Weapon(TypeGameObject type, int activeRadius, int shootSpeed, int flySpeed, int damage, int affectRadius, int maximumAmount, int amount) {
		this->type = type;
		this->activeRadius = activeRadius;
		this->shootSpeed = shootSpeed;
		this->flySpeed = flySpeed;
		this->damage = damage;
		this->affectRadius = affectRadius;
		this->maximumAmount = maximumAmount;
		this->amount = amount;
	}
};

class Vec2f
{
public:
	float x;
	float y;
	Vec2f() : x(0.0f), y(0.0f) {}
	Vec2f(float _x, float _y) : x(_x), y(_y) {}
	Vec2f operator =(Vec2f pos) { x = pos.x; y = pos.y; return *this; }
	Vec2f operator-(Vec2f& v) { return Vec2f(x - v.x, y - v.y); }
	Vec2f& operator*=(double s) { x *= s; y *= s; return *this; }
	float length() const { return std::sqrt(x * x + y * y); }
	Vec2f normalize() const
	{
		Vec2f tmp = *this;
		float lengthTmp = tmp.length();
		if (lengthTmp == 0)
			return tmp;
		tmp *= (1.0 / lengthTmp);
		return tmp;
	}
	static float dot(Vec2f v1, Vec2f v2)
	{
		return v1.x * v2.x + v1.y * v2.y;
	}

	float DistanceTo(Vec2f a) {
		Vec2f b(a.x - this->x, a.y - this->y);
		return b.length();
	}
};

struct PlayerActionParam
{
	Action actionID;
	int itemID; //it is item's ID when action Pick //it is item's type when action use
	float paramX;
	float paramY;
} g_playerAction;

//game's object information
struct Object
{
public:
	Object() :ID(0), m_type(0), r(0), distanceToPlayer(-1) {}
	int ID;
	Vec2f m_pos;
	Vec2f m_direction; //Vector Direction of bullet or grenade's moving, |m_direction| is NOT speed of bullet/grenade
	unsigned int m_type;
	float r;

	float distanceToPlayer;
};

class Player : public Object //player's information
{
public:
	Player() : m_hp(100), m_AK_Bullets(0), m_numberKit(0), m_numberGrenade(0), m_haveArmor(false), m_haveHelmet(false), Object() {}
	float m_hp; //current hp // max 100
	unsigned int m_AK_Bullets; //current  AK-47's number bullet //max 15 
	unsigned int m_numberKit; //current medical bags //max 3
	unsigned int m_numberGrenade; //current grenade you have //max 3
	bool m_haveArmor; //equipped armor
	bool m_haveHelmet; //equipped helmet
	int turnToCircle; // how many turn took to the safe area inside the circle

	int CalculateTurnToCircle(Circle cl) {

	}
};

class Circle // game's circle information
{
public:
	Circle() :m_currentRadius(0), m_turnCountDown(0), m_hpReduce(0.0f), m_currentCloseSpeed(0), m_nextRadius(0) {}
	Vec2f m_currentPosition; //green circle's position
	Vec2f m_nextPosition; //white circle's position // green circle will move to this position when it start to close
	float m_currentRadius; //green circle's radius
	float m_hpReduce; //player lose their hp when outside the circle on turn
	float m_currentCloseSpeed;
	float m_nextRadius; //white circle's radius // green circle will close to this radius when it start to close
	unsigned int m_turnCountDown; //turn remain to it start to close //when it equal 0 it mean circle is closing
};

//helper function
float RandFloat(float M, float N);
int Rand(int min, int max);
Vec2f GenObjectPositionInsideArea(Vec2f center, int radius);
void MoveRandomInsideCircle(int turn, Vec2f greenCirclePos, float greenCircleRadius);
//helper function end

//Actions set
void SetWaitAction(); //action do nothing
void SetMoveAction(Vec2f tagetPos); //action move to x, y
void SetUseAction(TypeGameObject typeItem, Vec2f tagetPos); //use a item at x,y taget
void SetPickAction(unsigned int itemID); //pick a item with a ID
void SetDropAction(TypeGameObject typeItem, int number); //drop item
void PushCommandToServer();
//end Actions


Player g_player; //your player information
bool isInitialize = false;
unsigned int g_currentTurn = 0; //current game's turn
unsigned int g_numberPlayer = 0;
unsigned int g_nAlive = 0; //number player alive
unsigned int nMapObjects = 0;



Circle g_gameCircle; //green circle and white circle information

vector<Object> g_objectsMap; //objects on map when game initialized 
vector<Object> g_objectsVisible; //game's objects (object, items, other players) in your visible area (radius 1500 around your position)
vector<Object> g_lastVisibleEnemies;
vector<Object> g_currentVisibleEnemies;
vector<Object> g_rocks;
vector<Object> g_bushs;


void ReadInitInfo()
{
	//number player in game // your player id and it's position on map at first turn and current player information
	std::cin >> g_currentTurn >> g_numberPlayer >> g_player.ID >> g_player.m_pos.x >> g_player.m_pos.y >> g_player.m_hp >> g_player.m_AK_Bullets >> g_player.m_haveHelmet >> g_player.m_haveArmor >> g_player.m_numberKit >> g_player.m_numberGrenade;
	std::cin >> nMapObjects;
	g_objectsMap.reserve(nMapObjects);
	for (size_t i = 0; i < nMapObjects; i++) //objects (rocks and bushs) information on map when game initialized 
	{
		Object object;
		std::cin >> object.ID >> object.m_type >> object.m_pos.x >> object.m_pos.y >> object.r; //it's id, type, position, radius
		g_objectsMap.push_back(object);
	}
}

void UpdateMapInfoEachTurn()
{
	//use std::cin for check, make sure game server alway send correct and enough information

	//get current player information
	std::cin >> g_currentTurn >> g_nAlive >> g_player.m_pos.x >> g_player.m_pos.y >> g_player.m_hp >> g_player.m_AK_Bullets >> g_player.m_haveHelmet >> g_player.m_haveArmor >> g_player.m_numberKit >> g_player.m_numberGrenade;

	//get Visible objects information
	//all objects in player's view
	unsigned int numberObjectVisible = 0;
	std::cin >> numberObjectVisible;
	g_objectsVisible.clear();
	g_objectsVisible.reserve(numberObjectVisible);
	for (unsigned int i = 0; i < numberObjectVisible; i++)
	{
		Object object;
		std::cin >> object.ID >> object.m_type >> object.m_pos.x >> object.m_pos.y >> object.m_direction.x >> object.m_direction.y; //visible object's id, type, position
		g_objectsVisible.push_back(object);
		
		// visible enemy;
		if (object.m_type == TypeGameObject::CHARACTER) {
			g_currentVisibleEnemies.push_back(object);
		}
	}

	//get game circle information
	std::cin >> g_gameCircle.m_currentPosition.x >> g_gameCircle.m_currentPosition.y >> g_gameCircle.m_currentRadius >> g_gameCircle.m_turnCountDown >> g_gameCircle.m_currentCloseSpeed >> g_gameCircle.m_hpReduce >> g_gameCircle.m_nextPosition.x >> g_gameCircle.m_nextPosition.y >> g_gameCircle.m_nextRadius; //visible object's id, type, position

}

//Get time
#include <chrono>
#include <ctime>
using namespace std::chrono;
uint64_t timeSinceEpochMillisec() {
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

std::vector<Weapon> g_weapons;

void InitWeaponInfo() {
	
}

int main()
{
	//init timer rand
	srand(time(NULL));

	//***************************************************************
	// Write an action using std::cout or printf()					|
	// To debug log: std::cerr << "Debug messages here...";			|
	//***************************************************************

	std::cerr << "Debug messages here..." << endl;

	while (true)
	{
		uint64_t t_start = timeSinceEpochMillisec();

		if (!isInitialize)
		{
			ReadInitInfo();
			isInitialize = true;
		}
		else
		{
			UpdateMapInfoEachTurn();
		}

		uint64_t t_readInput = timeSinceEpochMillisec();
		std::cerr << "Time read input: " << (t_readInput - t_start) << endl;

		//Simple bot demo
		std::vector<Object*> playerVisible; //players in your visible area
		std::vector<Object*> playerShotVisible;//players shot in your listenable area but you don't see him so you don't know that player's id //if that player in your visible area, it will not in this list
		std::vector<Object*> itemVisible; //items in your visible area
		std::vector<Object*> bulletsVisible; //bullets shot in your visible area
		std::vector<Object*> grenadeVisible; //grenades threw in your visible area

		for (size_t i = 0; i < g_objectsVisible.size(); i++)
		{
			if (g_objectsVisible[i].m_type == TypeGameObject::CHARACTER)
			{
				if (g_objectsVisible[i].ID != -1)
				{
					playerVisible.push_back(&(g_objectsVisible[i]));
				}
				else
				{
					playerShotVisible.push_back(&(g_objectsVisible[i]));
				}
			}
			else if (g_objectsVisible[i].m_type == TypeGameObject::ARMOR
				|| g_objectsVisible[i].m_type == TypeGameObject::HELMET
				|| g_objectsVisible[i].m_type == TypeGameObject::KIT
				|| g_objectsVisible[i].m_type == TypeGameObject::GRENADE
				|| g_objectsVisible[i].m_type == TypeGameObject::BULLETS_BOX
				|| g_objectsVisible[i].m_type == TypeGameObject::CHEST)
			{
				//items can pick
				itemVisible.push_back(&(g_objectsVisible[i]));
			}
			else if (g_objectsVisible[i].m_type == TypeGameObject::FLYING_BULLET)
			{
				//moving bullet
				bulletsVisible.push_back(&(g_objectsVisible[i]));
			}
			else if (g_objectsVisible[i].m_type == TypeGameObject::FLYING_GRENADE)
			{
				//moving grenade
				grenadeVisible.push_back(&(g_objectsVisible[i]));
			}
		}

		//Make a shoot
		if (playerVisible.size())
		{
			SetUseAction(TypeGameObject::GUN_BASIC, playerVisible[0]->m_pos);
		}
		else
		{
			//set a MOVE action
			MoveRandomInsideCircle(g_currentTurn, g_gameCircle.m_currentPosition, g_gameCircle.m_currentRadius);
		}

		//YOU MUST SEND COMMAND TO SERVER!
		//Send players' action to server		
		PushCommandToServer();

		uint64_t t_end = timeSinceEpochMillisec();
		std::cerr << "Total Time Execute: " << (t_end - t_start) << endl;

	}//end while


	return 0;
}



void SetWaitAction()
{
	g_playerAction.actionID = Action::WAIT;
	g_playerAction.paramX = 0; //should reset to 0
	g_playerAction.paramY = 0; //should reset to 0
}

void SetMoveAction(Vec2f tagetPos)
{
	g_playerAction.actionID = Action::MOVE;
	g_playerAction.paramX = tagetPos.x;
	g_playerAction.paramY = tagetPos.y;
}

void SetUseAction(TypeGameObject typeItem, Vec2f tagetPos) //position where you want to shot or where you want throw grenade
{
	g_playerAction.actionID = Action::USE;
	g_playerAction.itemID = typeItem;	//type item on your equipment want to use //type Gun, Kit, grenade
										//item kit will restore hp on yourself so it no need taget's positon but keep the format
	g_playerAction.paramX = tagetPos.x;
	g_playerAction.paramY = tagetPos.y;
}

void SetPickAction(unsigned int itemID)
{
	g_playerAction.actionID = Action::PICK;
	g_playerAction.itemID = itemID; //item's ID you want to pick
}

void SetDropAction(TypeGameObject typeItem, int number)
{
	g_playerAction.actionID = Action::DROP;
	g_playerAction.itemID = typeItem; //type item on your equipment want to drop //type bullets AK, kits, grenades
	g_playerAction.paramX = number; //number items on your equipment want to drop //number bullets AK, number kits, number grenades
}

void MoveRandomInsideCircle(int turn, Vec2f greenCirclePos, float greenCircleRadius)
{
	static Vec2f randomPos = GenObjectPositionInsideArea(greenCirclePos, greenCircleRadius);
	if (turn % 30 == 0)
	{
		randomPos = GenObjectPositionInsideArea(greenCirclePos, greenCircleRadius);
	}
	SetMoveAction(randomPos);
}

void PushCommandToServer() {
	//best performance with one-line printf out.
	//Must keep enter '\n' or endline at the end of command
	printf("%d %d %f %f\n", g_playerAction.actionID, g_playerAction.itemID, g_playerAction.paramX, g_playerAction.paramY);
}

float RandFloat(float M, float N)
{
	return M + (rand() / (RAND_MAX / (N - M)));
}

int Rand(int min, int max)
{
	return min + rand() % (max - min);
}

Vec2f GenObjectPositionInsideArea(Vec2f center, int radius)
{
	if (radius == 0)
	{
		return center;
	}
	Vec2f genPosition;

	int randomRadius = Rand(0, radius);
	float randomAngle = RandFloat(0, 2 * M_PI);

	genPosition.x = round(randomRadius * cos(randomAngle)) + center.x;
	genPosition.y = round(randomRadius * sin(randomAngle)) + center.y;

	return genPosition;
}
