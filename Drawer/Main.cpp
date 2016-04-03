#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <string>
#include <thread>
#include <chrono>

extern "C" {
	#include "picoc.h"
}
#define NUM_POINTS 1024

sf::Mutex mutex;
std::string sourceCode;
std::vector<sf::Vector2f> points;
sf::FloatRect graphRect(-10.f, -10.f, 20.f, 20.f);
sf::Text errorMessage;

void execute()
{
	std::vector<sf::Vector2f> result;
	char errorBuffer[1024];
	while (1)
	{
		result.clear();

		mutex.lock();
		float width = graphRect.width;
		float start = graphRect.left;
		std::string buffer = sourceCode;
		mutex.unlock();
		int isCrash = 0;

		for (int i = 0; i < NUM_POINTS; i++)
		{
			double x = (double)i / NUM_POINTS;
			x = x * width + start;

			float y = (float)parse(buffer.c_str(), x, &isCrash, errorBuffer);
			if (isCrash)
			{
				break;
			}
			if (i % 30 == 0)
				std::cout << "x";

			result.push_back(sf::Vector2f((float)x, y));
		}
		
		std::cout << std::endl;

		mutex.lock();
		points = result;
		if (isCrash)
			errorMessage.setString(errorBuffer);
		else
			errorMessage.setString(sf::String());
		mutex.unlock();
		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void callbackTextEdit(tgui::TextBox::Ptr source)
{
	mutex.lock();
	sourceCode = source->getText().toAnsiString();
	mutex.unlock();
}

void loadWidgets(tgui::Gui& gui)
{
	// Load the black theme
	auto theme = std::make_shared<tgui::Theme>();// "TGUI/widgets/Black.txt");

	// Get a bound version of the window size
	// Passing this to setPosition or setSize will make the widget automatically update when the view of the gui changes
	auto windowWidth = tgui::bindWidth(gui);
	auto windowHeight = tgui::bindHeight(gui);


	// Create the username edit box
	tgui::TextBox::Ptr editBox = theme->load("TextBox");
	editBox->setSize(windowWidth * 0.25f, windowHeight - 200);
	editBox->setPosition(10, 30);
	editBox->setText("#include \"stdio.h\"\n#include \"math.h\"\ndouble main(double x){\n\nreturn x;\n}");
	gui.add(editBox, "Code");

	editBox->connect("TextChanged", callbackTextEdit, editBox);
	callbackTextEdit(editBox);

	//// Create the login button
	//tgui::Button::Ptr button = theme->load("Button");
	//button->setSize(windowWidth * 0.25f, 50);
	//button->setPosition(10, windowHeight -150);
	//button->setText("Launch");
	//gui.add(button);

	//// Call the login function when the button is pressed
	//button->connect("pressed", launch, editBox);
	errorMessage.setFont(*gui.getFont());
	errorMessage.setCharacterSize(13);
	errorMessage.setColor(sf::Color::Red);
}

std::vector<float> computeAxisGraduation(float min, float max)
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
	for (; i < max+0.1*step; i += step)
	{
		if (abs(i) > 1e-9)
		{
			axis.push_back((float)i);
		}
	}

	return axis;
}

int main()
{
	// Create the window
	sf::RenderWindow window(sf::VideoMode(1000, 700), "TGUI window");
	window.setFramerateLimit(60);
	tgui::Gui gui(window);

	try
	{
		// Load the widgets
		loadWidgets(gui);
	}
	catch (const tgui::Exception& e)
	{
		std::cerr << "Failed to load TGUI widgets: " << e.what() << std::endl;
		system("pause");
		return 1;
	}

	std::thread thread(execute);

	// Main loop
	sf::Clock timer;
	bool drag = false;
	sf::Vector2f dragPosition;
	sf::Vector2i dragMousePosition;
	while (window.isOpen())
	{
		//***************************************************
		// Events and inputs
		//***************************************************
		sf::Event event;
		while (window.pollEvent(event))
		{
			// When the window is closed, the application ends
			if (event.type == sf::Event::Closed)
				window.close();

			// When the window is resized, the view is changed
			else if (event.type == sf::Event::Resized)
			{
				window.setView(sf::View(sf::FloatRect(0, 0, (float)event.size.width, (float)event.size.height)));
				gui.setView(window.getView());
			}

			// Pass the event to all the widgets
			gui.handleEvent(event);
		}

		// Zoom in
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::PageUp) && timer.getElapsedTime().asMilliseconds() > 10)
		{
			timer.restart();
			sf::Vector2f center(graphRect.left + 0.5f * graphRect.width, graphRect.top + 0.5f * graphRect.height);
			sf::Lock lock(mutex);
			graphRect.width *= 1.02f;
			graphRect.height *= 1.02f;
			graphRect.left = center.x - 0.5f * graphRect.width;
			graphRect.top = center.y - 0.5f * graphRect.height;
		}
		// Zoom out
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::PageDown) && timer.getElapsedTime().asMilliseconds() > 10)
		{
			timer.restart();
			sf::Vector2f center(graphRect.left + 0.5f * graphRect.width, graphRect.top + 0.5f * graphRect.height);
			sf::Lock lock(mutex);
			graphRect.width *= 0.98f;
			graphRect.height *= 0.98f;
			graphRect.left = center.x - 0.5f * graphRect.width;
			graphRect.top = center.y - 0.5f * graphRect.height;
		}
		// mouse
		if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
		{
			if (drag)
			{
				sf::Vector2i delta = sf::Mouse::getPosition() - dragMousePosition;
				const float sensibility = 0.001f;
				graphRect.left = dragPosition.x - delta.x * sensibility * graphRect.width;
				graphRect.top = dragPosition.y + delta.y * sensibility * graphRect.height;
			}
			else
			{
				drag = true;
				dragPosition = sf::Vector2f(graphRect.left, graphRect.top);
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
		window.clear();

		// Draw all created widgets
		gui.draw();

		// Curve
		sf::FloatRect graphScreen(gui.getSize().x * 0.25f + 30.f, 100.f, gui.getSize().x * 0.65f, gui.getSize().y - 200.f);

		std::vector<sf::Vertex> lines;
		mutex.lock();
		for (sf::Vector2f p : points)
		{
			p.x = (p.x - graphRect.left) / graphRect.width;
			p.y = (p.y - graphRect.top) / graphRect.height;
			lines.push_back(sf::Vector2f(graphScreen.left + p.x * graphScreen.width, graphScreen.top + (1.f-p.y) * graphScreen.height));
		}
		mutex.unlock();
		window.draw(lines.data(), lines.size(), sf::LinesStrip);

		// Axis
		std::vector<sf::Vertex> axis;
		// horizontal
		float middleY = 1.f + graphRect.top / graphRect.height;
		lines.push_back(sf::Vector2f(graphScreen.left, graphScreen.top + middleY*graphScreen.height));
		lines.push_back(sf::Vector2f(graphScreen.left+graphScreen.width, graphScreen.top + middleY*graphScreen.height));
		//vertical
		float middleX = -graphRect.left / graphRect.width;
		lines.push_back(sf::Vector2f(graphScreen.left + middleX*graphScreen.width, graphScreen.top));
		lines.push_back(sf::Vector2f(graphScreen.left + middleX*graphScreen.width, graphScreen.top+graphScreen.height));

		std::vector<float> graduation = computeAxisGraduation(graphRect.left, graphRect.left + graphRect.width);
		const float graduationSize = 2.f;
		for (float x : graduation)
		{
			char str[32];
			sprintf_s<32>(str, "%g", x);
			sf::Text text(str, *gui.getFont(), 12);
			x = (x - graphRect.left) / graphRect.width;
			text.setPosition(graphScreen.left + x * graphScreen.width, graphScreen.top + middleY*graphScreen.height - graduationSize);
			window.draw(text);

			lines.push_back(sf::Vector2f(graphScreen.left + x * graphScreen.width, graphScreen.top + middleY*graphScreen.height + graduationSize));
			lines.push_back(sf::Vector2f(graphScreen.left + x * graphScreen.width, graphScreen.top + middleY*graphScreen.height - graduationSize));
		}

		graduation = computeAxisGraduation(graphRect.top, graphRect.top + graphRect.height);
		for (float y : graduation)
		{
			char str[32];
			sprintf_s<32>(str, "%g", y);
			sf::Text text(str, *gui.getFont(), 12);
			y = (y - graphRect.top) / graphRect.height;
			text.setPosition(graphScreen.left + middleX*graphScreen.width + graduationSize + 1.f, graphScreen.top + (1.f - y) * graphScreen.height - 5.f);
			window.draw(text);

			lines.push_back(sf::Vector2f(graphScreen.left + middleX*graphScreen.width + graduationSize, graphScreen.top + (1.f - y) * graphScreen.height));
			lines.push_back(sf::Vector2f(graphScreen.left + middleX*graphScreen.width - graduationSize, graphScreen.top + (1.f - y) * graphScreen.height));
		}
		window.draw(lines.data(), lines.size(), sf::Lines);

		// Display messages
		mutex.lock();
		errorMessage.setPosition(30, gui.getSize().y - 100);
		window.draw(errorMessage);
		mutex.unlock();

		window.display();
	}

	thread.detach();
	return EXIT_SUCCESS;
}
