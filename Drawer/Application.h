#pragma once

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <TGUI/TGUI.hpp>
#include <string>
#include <thread>
#include <chrono>

enum enumCoordinate
{
	CARTESIAN,
	POLAR,
	THREE_D
};

class Application
{
public:
	void init();
	int main();


private:
	void execute();
	void evaluate2D(std::vector<sf::Vector2f>& result, enumCoordinate coordinate);
	void evaluate3D(std::vector<sf::Vector3f>& result);
	void showGraph();
	void show3DGraph();
	void callbackTextEdit(tgui::TextBox::Ptr source);
	void fillDefaultSourceCode();
	void showBuiltInFunctions();
	void loadWidgets();
	sf::Vector2f convertGraphCoordToScreen(const sf::Vector2f& point) const;
	sf::Vector2f convertScreenCoordToGraph(const sf::Vector2f& point) const;
	std::vector<float> computeAxisGraduation(float min, float max) const;
	float getAccurateYValue(float x) const;
	sf::Color rainbowColor(float i);


	sf::RenderWindow mWindow;
	tgui::Gui mGui;
	tgui::TextBox::Ptr mSourceCodeEditBox;
	std::thread mThread;
	mutable sf::Mutex mMutex;
	std::string mSourceCode;
	std::vector<sf::Vector2f> mPoints2D;
	std::vector<sf::Vector3f> mPoints3D;
	sf::FloatRect mGraphRect = sf::FloatRect(-10.f, -10.f, 20.f, 20.f);
	sf::FloatRect mGraphScreen;
	sf::Text mErrorMessage;
	float mProgression = 0.f;
	bool mShowFunctionList = false;
	enumCoordinate mCoordinate = THREE_D;

};