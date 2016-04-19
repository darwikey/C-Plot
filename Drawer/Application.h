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
	bool evaluate2D(std::vector<sf::Vector2f>& result, enumCoordinate coordinate);
	bool evaluate3D(std::vector<sf::Vector3f>& result, int& curveWidth);
	void ApplyZoomOnGraph(float factor);
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
	std::thread* mThread = nullptr;
	mutable sf::Mutex mMutex;
	std::string mSourceCode;
	std::vector<sf::Vector2f> mPoints2D;
	std::vector<sf::Vector3f> mPoints3D;
	int mCurveWidth = 32;
	int mNumPoint2D = 1024;
	int mNumPoint3D = 32;
	sf::FloatRect mGraphRect = sf::FloatRect(-10.f, -10.f, 20.f, 20.f);
	sf::FloatRect mGraphScreen;
	sf::Text mErrorMessage;
	float mProgression = 0.f;
	bool mShowFunctionList = false;
	enumCoordinate mCoordinate = THREE_D;

};