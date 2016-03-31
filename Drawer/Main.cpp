#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>
#include <string>

extern "C" {
	#include "picoc.h"
}
#define NUM_POINTS 1024

std::vector<float> points;
sf::FloatRect graphRect(-10.f, -10.f, 20.f, 20.f);

void launch(tgui::TextBox::Ptr username)
{
	points.clear();

	//std::cout << "Username: " << username->getText().toAnsiString() << std::endl;
	std::string str = username->getText().toAnsiString();
	char* buffer = new char[str.size()+1];
	memcpy(buffer, str.data(), str.size()+1);
	sqrt(1);
	for (int i = 0; i < NUM_POINTS; i++)
	{
		double x = (double)i / NUM_POINTS;
		x = x * graphRect.width + graphRect.left;

		float y = (float)parse(buffer, x);
		if (i % 30 == 0)
			std::cout << "x";

		y = (y - graphRect.top) / graphRect.height;
		points.push_back(y);
	}
	delete[] buffer;
	std::cout << "OK" << std::endl;
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
	tgui::TextBox::Ptr editBoxUsername = theme->load("TextBox");
	editBoxUsername->setSize(windowWidth * 0.25f, windowHeight - 200);
	editBoxUsername->setPosition(10, 30);
	editBoxUsername->setText("#include \"stdio.h\"\n#include \"math.h\"\ndouble main(double x){\n\nreturn x;\n}");
	gui.add(editBoxUsername, "Code");


	// Create the login button
	tgui::Button::Ptr button = theme->load("Button");
	button->setSize(windowWidth * 0.25f, 50);
	button->setPosition(10, windowHeight -150);
	button->setText("Launch");
	gui.add(button);

	// Call the login function when the button is pressed
	button->connect("pressed", launch, editBoxUsername);
}

std::vector<float> computeAxisGraduation(float min, float max)
{
	float delta = max - min;
	const static double mul[] = { 1.0, 2.0, 5.0 };
	std::vector<float> axis;

	bool ok = false;
	float step = FLT_MAX;
	for (int e = -7; e < 9; e++)
	{
		double a = pow(10.0, e);

		for (int i = 0; i < 3; i++)
		{
			double b = a * mul[i];

			if (delta / b <= 10)
			{
				step = (float)b;
				ok = true;
				break;
			}
		}
		if (ok)
		{
			break;
		}
	}

	float i = floor(min / step) * step;
	for (; i < max+0.1f*step; i += step)
	{
		if (abs(i) > 1e-9)
		{
			axis.push_back(i);
		}
	}

	return axis;
}

int main()
{
	// Create the window
	sf::RenderWindow window(sf::VideoMode(1000, 700), "TGUI window");
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

	// Main loop
	while (window.isOpen())
	{
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

		window.clear();

		// Draw all created widgets
		gui.draw();

		// Curve
		sf::FloatRect graphScreen(gui.getSize().x * 0.25f + 30.f, 100.f, gui.getSize().x * 0.65f, gui.getSize().y - 200.f);

		std::vector<sf::Vertex> lines;
		int i = 0;
		for (float y : points)
		{
			float x = (float)i / NUM_POINTS;
			lines.push_back(sf::Vector2f(graphScreen.left + x * graphScreen.width, graphScreen.top + (1.f-y) * graphScreen.height));
			i++;
		}
		window.draw(lines.data(), lines.size(), sf::LinesStrip);

		// Axis
		std::vector<sf::Vertex> axis;
		// horizontal
		lines.push_back(sf::Vector2f(graphScreen.left, graphScreen.top + 0.5f*graphScreen.height));
		lines.push_back(sf::Vector2f(graphScreen.left+graphScreen.width, graphScreen.top + 0.5f*graphScreen.height));
		//vertical
		lines.push_back(sf::Vector2f(graphScreen.left + 0.5f*graphScreen.width, graphScreen.top));
		lines.push_back(sf::Vector2f(graphScreen.left + 0.5f*graphScreen.width, graphScreen.top+graphScreen.height));

		std::vector<float> graduation = computeAxisGraduation(graphRect.left, graphRect.left + graphRect.width);
		const float graduationSize = 2.f;
		for (float x : graduation)
		{
			char str[32];
			sprintf_s<32>(str, "%g", x);
			sf::Text text(str, *gui.getFont(), 12);
			x = (x - graphRect.left) / graphRect.width;
			text.setPosition(graphScreen.left + x * graphScreen.width, graphScreen.top + 0.5f*graphScreen.height - graduationSize);
			window.draw(text);

			lines.push_back(sf::Vector2f(graphScreen.left + x * graphScreen.width, graphScreen.top + 0.5f*graphScreen.height + graduationSize));
			lines.push_back(sf::Vector2f(graphScreen.left + x * graphScreen.width, graphScreen.top + 0.5f*graphScreen.height - graduationSize));
		}

		graduation = computeAxisGraduation(graphRect.top, graphRect.top + graphRect.height);
		for (float y : graduation)
		{
			char str[32];
			sprintf_s<32>(str, "%g", y);
			sf::Text text(str, *gui.getFont(), 12);
			y = (y - graphRect.top) / graphRect.height;
			text.setPosition(graphScreen.left + 0.5f*graphScreen.width + graduationSize + 1.f, graphScreen.top + (1.f - y) * graphScreen.height - 5.f);
			window.draw(text);

			lines.push_back(sf::Vector2f(graphScreen.left + 0.5f*graphScreen.width + graduationSize, graphScreen.top + (1.f - y) * graphScreen.height));
			lines.push_back(sf::Vector2f(graphScreen.left + 0.5f*graphScreen.width - graduationSize, graphScreen.top + (1.f - y) * graphScreen.height));		
		}
		window.draw(lines.data(), lines.size(), sf::Lines);

		window.display();
	}

	return EXIT_SUCCESS;
}
