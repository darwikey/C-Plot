#include "Application.h"
#include "picoc.h"
#include <iostream>

void Application::init()
{
	std::random_device rd;
	mRandomGenerator.seed(rd());

	// Create the window
	mWindow.create(sf::VideoMode(1000, 700), "C-Plot", 7, sf::ContextSettings(24, 8, 8));
	mWindow.setFramerateLimit(60);

	mWindow.setActive();

	// Enable Z-buffer read and write
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glClearDepth(1.f);

	// Disable lighting
	glDisable(GL_LIGHTING);

	// Setup a perspective projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	GLfloat ratio = static_cast<float>(mWindow.getSize().x) / mWindow.getSize().y;
	glFrustum(-ratio, ratio, -1.f, 1.f, 1.f, 500.f);

	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

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
	mThread = new std::thread(&Application::execute, this);
}

int Application::main()
{
	// Main loop
	sf::Clock timer;
	enumDragMode drag = NO_DRAG;
	sf::Vector2f dragPosition;
	sf::Vector2f dragMousePosition;
	sf::FloatRect dragGraphRect = mGraphRect;
	size_t dragPointIndex = 0;

	while (mWindow.isOpen())
	{
		//***************************************************
		// Events and inputs
		//***************************************************
		sf::Event event;
		int mouseWheel = 0;
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
				glViewport(0, 0, event.size.width, event.size.height);
				glViewport((GLsizei)(event.size.width * 0.25f), 0, (GLsizei)(event.size.width*0.75f), event.size.height);
			}
			else if (event.type == sf::Event::MouseWheelMoved)
			{
				mouseWheel = event.mouseWheel.delta;
			}
			else if (event.type == sf::Event::KeyPressed)
			{
				// Undo
				if (event.key.code == sf::Keyboard::Z && event.key.control && !mSourceCodeHistory.empty())
				{
					mMutex.lock();
					mSourceCodeRedo.push_back(mSourceCode);
					if (mSourceCodeRedo.size() > 50) // limit the size of the history
						mSourceCodeRedo.pop_front();
					mSourceCode = mSourceCodeHistory.back();
					mSourceDirty = true;
					mSourceCodeHistory.pop_back();
					mMutex.unlock();
					mSourceCodeEditBox->setText(mSourceCode);
				}
				//Redo
				if (event.key.code == sf::Keyboard::Y && event.key.control && !mSourceCodeRedo.empty())
				{
					mMutex.lock();
					mSourceCodeHistory.push_back(mSourceCode);
					if (mSourceCodeHistory.size() > 50)//limit the size of the history
						mSourceCodeHistory.pop_front();
					mSourceCode = mSourceCodeRedo.back();
					mSourceDirty = true;
					mSourceCodeRedo.pop_back();
					mMutex.unlock();
					mSourceCodeEditBox->setText(mSourceCode);
				}
			}

			// Pass the event to all the widgets
			mGui.handleEvent(event);
		}

		// Zoom in
		if (mWindow.hasFocus() && (float)sf::Mouse::getPosition(mWindow).x > mDelimitatorRatio * mWindow.getSize().x)
		{
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::PageUp) && timer.getElapsedTime().asMilliseconds() > 10)
			{
				timer.restart();
				ApplyZoomOnGraph(1.02f);
			}
			// Zoom out
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::PageDown) && timer.getElapsedTime().asMilliseconds() > 10)
			{
				timer.restart();
				ApplyZoomOnGraph(0.98f);
			}
			if (mouseWheel != 0)
			{
				ApplyZoomOnGraph(1.f + 0.15f * mouseWheel);
			}
		}
		// mouse
		sf::Vector2f mousePosition((float)sf::Mouse::getPosition(mWindow).x, (float)sf::Mouse::getPosition(mWindow).y);
		if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && mWindow.hasFocus() && mousePosition.y < mWindow.getSize().y - 100.f)
		{
			if (drag != NO_DRAG)
			{
				sf::Lock lock(mMutex);
				sf::Vector2f delta = mousePosition - dragMousePosition;
				const float sensibility = 0.001f;
				switch (drag)
				{
				case DRAG_XY:
					mGraphRect.left = dragPosition.x - delta.x * sensibility * mGraphRect.width;
					mGraphRect.top = dragPosition.y + delta.y * sensibility * mGraphRect.height;
					break;

				case DRAG_X:
				{
					float center = dragGraphRect.left + 0.5f * dragGraphRect.width;
					mGraphRect.width = dragGraphRect.width * pow(2.f, delta.x * 0.01f);
					mGraphRect.left = center - 0.5f * mGraphRect.width;
				}
				break;

				case DRAG_Y:
				{
					float center = dragGraphRect.top + 0.5f * dragGraphRect.height;
					mGraphRect.height = dragGraphRect.height * pow(2.f, delta.y * 0.01f);
					mGraphRect.top = center - 0.5f * mGraphRect.height;
				}
				break;

				case DRAG_DELIMITATOR:
					mDelimitatorRatio = mousePosition.x / mWindow.getSize().x;
					mDelimitatorRatio = std::max(std::min(mDelimitatorRatio, 0.9f), 0.f);
					mMainContainer->setSize(mDelimitatorRatio * tgui::bindWidth(mGui) - 20, tgui::bindHeight(mGui));
					break;

				case DRAG_POINT:
					if (dragPointIndex < mPoints.size())
					{
						mPoints[dragPointIndex] = convertScreenCoordToGraph(mousePosition);
					}
					break;
				}
				mSourceDirty = true;
			}
			else 
			{
				bool isMouseOverPoint = false;
				for (size_t i = 0; i < mPoints.size(); i++)
				{
					sf::Vector2f pointOnScreen = convertGraphCoordToScreen(mPoints[i]);
					if (fabs(pointOnScreen.x - mousePosition.x) + fabs(pointOnScreen.y - mousePosition.y) < 5.5f)
					{
						dragPointIndex = i;
						isMouseOverPoint = true;
					}
				}

				if (isMouseOverPoint)
				{
					drag = DRAG_POINT;
				}
				else if (isMouseOverDelimitator())
				{
					drag = DRAG_DELIMITATOR;
				}
				else if (mousePosition.x > mDelimitatorRatio * mWindow.getSize().x)
				{
					if (isMouseOverXAxis())
						drag = DRAG_X;
					else if (isMouseOverYAxis())
						drag = DRAG_Y;
					else
						drag = DRAG_XY;
				}
				dragPosition = sf::Vector2f(mGraphRect.left, mGraphRect.top);
				dragMousePosition = mousePosition;
				dragGraphRect = mGraphRect;
			}
		}
		else
		{
			drag = NO_DRAG;
		}


		//***************************************************
		// Rendering
		//***************************************************
		mWindow.clear();

		// Draw all created widgets
		mWindow.pushGLStates();

		// delimitator
		sf::RectangleShape delimitator(sf::Vector2f(5.f, (float)mWindow.getSize().y));
		delimitator.setPosition(mWindow.getSize().x * mDelimitatorRatio, 0);
		delimitator.setFillColor(sf::Color(128, 128, 128));
		mWindow.draw(delimitator);

		// Curve
		mGraphScreen = sf::FloatRect(mWindow.getSize().x * mDelimitatorRatio + 30.f, 100.f, mWindow.getSize().x * (1.f - mDelimitatorRatio) - 100.f, mWindow.getSize().y - 200.f);

		if (mShowFunctionList)
		{
			showBuiltInFunctions();
		}
		else if (mCoordinate == THREE_D)
		{
			show3DGraph();
		}
		else
		{
			showGraph();
		}

		// Display messages
		mMutex.lock();
		mErrorMessage.setPosition(30.f, mWindow.getSize().y - 70.f);
		mWindow.draw(mErrorMessage);
		mMutex.unlock();

		// Progression bar
		sf::RectangleShape bar (sf::Vector2f(mProgression * mMainContainer->getSize().x, 3.f));
		bar.setPosition(10, 15);
		bar.setFillColor(sf::Color(50, 50, 255));
		bar.setOutlineThickness(1.f);
		bar.setOutlineColor(sf::Color::Blue);
		mWindow.draw(bar);

		// UI
		mGui.draw();

		mWindow.popGLStates();
		mWindow.display();
	}

	if (mThread)
		mThread->detach();
	return EXIT_SUCCESS;
}

