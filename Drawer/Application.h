#pragma once

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <TGUI/TGUI.hpp>
#include <thread>
#include <chrono>
#include "Tweakable.h"

enum enumCoordinate
{
	CARTESIAN,
	POLAR,
	THREE_D
};

enum enumDragMode
{
	NO_DRAG,
	DRAG_X,
	DRAG_Y,
	DRAG_XY,
	DRAG_DELIMITATOR
};

class Application
{
public:
	void init();
	int main();


private:
	void               execute();
	bool               evaluate2D(std::vector<sf::Vector2f>& result, enumCoordinate coordinate);
	bool               evaluate3D(std::vector<sf::Vector3f>& result, int& curveWidth);
	void               ApplyZoomOnGraph(float factor);
	void               showGraph();
	void               show3DGraph();
	void               callbackTextEdit();
	void               fillDefaultSourceCode();
	void               showBuiltInFunctions();
	void               callbackAddTweakable(tgui::EditBox::Ptr editbox);
	void               addTweakableButtons(size_t index, const std::string& tweakableName);
	void               updateTweakable(bool rebuildUI);
	void               showTweakableSettings(const std::string& currentTweakable);
	void               loadWidgets();
	sf::Vector2f       convertGraphCoordToScreen(const sf::Vector2f& point) const;
	sf::Vector2f       convertScreenCoordToGraph(const sf::Vector2f& point) const;
	bool               isMouseOverXAxis();
	bool               isMouseOverYAxis();
	bool               isMouseOverDelimitator();
	std::vector<float> computeAxisGraduation(float min, float max) const;
	float              getAccurateYValue(float x) const;
	sf::Color          rainbowColor(float i);


	sf::RenderWindow          mWindow;
	tgui::Gui                 mGui;
	tgui::VerticalLayout::Ptr mMainContainer;
	tgui::TextBox::Ptr        mTweakableEditBox;
	tgui::TextBox::Ptr        mSourceCodeEditBox;
	tgui::Panel::Ptr          mAddTweakableBox;
	std::thread*              mThread = nullptr;
	mutable sf::Mutex         mMutex;
	std::string               mSourceCode;
	std::list<std::string>    mSourceCodeHistory;
	std::list<std::string>    mSourceCodeRedo;
	bool                      mSourceDirty = true;
	std::vector<sf::Vector2f> mPoints2D;
	std::vector<sf::Vector3f> mPoints3D;
	int                       mCurveWidth = 32;
	int                       mNumPoint2D = 1024;
	int                       mNumPoint3D = 32;
	float                     mDelimitatorRatio = 0.25f;
	sf::FloatRect             mGraphRect = sf::FloatRect(-10.f, -10.f, 20.f, 20.f);
	sf::FloatRect             mGraphScreen;
	sf::Text                  mErrorMessage;
	float                     mProgression = 0.f;
	bool                      mShowFunctionList = false;
	enumCoordinate            mCoordinate = CARTESIAN;
	std::vector<Tweakable>    mTweakables;
	std::string               mCurrentTweakable;
};