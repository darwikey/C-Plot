#include "Application.h"
#include "picoc.h"

#define NUM_POINTS 1024

void Application::init()
{
	// Create the window
	mWindow.create(sf::VideoMode(1000, 700), "C-Plot");
	mWindow.setFramerateLimit(60);
	mGui.setWindow(mWindow);

	try
	{
		// Load the widgets
		loadWidgets();
	}
	catch (const tgui::Exception& e)
	{
		std::cerr << "Failed to load TGUI widgets: " << e.what() << std::endl;
		system("pause");
		return ;
	}

	// launch a thread with the parser
	mThread = std::thread(&Application::execute, this);
}

int Application::main()
{
	// Main loop
	sf::Clock timer;
	bool drag = false;
	sf::Vector2f dragPosition;
	sf::Vector2i dragMousePosition;
	while (mWindow.isOpen())
	{
		//***************************************************
		// Events and inputs
		//***************************************************
		sf::Event event;
		while (mWindow.pollEvent(event))
		{
			// When the window is closed, the application ends
			if (event.type == sf::Event::Closed)
				mWindow.close();

			// When the window is resized, the view is changed
			else if (event.type == sf::Event::Resized)
			{
				mWindow.setView(sf::View(sf::FloatRect(0, 0, (float)event.size.width, (float)event.size.height)));
				mGui.setView(mWindow.getView());
			}

			// Pass the event to all the widgets
			mGui.handleEvent(event);
		}

		// Zoom in
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::PageUp) && timer.getElapsedTime().asMilliseconds() > 10)
		{
			timer.restart();
			sf::Vector2f center(mGraphRect.left + 0.5f * mGraphRect.width, mGraphRect.top + 0.5f * mGraphRect.height);
			sf::Lock lock(mMutex);
			mGraphRect.width *= 1.02f;
			mGraphRect.height *= 1.02f;
			mGraphRect.left = center.x - 0.5f * mGraphRect.width;
			mGraphRect.top = center.y - 0.5f * mGraphRect.height;
		}
		// Zoom out
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::PageDown) && timer.getElapsedTime().asMilliseconds() > 10)
		{
			timer.restart();
			sf::Vector2f center(mGraphRect.left + 0.5f * mGraphRect.width, mGraphRect.top + 0.5f * mGraphRect.height);
			sf::Lock lock(mMutex);
			mGraphRect.width *= 0.98f;
			mGraphRect.height *= 0.98f;
			mGraphRect.left = center.x - 0.5f * mGraphRect.width;
			mGraphRect.top = center.y - 0.5f * mGraphRect.height;
		}
		// mouse
		if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
		{
			if (drag)
			{
				sf::Vector2i delta = sf::Mouse::getPosition() - dragMousePosition;
				const float sensibility = 0.001f;
				mGraphRect.left = dragPosition.x - delta.x * sensibility * mGraphRect.width;
				mGraphRect.top = dragPosition.y + delta.y * sensibility * mGraphRect.height;
			}
			else
			{
				drag = true;
				dragPosition = sf::Vector2f(mGraphRect.left, mGraphRect.top);
				dragMousePosition = sf::Mouse::getPosition();
			}
		}
		else
		{
			drag = false;
		}


		//***************************************************
		// Rendering
		//***************************************************
		mWindow.clear();

		// Draw all created widgets
		mGui.draw();

		// Curve
		mGraphScreen = sf::FloatRect(mGui.getSize().x * 0.25f + 30.f, 100.f, mGui.getSize().x * 0.65f, mGui.getSize().y - 200.f);

		if (mShowFunctionList)
		{
			showBuiltInFunctions();
		}
		else
		{
			showGraph();
		}

		// Display messages
		mMutex.lock();
		mErrorMessage.setPosition(30, mGui.getSize().y - 100);
		mWindow.draw(mErrorMessage);
		mMutex.unlock();

		// Progression bar
		sf::RectangleShape bar (sf::Vector2f(mProgression * 0.25f * mGui.getSize().x, 3.f));
		bar.setPosition(0, 15);
		bar.setFillColor(sf::Color(50, 50, 255));
		bar.setOutlineThickness(1.f);
		bar.setOutlineColor(sf::Color::Blue);
		mWindow.draw(bar);

		mWindow.display();
	}

	mThread.detach();
	return EXIT_SUCCESS;
}