void Application::execute()
{
	std::vector<sf::Vector2f> result2D;
	std::vector<sf::Vector3f> result3D;
	
	while (1)
	{
		while (!mSourceDirty)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		mSourceDirty = false;

		mMutex.lock();
		enumCoordinate coordinate = mCoordinate;
		mMutex.unlock();
		int curveWidth = 0;

		if (coordinate != THREE_D)
		{
			result2D.clear();
			if (evaluate2D(result2D, coordinate))
			{
				continue;
			}
		}
		else // 3D curve
		{
			result3D.clear();
			if (evaluate3D(result3D, curveWidth))
			{
				continue;
			}
		}

		mMutex.lock();
		if (coordinate == mCoordinate)
		{
			if (coordinate != THREE_D)
			{
				mPoints2D = result2D;
			}
			else // 3d curve
			{
				mPoints3D = result3D;
				mCurveWidth = curveWidth;
			}
		}
		mMutex.unlock();
	}
}

bool Application::evaluate2D(std::vector<sf::Vector2f>& result, enumCoordinate coordinate)
{
	mMutex.lock();
	float width = mGraphRect.width;
	float start = mGraphRect.left;
	std::string buffer = mSourceCode;
	int numPoint = mNumPoint2D;
	std::vector<Tweakable> tweakables = mTweakables;
	mMutex.unlock();

	Picoc pc;
	std::string errorBuffer;
	double x;
	PicocInitialise(&pc, &x, 1, buffer, tweakables, errorBuffer);

	if (errorBuffer.empty())
	{
		for (int i = 0; i < numPoint; i++)
		{
			x = (double)i / numPoint;
			mProgression = (float)x;
			if (coordinate == CARTESIAN)
			{
				x = x * width + start;
			}
			else
			{
				x *= 6.283185307179586;
			}

			float y = (float)PicocEvaluate(pc, 1, errorBuffer);
			if (!errorBuffer.empty())
			{
				break;
			}

			result.push_back(sf::Vector2f((float)x, y));
		}
	}
	PicocCleanup(&pc);
	mErrorMessage.setString(errorBuffer);

	return !errorBuffer.empty();
}

