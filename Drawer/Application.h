#pragma once

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
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
	void showGraph();
	void callbackTextEdit(tgui::TextBox::Ptr source);
	void showBuiltInFunctions();
	void loadWidgets();
	sf::Vector2f convertGraphCoordToScreen(const sf::Vector2f& point) const;
	sf::Vector2f convertScreenCoordToGraph(const sf::Vector2f& point) const;
	std::vector<float> computeAxisGraduation(float min, float max) const;
	float getAccurateYValue(float x) const;


	sf::RenderWindow mWindow;
	tgui::Gui mGui;
	std::thread mThread;
	mutable sf::Mutex mMutex;
	std::string mSourceCode;
	std::vector<sf::Vector2f> mPoints;
	sf::FloatRect mGraphRect = sf::FloatRect(-10.f, -10.f, 20.f, 20.f);
	sf::FloatRect mGraphScreen;
	sf::Text mErrorMessage;
	float mProgression = 0.f;
	bool mShowFunctionList = false;
	enumCoordinate mCoordinate = CARTESIAN;

};