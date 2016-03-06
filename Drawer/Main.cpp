#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <TGUI/TGUI.hpp>

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


	// Create the code edit box
	tgui::TextBox::Ptr editBox = theme->load("TextBox");
	editBox->setSize(windowWidth * 0.25, windowHeight - 200);
	editBox->setPosition(10, 30);
	editBox->setText("#include \"stdio.h\"\n#include \"math.h\"\ndouble main(double x){\n\nreturn x;\n}");
	gui.add(editBox, "Code");


	// Create the launch button
	tgui::Button::Ptr button = theme->load("Button");
	button->setSize(windowWidth * 0.25, 50);
	button->setPosition(10, windowHeight -150);
	button->setText("Launch");
	gui.add(button);

	// Call the login function when the button is pressed
	button->connect("pressed", launch, editBox);
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
		sf::FloatRect graphScreen(gui.getSize().x * 0.25f, 100.f, gui.getSize().x * 0.65f, gui.getSize().y - 200.f);

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
		lines.clear();
		// horizontal
		lines.push_back(sf::Vector2f(graphScreen.left, graphScreen.top + 0.5f*graphScreen.height));
		lines.push_back(sf::Vector2f(graphScreen.left+graphScreen.width, graphScreen.top + 0.5f*graphScreen.height));
		//vertical
		lines.push_back(sf::Vector2f(graphScreen.left + 0.5f*graphScreen.width, graphScreen.top));
		lines.push_back(sf::Vector2f(graphScreen.left + 0.5f*graphScreen.width, graphScreen.top+graphScreen.height));
		// axis color
		for (auto& l : lines)
		{
			l.color = sf::Color(128, 128, 128);
		}
		window.draw(lines.data(), lines.size(), sf::Lines);

		window.display();
	}

	return EXIT_SUCCESS;
}