bool Application::evaluate3D(std::vector<sf::Vector3f>& result, int& curveWidth)
{
	mMutex.lock();
	float width = mGraphRect.width;
	float start = mGraphRect.left;
	std::string buffer = mSourceCode;
	curveWidth = mNumPoint3D;
	std::vector<Tweakable> tweakables = mTweakables;
	mMutex.unlock();
	
	Picoc pc;
	std::string errorBuffer;
	double point[2];
	PicocInitialise(&pc, point, 2, buffer, tweakables, errorBuffer);

	for (int i = 0; i < curveWidth; i++)
	{
		double posX = (double)i / curveWidth;
		mProgression = (float)posX;

		for (int j = 0; j < curveWidth; j++)
		{
			point[0] = posX * width + start;
			double posY = (double)j / curveWidth;
			point[1] = posY * width + start;

			float z = (float)PicocEvaluate(pc, 2, errorBuffer);
			if (!errorBuffer.empty())
			{
				break;
			}

			result.push_back(sf::Vector3f((float)(posX-0.5f), (float)(posY-0.5f), z));
		}

		if (!errorBuffer.empty())
		{
			break;
		}
	}
	PicocCleanup(&pc);
	mErrorMessage.setString(errorBuffer);
	
	return !errorBuffer.empty();
}

void Application::ApplyZoomOnGraph(float factor)
{
	sf::Vector2f center(mGraphRect.left + 0.5f * mGraphRect.width, mGraphRect.top + 0.5f * mGraphRect.height);
	sf::Lock lock(mMutex);
	mGraphRect.width *= factor;
	mGraphRect.height *= factor;
	mGraphRect.left = center.x - 0.5f * mGraphRect.width;
	mGraphRect.top = center.y - 0.5f * mGraphRect.height;
	mSourceDirty = true;
}