void Application::execute()
{
	std::vector<sf::Vector2f> result;
	char errorBuffer[1024];
	while (1)
	{
		result.clear();

		mMutex.lock();
		float width = mGraphRect.width;
		float start = mGraphRect.left;
		std::string buffer = mSourceCode;
		bool polarCoordinate = mPolarCoordinate;
		mMutex.unlock();
		int isCrash = 0;

		for (int i = 0; i < NUM_POINTS; i++)
		{
			double x = (double)i / NUM_POINTS;
			mProgression = (float)x;
			if (polarCoordinate)
			{
				x *= 6.283185307179586;
			}
			else
			{
				x = x * width + start;
			}

			float y = (float)parse(buffer.c_str(), x, &isCrash, errorBuffer);
			if (isCrash)
			{
				break;
			}
			//if (i % 30 == 0)
			//std::cout << "x";

			result.push_back(sf::Vector2f((float)x, y));
		}

		//std::cout << std::endl;

		mMutex.lock();
		if (polarCoordinate == mPolarCoordinate)
		{
			mPoints = result;
		}
		if (isCrash)
			mErrorMessage.setString(errorBuffer);
		else
			mErrorMessage.setString(sf::String());
		mMutex.unlock();
		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void Application::showGraph()
{
	std::vector<sf::Vertex> lines;
	mMutex.lock();
	for (const sf::Vector2f& p : mPoints)
	{
		if (mPolarCoordinate)
		{
			sf::Vector2f p(p.y * cos(p.x), p.y * sin(p.x));
			lines.push_back(convertGraphCoordToScreen(p));
		}
		else // cartesian coordinate
		{
			lines.push_back(convertGraphCoordToScreen(p));
		}
	}
	mMutex.unlock();
	mGui.getWindow()->draw(lines.data(), lines.size(), sf::LinesStrip);
	lines.clear();

	// Axis
	std::vector<sf::Vertex> axis;
	// horizontal
	float middleY = 1.f + mGraphRect.top / mGraphRect.height;
	lines.push_back(sf::Vector2f(mGraphScreen.left, mGraphScreen.top + middleY*mGraphScreen.height));
	lines.push_back(sf::Vector2f(mGraphScreen.left + mGraphScreen.width, mGraphScreen.top + middleY*mGraphScreen.height));
	//vertical
	float middleX = -mGraphRect.left / mGraphRect.width;
	lines.push_back(sf::Vector2f(mGraphScreen.left + middleX*mGraphScreen.width, mGraphScreen.top));
	lines.push_back(sf::Vector2f(mGraphScreen.left + middleX*mGraphScreen.width, mGraphScreen.top + mGraphScreen.height));

	std::vector<float> graduation = computeAxisGraduation(mGraphRect.left, mGraphRect.left + mGraphRect.width);
	const float graduationSize = 2.f;
	for (float x : graduation)
	{
		char str[32];
		sprintf_s<32>(str, "%g", x);
		sf::Text text(str, *mGui.getFont(), 12);
		x = (x - mGraphRect.left) / mGraphRect.width;
		text.setPosition(mGraphScreen.left + x * mGraphScreen.width, mGraphScreen.top + middleY*mGraphScreen.height - graduationSize);
		mGui.getWindow()->draw(text);

		lines.push_back(sf::Vector2f(mGraphScreen.left + x * mGraphScreen.width, mGraphScreen.top + middleY*mGraphScreen.height + graduationSize));
		lines.push_back(sf::Vector2f(mGraphScreen.left + x * mGraphScreen.width, mGraphScreen.top + middleY*mGraphScreen.height - graduationSize));
	}

	graduation = computeAxisGraduation(mGraphRect.top, mGraphRect.top + mGraphRect.height);
	for (float y : graduation)
	{
		char str[32];
		sprintf_s<32>(str, "%g", y);
		sf::Text text(str, *mGui.getFont(), 12);
		y = (y - mGraphRect.top) / mGraphRect.height;
		text.setPosition(mGraphScreen.left + middleX*mGraphScreen.width + graduationSize + 1.f, mGraphScreen.top + (1.f - y) * mGraphScreen.height - 5.f);
		mWindow.draw(text);

		lines.push_back(sf::Vector2f(mGraphScreen.left + middleX*mGraphScreen.width + graduationSize, mGraphScreen.top + (1.f - y) * mGraphScreen.height));
		lines.push_back(sf::Vector2f(mGraphScreen.left + middleX*mGraphScreen.width - graduationSize, mGraphScreen.top + (1.f - y) * mGraphScreen.height));
	}
	mWindow.draw(lines.data(), lines.size(), sf::Lines);

	sf::Vector2f mouse = convertScreenCoordToGraph(sf::Vector2f((float)sf::Mouse::getPosition(mWindow).x, (float)sf::Mouse::getPosition(mWindow).y));
	
	if (mouse.x >= mGraphRect.left && mouse.x <= mGraphRect.left + mGraphRect.width)
	{
		if (mPolarCoordinate)
		{
			mouse.x = atan2(mouse.y, mouse.x);
			if (mouse.x < 0)
				mouse.x += 6.283185307179586f;
		}

		float y = getAccurateYValue(mouse.x);
		char str[64];
		sprintf_s<64>(str, "(%g, %g)", mouse.x, y);
		sf::Text text(str, *mGui.getFont(), 12);
		sf::Vector2f textPos;
		if (!mPolarCoordinate)
		{
			textPos = convertGraphCoordToScreen(sf::Vector2f(0.f, y));
			textPos.x = (float)sf::Mouse::getPosition(mWindow).x;
		}
		else
		{
			textPos = sf::Vector2f(y * cos(mouse.x), y * sin(mouse.x));
			textPos = convertGraphCoordToScreen(textPos);
		}
		text.setPosition(textPos);
		mWindow.draw(text);
		sf::RectangleShape rect(sf::Vector2f(3.f,3.f));
		rect.setPosition(textPos.x - 1.5f, textPos.y - 1.5f);
		rect.setFillColor(sf::Color(128,128,255));
		mWindow.draw(rect);
	}
}

void Application::callbackTextEdit(tgui::TextBox::Ptr source)
{
	mMutex.lock();
	mSourceCode = source->getText().toAnsiString();
	mMutex.unlock();
}

void Application::showBuiltInFunctions()
{
	std::string list;
	GetBuiltInFunction(list);
	const char* str = list.c_str();

	sf::Text text("", *mGui.getFont(), 12);
	text.setPosition(mGui.getSize().x * 0.25f + 30.f, 30.f);
	text.setColor(sf::Color::White);

	const char* strEnd = str + list.length();

	for (const char* splitEnd = str; splitEnd != strEnd; ++splitEnd)
	{
		if (*splitEnd == '\n')
		{
			const ptrdiff_t splitLen = splitEnd - str;
			text.setString(std::string(str, splitLen));
			str = splitEnd + 1;

			sf::Vector2f pos = text.getPosition();
			pos.y += 15.f;
			if (pos.y > mGui.getSize().y - 50)
			{
				pos = sf::Vector2f(pos.x + 250.f, 30.f);
			}
			text.setPosition(pos);
			mGui.getWindow()->draw(text);
		}
	}
}

void Application::loadWidgets()
{
	// Load the black theme
	auto theme = std::make_shared<tgui::Theme>();// "TGUI/widgets/Black.txt");

	auto windowWidth = tgui::bindWidth(mGui);
	auto windowHeight = tgui::bindHeight(mGui);


	// Create the username edit box
	tgui::TextBox::Ptr editBox = theme->load("TextBox");
	editBox->setSize(windowWidth * 0.25f, windowHeight - 200);
	editBox->setPosition(10, 30);
	editBox->setText("double main(double x){\n\nreturn x;\n}");
	mGui.add(editBox, "Code");

	editBox->connect("TextChanged", &Application::callbackTextEdit, this, editBox);
	callbackTextEdit(editBox);

	// Create the login button
	tgui::Button::Ptr button = theme->load("Button");
	button->setSize(windowWidth * 0.25f, 25);
	button->setPosition(10, windowHeight -150);
	button->setText("Show built-in functions");
	mGui.add(button);
	button->connect("pressed", [this] {
		mShowFunctionList = !mShowFunctionList;
	});

	tgui::CheckBox::Ptr coordinateBox = theme->load("CheckBox");
	coordinateBox->setSize(20, 20);
	coordinateBox->setPosition(windowWidth * 0.25f + 60.f, 10.f);
	coordinateBox->setText("Polar coordinates");
	mGui.add(coordinateBox);
	coordinateBox->connect("checked", [this]() {
		mPoints.clear();
		mPolarCoordinate = true;
	});
	coordinateBox->connect("unchecked", [this]() {
		mPoints.clear();
		mPolarCoordinate = false;
	});

	mErrorMessage.setFont(*mGui.getFont());
	mErrorMessage.setCharacterSize(13);
	mErrorMessage.setColor(sf::Color::Red);
}

sf::Vector2f Application::convertGraphCoordToScreen(const sf::Vector2f& point) const
{
	sf::Vector2f p((point.x - mGraphRect.left) / mGraphRect.width, (point.y - mGraphRect.top) / mGraphRect.height);
	return sf::Vector2f(mGraphScreen.left + p.x * mGraphScreen.width, mGraphScreen.top + (1.f - p.y) * mGraphScreen.height);
}

sf::Vector2f Application::convertScreenCoordToGraph(const sf::Vector2f& point) const
{
	sf::Vector2f p((point.x - mGraphScreen.left) / mGraphScreen.width, (point.y - mGraphScreen.top) / mGraphScreen.height);
	return sf::Vector2f(p.x * mGraphRect.width + mGraphRect.left, (1.f-p.y) * mGraphRect.height + mGraphRect.top);
}

std::vector<float> Application::computeAxisGraduation(float min, float max) const
{
	float delta = max - min;
	const static double mul[] = { 1.0, 2.0, 5.0 };
	std::vector<float> axis;

	bool ok = false;
	double step = FLT_MAX;
	for (int e = -7; e < 9; e++)
	{
		double a = pow(10.0, e);

		for (int i = 0; i < 3; i++)
		{
			double b = a * mul[i];

			if (delta / b <= 10)
			{
				step = b;
				ok = true;
				break;
			}
		}
		if (ok)
		{
			break;
		}
	}

	double i = floor(min / step) * step;
	for (; i < max + 0.1*step; i += step)
	{
		if (abs(i) > 1e-9)
		{
			axis.push_back((float)i);
		}
	}

	return axis;
}

float Application::getAccurateYValue(float x) const
{
	if (mPoints.size() < 2)
		return 0.f;

	sf::Lock lock(mMutex);

	sf::Vector2f p0 = mPoints[0];
	sf::Vector2f p1 = mPoints[1];
	for (unsigned i = 1; i < mPoints.size(); i++)
	{
		if (mPoints[i].x > x)
		{
			p0 = mPoints[i-1];
			p1 = mPoints[i];
			break;
		}
	}

	float a = (x - p0.x) / (p1.x - p0.x);
	return a * (p1.y - p0.y) + p0.y;
}
