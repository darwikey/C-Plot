#pragma once

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <string>
#include <thread>
#include <chrono>


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
	std::vector<float> computeAxisGraduation(float min, float max);


	sf::RenderWindow mWindow;
	tgui::Gui mGui;
	std::thread mThread;
	sf::Mutex mMutex;
	std::string mSourceCode;
	std::vector<sf::Vector2f> mPoints;
	sf::FloatRect mGraphRect = sf::FloatRect(-10.f, -10.f, 20.f, 20.f);
	sf::FloatRect mGraphScreen;
	sf::Text mErrorMessage;
	float mProgression = 0.f;
	bool mShowFunctionList = false;

};