void Application::showGraph()
{
	std::vector<sf::Vertex> lines;
	mMutex.lock();
	for (const sf::Vector2f& p : mPoints2D)
	{
		if (mCoordinate == CARTESIAN)
		{
			lines.push_back(convertGraphCoordToScreen(p));
		}
		else // polar coordinate
		{
			sf::Vector2f p(p.y * cos(p.x), p.y * sin(p.x));
			lines.push_back(convertGraphCoordToScreen(p));
		}
	}
	mMutex.unlock();
	mGui.getWindow()->draw(lines.data(), lines.size(), sf::LinesStrip);
	lines.clear();

	// Axis
	std::vector<sf::Vertex> axis;
	// horizontal
	sf::Color axisColor = isMouseOverXAxis() ? sf::Color(150, 150, 150) : sf::Color::White;
	float middleY = 1.f + mGraphRect.top / mGraphRect.height;
	lines.push_back(sf::Vertex(sf::Vector2f(mGraphScreen.left, mGraphScreen.top + middleY*mGraphScreen.height), axisColor));
	lines.push_back(sf::Vertex(sf::Vector2f(mGraphScreen.left + mGraphScreen.width, mGraphScreen.top + middleY*mGraphScreen.height), axisColor));
	//vertical
	axisColor = isMouseOverYAxis() ? sf::Color(150, 150, 150) : sf::Color::White;
	float middleX = -mGraphRect.left / mGraphRect.width;
	lines.push_back(sf::Vertex(sf::Vector2f(mGraphScreen.left + middleX*mGraphScreen.width, mGraphScreen.top - 20.f), axisColor));
	lines.push_back(sf::Vertex(sf::Vector2f(mGraphScreen.left + middleX*mGraphScreen.width, mGraphScreen.top + mGraphScreen.height + 50.f), axisColor));

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

	for (const auto& it : mPoints)
	{
		sf::CircleShape circle(2.5f);
		circle.setFillColor(sf::Color::Blue);
		circle.setOutlineThickness(1.f);
		circle.setOutlineColor(sf::Color::Cyan);
		circle.setPosition(convertGraphCoordToScreen(it) - sf::Vector2f(2.5f, 2.5f));
		mWindow.draw(circle);
	}

	sf::Vector2f mouse = convertScreenCoordToGraph(sf::Vector2f((float)sf::Mouse::getPosition(mWindow).x, (float)sf::Mouse::getPosition(mWindow).y));
	
	if (mouse.x >= mGraphRect.left && mouse.x <= mGraphRect.left + mGraphRect.width)
	{
		if (mCoordinate == POLAR)
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
		if (mCoordinate == CARTESIAN)
		{
			textPos = convertGraphCoordToScreen(sf::Vector2f(0.f, y));
			textPos.x = (float)sf::Mouse::getPosition(mWindow).x;
		}
		else // polar coordinate
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

void Application::show3DGraph()
{
	mWindow.popGLStates();

	mMutex.lock();
	if (mPoints3D.empty())
	{
		mMutex.unlock();
		mWindow.pushGLStates();
		return;
	}

	glViewport((GLsizei)(mWindow.getSize().x * mDelimitatorRatio), 0, (GLsizei)(mWindow.getSize().x*(1.f - mDelimitatorRatio)), mWindow.getSize().y);

	// Clear the depth buffer
	glClear(GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.f, 0.f, -4.f);
	static sf::Clock clock;
	glRotatef(30.f, -1.f, 0.2f, 0.f);
	glRotatef(clock.getElapsedTime().asSeconds() * 30.f, 0.f, 0.f, 1.f);
	float scale = 3.f;
	glScalef(scale, scale, scale);

	std::vector<sf::Vector3f> positions;
	std::vector<sf::Color> colors;

	float minZ = 0, maxZ = 0;
	for (const sf::Vector3f& p : mPoints3D)
	{
		if (minZ > p.z)
			minZ = p.z;
		if (maxZ < p.z)
			maxZ = p.z;
	}
	float deltaZ = 0.f;
	if (maxZ - minZ > 1e-7f)
		deltaZ = 1.f / (maxZ - minZ);

	for (int x = 0; x < mCurveWidth-1; x++)
	{
		for (int y = 0; y < mCurveWidth-1; y++)
		{
			sf::Vector3f p0 = mPoints3D[x * mCurveWidth + y];
			sf::Vector3f p1 = mPoints3D[(x+1) * mCurveWidth + y];
			sf::Vector3f p2 = mPoints3D[(x+1) * mCurveWidth + y + 1];
			sf::Vector3f p3 = mPoints3D[x * mCurveWidth + y + 1];
			sf::Color c0 = rainbowColor(p0.z = (p0.z - minZ) * deltaZ);
			sf::Color c1 = rainbowColor(p1.z = (p1.z - minZ) * deltaZ);
			sf::Color c2 = rainbowColor(p2.z = (p2.z - minZ) * deltaZ);
			sf::Color c3 = rainbowColor(p3.z = (p3.z - minZ) * deltaZ);
			p0.z -=  0.5f; p1.z -= 0.5f; p2.z -= 0.5f; p3.z -= 0.5f;
			p0.z *= 0.5f; p1.z *= 0.5f; p2.z *= 0.5f; p3.z *= 0.5f;

			positions.push_back(p0);
			positions.push_back(p1);
			positions.push_back(p2);
			positions.push_back(p2);
			positions.push_back(p3);
			positions.push_back(p0);
			colors.push_back(c0);
			colors.push_back(c1);
			colors.push_back(c2);
			colors.push_back(c2);
			colors.push_back(c3);
			colors.push_back(c0);
		}
	}
	mMutex.unlock();

	glVertexPointer(3, GL_FLOAT, 3 * sizeof(float), positions.data());
	glColorPointer(4, GL_UNSIGNED_BYTE, 4 * sizeof(unsigned char), colors.data());
	glDrawArrays(GL_TRIANGLES, 0, positions.size());

	//Axis
	positions.clear();
	colors.clear();
	const float axisSize = 0.85f;
	positions.push_back(sf::Vector3f(-axisSize, 0.f, 0.f));
	positions.push_back(sf::Vector3f(axisSize, 0.f, 0.f));
	positions.push_back(sf::Vector3f(axisSize, 0.f, 0.f));
	positions.push_back(sf::Vector3f(axisSize-0.07f, 0.07f, 0.f));
	positions.push_back(sf::Vector3f(axisSize, 0.f, 0.f));
	positions.push_back(sf::Vector3f(axisSize-0.07f, -0.07f, 0.f));
	for (int i = 0; i < 6; i++)
		colors.push_back(sf::Color::Red);

	positions.push_back(sf::Vector3f(0.f, -axisSize, 0.f));
	positions.push_back(sf::Vector3f(0.f, axisSize, 0.f));
	positions.push_back(sf::Vector3f(0.f, axisSize, 0.f));
	positions.push_back(sf::Vector3f(0.07f, axisSize - 0.07f, 0.f));
	positions.push_back(sf::Vector3f(0.f, axisSize, 0.f));
	positions.push_back(sf::Vector3f(-0.07f, axisSize - 0.07f, 0.f));
	for (int i = 0; i < 6; i++)
		colors.push_back(sf::Color::Green);

	positions.push_back(sf::Vector3f(0.f, 0.f, -axisSize));
	positions.push_back(sf::Vector3f(0.f, 0.f, axisSize));
	positions.push_back(sf::Vector3f(0.f, 0.f, axisSize));
	positions.push_back(sf::Vector3f(0.07f, 0.f, axisSize - 0.07f));
	positions.push_back(sf::Vector3f(0.f, 0.f, axisSize));
	positions.push_back(sf::Vector3f(-0.07f, 0.f, axisSize - 0.07f));
	for (int i = 0; i < 6; i++)
		colors.push_back(sf::Color::Blue);

	glVertexPointer(3, GL_FLOAT, 3 * sizeof(float), positions.data());
	glColorPointer(4, GL_UNSIGNED_BYTE, 4 * sizeof(unsigned char), colors.data());
	glDrawArrays(GL_LINES, 0, positions.size());

	mWindow.pushGLStates();

	// Axis description
	char buffer[256];
	sprintf_s<256>(buffer, "Axis Z: %g to %g", minZ, maxZ);
	sf::Text text(buffer, *mGui.getFont(), 14);
	text.setPosition(mWindow.getSize().x - 230.f, mWindow.getSize().y - 60.f);
	text.setColor(sf::Color::Blue);
	mWindow.draw(text);

	sprintf_s<256>(buffer, "Axis Y: %g to %g", mGraphRect.top, mGraphRect.top + mGraphRect.height);
	text.setString(buffer);
	text.setPosition(text.getPosition().x, text.getPosition().y - 30);
	text.setColor(sf::Color::Green);
	mWindow.draw(text);

	sprintf_s<256>(buffer, "Axis X: %g to %g", mGraphRect.left, mGraphRect.left + mGraphRect.width);
	text.setString(buffer);
	text.setPosition(text.getPosition().x, text.getPosition().y - 30);
	text.setColor(sf::Color::Red);
	mWindow.draw(text);
}

void Application::callbackTextEdit()
{
	mMutex.lock();
	mSourceCodeHistory.push_back(mSourceCode);
	if (mSourceCodeHistory.size() > 50)//limit the size of the history
		mSourceCodeHistory.pop_front();
	mSourceCodeRedo.clear();
	mSourceCode = mSourceCodeEditBox->getText().toAnsiString();
	mSourceDirty = true;
	mMutex.unlock();
}

void Application::fillDefaultSourceCode()
{
	if (mCoordinate != THREE_D)
	{
		mSourceCodeEditBox->setText("double main(double x){\n\nreturn x;\n}");
	}
	else
	{
		mSourceCodeEditBox->setText("double main(double x, double y){\n\nreturn sin(x*0.5)*sin(y*0.5);\n}");
	}
	callbackTextEdit();
}

void Application::showBuiltInFunctions()
{
	std::string list;
	GetBuiltInFunctionConstants(list);
	const char* str = list.c_str();

	sf::Text text("", *mGui.getFont(), 12);
	text.setPosition(mWindow.getSize().x * mDelimitatorRatio + 30.f, 30.f);
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
			if (pos.y > mWindow.getSize().y - 50)
			{
				pos = sf::Vector2f(pos.x + 250.f, 30.f);
			}
			text.setPosition(pos);
			mGui.getWindow()->draw(text);
		}
	}
}

void Application::callbackAddTweakable(tgui::EditBox::Ptr editBox)
{
	std::string text = editBox->getText();
	if (!text.empty() && isalpha(text[0]))
	{
		// to upper case
		std::transform(text.begin(), text.end(), text.begin(), ::toupper);
		// prevent duplicate
		for (const Tweakable& it : mTweakables)
		{
			if (it.name == text)
			{
				editBox->getRenderer()->setBorderColor(sf::Color::Red);
				return;
			}
		}

		mMutex.lock();
		mTweakables.push_back(Tweakable(text));
		mSourceCodeEditBox->setText(mSourceCode);
		mMutex.unlock();
		
		callbackTextEdit();
		updateTweakable(true);
		editBox->setText("");
		editBox->getRenderer()->setBorderColor(sf::Color::Black);
		mAddTweakableBox->hide();
	}
	else
	{
		editBox->getRenderer()->setBorderColor(sf::Color::Red);
	}
}

void Application::addTweakableButtons(size_t index, const std::string& tweakableName)
{
	tgui::Panel::Ptr tweakableContainer = std::dynamic_pointer_cast<tgui::Panel>(mGui.get("TweakableContainer"));
	tgui::Button::Ptr button = tgui::Button::create();
	button->setSize(tgui::bindWidth(tweakableContainer) - 50, 20);
	button->setPosition(5, 25.f * index + 5.f);
	button->setText(tweakableName);
	tweakableContainer->add(button, tweakableName);
	button->connect("pressed", [this, tweakableName] {
		showTweakableSettings(tweakableName);
	});

	tgui::Button::Ptr deleteButton = tgui::Button::create();
	deleteButton->setSize(30, 20);
	deleteButton->setPosition(tgui::bindRight(button) + 10, tgui::bindTop(button));
	deleteButton->setText("Del");
	tweakableContainer->add(deleteButton);
	deleteButton->connect("pressed", [this, button, deleteButton, tweakableName] {
		for (auto it = mTweakables.begin(); it != mTweakables.end(); ++it)
		{
			if (it->name == tweakableName)
			{
				mTweakables.erase(it);
				break;
			}
		}
		updateTweakable(true);
	});
}

void Application::updateTweakable(bool rebuildUI)
{
	tgui::Panel::Ptr tweakableContainer = std::dynamic_pointer_cast<tgui::Panel>(mGui.get("TweakableContainer"));

	if (mTweakables.empty())
	{
		if (rebuildUI)
		{
			mTweakableEditBox->setSize(mTweakableEditBox->getSizeLayout().x, 0.f);
			mTweakableEditBox->hide();
			tweakableContainer->hide();
		}
	}
	else
	{
		if (rebuildUI)
		{
			tweakableContainer->setSize(0.1f * tgui::bindWidth(mGui), 25.f * mTweakables.size() + 15);
			tweakableContainer->removeAllWidgets();
			for (size_t i = 0; i < mTweakables.size(); i++)
			{
				addTweakableButtons(i, mTweakables[i].name);
			}
			tweakableContainer->show();
		}

		std::string text;
		for (const Tweakable& it : mTweakables)
		{
			text.append("double ");
			text.append(it.name);
			text.append(" = ");
			text.append(std::to_string(it.value));
			text.append(";\n");
		}
		text.resize(std::max((int)text.size() - 1, 0));// remove last \n
		mTweakableEditBox->show();
		mTweakableEditBox->setSize(mTweakableEditBox->getSizeLayout().x, 20.f * std::min<int>(mTweakables.size(), 3) + 7.f);
		mTweakableEditBox->setText(text);
	}
}

void Application::showTweakableSettings(const std::string& currentTweakable)
{
	mCurrentTweakable = currentTweakable;

	for (const Tweakable& it : mTweakables)
	{
		if (it.name == mCurrentTweakable)
		{
			auto tweakableSettings = std::dynamic_pointer_cast<tgui::Panel>(mGui.get("TweakableSettings"));
			tweakableSettings->show();

			//slider
			auto slider = std::dynamic_pointer_cast<tgui::Slider>(tweakableSettings->get("Slider"));		
			double i = (it.value - it.min) / (it.max - it.min);
			slider->setValue((int)(i * slider->getMaximum()));

			// min / max
			auto minBox = std::dynamic_pointer_cast<tgui::EditBox>(tweakableSettings->get("Min"));
			minBox->setText(std::to_string(it.min));
			auto maxBox = std::dynamic_pointer_cast<tgui::EditBox>(tweakableSettings->get("Max"));
			maxBox->setText(std::to_string(it.max));
			return;
		}
	}
}

void Application::loadWidgets()
{
	tgui::disableTabKeyUsage();
	auto windowHeight = tgui::bindHeight(mGui);

	mMainContainer = std::make_shared<tgui::VerticalLayout>();
	mMainContainer->setSize(mDelimitatorRatio * tgui::bindWidth(mGui) - 20, windowHeight);
	mGui.add(mMainContainer, "Main");

	mTweakableEditBox = tgui::TextBox::create();
	mTweakableEditBox->setSize(tgui::bindWidth(mMainContainer), 0);
	mTweakableEditBox->setPosition(10, 30);
	mTweakableEditBox->setReadOnly(true);
	mTweakableEditBox->hide();
	mGui.add(mTweakableEditBox);

	// Create the source code edit box
	mSourceCodeEditBox = tgui::SourceTextBox::create();
	mSourceCodeEditBox->setSize(tgui::bindWidth(mMainContainer), windowHeight - 200 - tgui::bindHeight(mTweakableEditBox));
	mSourceCodeEditBox->setPosition(10, tgui::bindBottom(mTweakableEditBox) + 10);
	mGui.add(mSourceCodeEditBox, "Code");
	mSourceCodeEditBox->connect("TextChanged", &Application::callbackTextEdit, this);
	
	// Apply default source code
	fillDefaultSourceCode();

	// Create buttons
	tgui::Button::Ptr functionButton = tgui::Button::create();
	functionButton->setSize(tgui::bindWidth(mMainContainer), 25);
	functionButton->setPosition(10, tgui::bindBottom(mSourceCodeEditBox) + 10);
	functionButton->setText("Show built-in functions");
	mGui.add(functionButton);
	functionButton->connect("pressed", [this] {
		mShowFunctionList = !mShowFunctionList;
	});

	tgui::Button::Ptr resetButton = tgui::Button::create();
	resetButton->setSize(tgui::bindWidth(mMainContainer), 25);
	resetButton->setPosition(10, tgui::bindBottom(functionButton) + 10);
	resetButton->setText("Reset interpreter");
	mGui.add(resetButton);
	resetButton->connect("pressed", [this] {
		extern bool gResetParser;
		gResetParser = true;
	});

	tgui::ComboBox::Ptr coordinateBox = tgui::ComboBox::create();
	coordinateBox->setSize(180, 25);
	coordinateBox->setPosition(tgui::bindWidth(mMainContainer) + 60.f, 10.f);
	coordinateBox->addItem("Cartesian coordinates");
	coordinateBox->addItem("Polar coordinates");
	coordinateBox->addItem("3D curve");
	coordinateBox->setSelectedItemByIndex(mCoordinate);
	mGui.add(coordinateBox);
	coordinateBox->connect("ItemSelected", [this](tgui::ComboBox::Ptr box) {
		mPoints2D.clear();
		mPoints3D.clear();
		if (mCoordinate != (enumCoordinate)box->getSelectedItemIndex())
		{
			mCoordinate = (enumCoordinate)box->getSelectedItemIndex();
			fillDefaultSourceCode();
		}
		mCoordinate = (enumCoordinate)box->getSelectedItemIndex();
	}, coordinateBox);

	tgui::ComboBox::Ptr highDefBox = tgui::ComboBox::create();
	highDefBox->setSize(120, 25);
	highDefBox->setPosition(tgui::bindRight(coordinateBox) + 20.f, tgui::bindTop(coordinateBox));
	highDefBox->addItem("Low Def");
	highDefBox->addItem("Medium Def");
	highDefBox->addItem("High Def");
	highDefBox->setSelectedItemByIndex(1);
	mGui.add(highDefBox);
	highDefBox->connect("ItemSelected", [this](tgui::ComboBox::Ptr box) {
		switch (box->getSelectedItemIndex())
		{
		case 0:
			mNumPoint2D = 128;
			mNumPoint3D = 16;
			break;
		case 1:
			mNumPoint2D = 1024;
			mNumPoint3D = 32;
			break;
		case 2:
			mNumPoint2D = 1500;
			mNumPoint3D = 64;
			break;
		}
	}, highDefBox);

	mErrorMessage.setFont(*mGui.getFont());
	mErrorMessage.setCharacterSize(14);
	mErrorMessage.setColor(sf::Color::Red);

	// Tweakables
	{
		tgui::Panel::Ptr tweakableContainer = std::make_shared<tgui::Panel>();
		tweakableContainer->setSize(0.1f * tgui::bindWidth(mGui), 10.f);
		tweakableContainer->setPosition(tgui::bindWidth(mGui) - tgui::bindWidth(tweakableContainer) - 10.f, windowHeight - tgui::bindHeight(tweakableContainer) - 10.f);
		tweakableContainer->getRenderer()->setBackgroundColor(sf::Color(180,180,180,180));
		tweakableContainer->hide();
		mGui.add(tweakableContainer, "TweakableContainer");

		tgui::Button::Ptr addTweakable = tgui::Button::create();
		addTweakable->setSize(tgui::bindWidth(tweakableContainer) - 14, 20);
		addTweakable->setPosition(tgui::bindLeft(tweakableContainer) + 7, tgui::bindBottom(tweakableContainer) - 13);
		addTweakable->setText("Add tweakable");
		addTweakable->setTextSize(10);
		mGui.add(addTweakable);
		addTweakable->connect("pressed", [this]() {
			mAddTweakableBox->show();
			mAddTweakableBox->get("EditBox")->focus();
		});

		tgui::Panel::Ptr tweakableSettings = std::make_shared<tgui::Panel>();
		tweakableSettings->setSize(0.25f * tgui::bindWidth(mGui), 70.f);
		tweakableSettings->setPosition(0.4f * tgui::bindWidth(mGui), windowHeight - 70.f);
		tweakableSettings->hide();
		tweakableSettings->getRenderer()->setBackgroundColor(sf::Color(180, 180, 180, 180));
		mGui.add(tweakableSettings, "TweakableSettings");

		tgui::Slider::Ptr tweakableSlider = tgui::Slider::create();
		tweakableSlider->setPosition(10, 10);
		tweakableSlider->setSize(tgui::bindWidth(tweakableSettings) - 20, 10);
		tweakableSlider->setMaximum(1000);
		tweakableSettings->add(tweakableSlider, "Slider");
		tweakableSlider->connect("ValueChanged", [this, tweakableSlider]() {
			for (Tweakable& it : mTweakables)
			{
				if (it.name == mCurrentTweakable)
				{
					sf::Lock lock(mMutex);
					it.value = (double)tweakableSlider->getValue() / tweakableSlider->getMaximum();
					it.value = it.value * (it.max - it.min) + it.min;
					updateTweakable(false);
					mSourceDirty = true;
					return;
				}
			}
		});

		tgui::Label::Ptr minText = tgui::Label::create();
		minText->setSize(50, 20);
		minText->setPosition(10, 35);
		minText->setText("Min:");
		tweakableSettings->add(minText);

		tgui::EditBox::Ptr minEditBox = tgui::EditBox::create();
		minEditBox->setSize(50, 20);
		minEditBox->setPosition(tgui::bindPosition(minText) + sf::Vector2f(40.f, 0.f));
		tweakableSettings->add(minEditBox, "Min");
		minEditBox->connect("TextChanged", [this, minEditBox]() {
			for (Tweakable& it : mTweakables)
			{
				if (it.name == mCurrentTweakable)
				{
					it.min = atof(minEditBox->getText().toAnsiString().c_str());
					return;
				}
			}
		});

		tgui::Label::Ptr maxText = tgui::Label::create();
		maxText->setSize(50, 20);
		maxText->setPosition(tgui::bindPosition(minEditBox) + sf::Vector2f(70.f, 0.f));
		maxText->setText("Max:");
		tweakableSettings->add(maxText);

		tgui::EditBox::Ptr maxEditBox = tgui::EditBox::create();
		maxEditBox->setSize(50, 20);
		maxEditBox->setPosition(tgui::bindPosition(maxText) + sf::Vector2f(40.f, 0.f));
		tweakableSettings->add(maxEditBox, "Max");
		maxEditBox->connect("TextChanged", [this, maxEditBox]() {
			for (Tweakable& it : mTweakables)
			{
				if (it.name == mCurrentTweakable)
				{
					it.max = atof(maxEditBox->getText().toAnsiString().c_str());
					return;
				}
			}
		});
	}

	// Points
	{
		tgui::Button::Ptr createPointButton = tgui::Button::create();
		createPointButton->setSize(140, 20);
		createPointButton->setPosition(tgui::bindLeft(mGui.get("TweakableContainer")) - 180, windowHeight - 25);
		createPointButton->setText("Add point");
		createPointButton->setTextSize(10);
		mGui.add(createPointButton);
		createPointButton->connect("pressed", [this]() {
			if (mCoordinate == CARTESIAN)
			{
				mPoints.push_back(sf::Vector2f(mGraphRect.left + 0.5f * mGraphRect.width, mGraphRect.top + 0.5f * mGraphRect.height));
			}
		});
	}

	//Confirmation window
	{
		mAddTweakableBox = tgui::Panel::create();
		mAddTweakableBox->setSize(400, 200);
		mAddTweakableBox->setPosition(0.5f * tgui::bindSize(mGui) - sf::Vector2f(200.f, 100.f));
		mAddTweakableBox->getRenderer()->setBackgroundColor(sf::Color(180,180,180,180));
		mAddTweakableBox->hide();
		mGui.add(mAddTweakableBox);

		tgui::Label::Ptr text = tgui::Label::create();
		text->setPosition(100, 40);
		text->setText("Tweakable name:");
		mAddTweakableBox->add(text);

		tgui::EditBox::Ptr editBox = tgui::EditBox::create();
		editBox->setSize(200, 25);
		editBox->setPosition(100, 80);
		mAddTweakableBox->add(editBox, "EditBox");
		editBox->connect("ReturnKeyPressed", &Application::callbackAddTweakable, this, editBox);

		tgui::Button::Ptr OKButton = tgui::Button::create();
		OKButton->setSize(50, 25);
		OKButton->setPosition(50, 150);
		OKButton->setText("OK");
		mAddTweakableBox->add(OKButton);
		OKButton->connect("pressed", &Application::callbackAddTweakable, this, editBox);

		tgui::Button::Ptr CancelButton = tgui::Button::create();
		CancelButton->setSize(50, 25);
		CancelButton->setPosition(300, 150);
		CancelButton->setText("Cancel");
		mAddTweakableBox->add(CancelButton);
		CancelButton->connect("pressed", [this, editBox]{
			editBox->setText("");
			mAddTweakableBox->hide();
		});
	}
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

bool Application::isMouseOverXAxis()
{
	float middleY = 1.f + mGraphRect.top / mGraphRect.height;
	float axis = mGraphScreen.top + middleY*mGraphScreen.height;
	return fabs(axis - sf::Mouse::getPosition(mWindow).y) < 10.f;
}

bool Application::isMouseOverYAxis()
{
	float middleX = -mGraphRect.left / mGraphRect.width;
	float axis = mGraphScreen.left + middleX*mGraphScreen.width;
	return fabs(axis - sf::Mouse::getPosition(mWindow).x) < 10.f;
}

bool Application::isMouseOverDelimitator()
{
	return fabs(mWindow.getSize().x * mDelimitatorRatio - sf::Mouse::getPosition(mWindow).x) < 5.f;
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
	if (mPoints2D.size() < 2)
		return 0.f;

	sf::Lock lock(mMutex);

	sf::Vector2f p0 = mPoints2D[0];
	sf::Vector2f p1 = mPoints2D[1];
	for (unsigned i = 1; i < mPoints2D.size(); i++)
	{
		if (mPoints2D[i].x > x)
		{
			p0 = mPoints2D[i-1];
			p1 = mPoints2D[i];
			break;
		}
	}

	float a = (x - p0.x) / (p1.x - p0.x);
	return a * (p1.y - p0.y) + p0.y;
}

//i entre 0 et 1
sf::Color Application::rainbowColor(float i)
{
	i *= 0.833333f;

	if (i < 0.16666667f)
	{
		return sf::Color(255, 0, (sf::Uint8)(i * 6.f*255.f));
	}
	else if (i < 0.33333333f)
	{
		return sf::Color((sf::Uint8)(255.f - (i - 0.16666667f) * 6.f * 255.f), 0, 255);
	}
	else if (i < 0.5f)
	{
		return sf::Color(0, (sf::Uint8)((i - 0.3333333f) * 6.f * 255.f), 255);
	}
	else if (i < 0.66666667f)
	{
		return sf::Color(0, 255, (sf::Uint8)(255.f - (i - 0.5f) * 6.f * 255.f));
	}
	else if (i < 0.8333333f)
	{
		return sf::Color((sf::Uint8)((i - 0.6666667f) * 6.f * 255.f), 255, 0);
	}
	else
	{
		return sf::Color(255, (sf::Uint8)(255.f - (i - 0.8333333f) * 6.f * 255.f), 0);
	}
}