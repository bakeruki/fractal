#include <SFML/Graphics.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <sstream>

//returns the mod of two numbers squared
float mod2(float x, float y) {
    return x * x + y * y;
}

//calculates the number of iterations at a coordinate required to escape
int calculateIterations(sf::Vector2i current, sf::Vector2f c, int maxIterations, sf::Vector2i dimensions, float zoom) {
    float zr = ((float)current.x - (float)dimensions.x / 2) / ((float)dimensions.x / 2) / zoom;
    float zi = ((float)current.y - (float)dimensions.y / 2) / ((float)dimensions.y / 2) / zoom;
    int iter = 0;

    while (iter < maxIterations && mod2(zr, zi) < 4.0) {
        float temp = zr;
        zr = zr * zr - zi * zi + c.x;
        zi = 2 * temp * zi + c.y;

        iter++;
    }
    //smoothing operations
    float mod = sqrt(mod2(zr, zi));
    float smoothIter = float(iter) - log2(std::max(1.0f, log2(mod)));
    return smoothIter;
}

//generates the image normally
void generate(sf::VertexArray& image, sf::Vector2f c, float zoom, int maxIterations, sf::Vector2i dimensions) {
    int screenWidth = dimensions.x;
    int screenHeight = dimensions.y;

    auto timeBefore = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    for (int x = 0; x < screenWidth; x++) {
        for (int y = 0; y < screenHeight; y++) {
            sf::Vector2i current = { x, y };
            int iter = calculateIterations(current, c, maxIterations, sf::Vector2i(screenWidth, screenHeight), zoom);

            int index = y * screenWidth + x;
            int colorVal = iter * 255 / maxIterations;
            image[index] = sf::Vertex(sf::Vector2f((float)x, (float)y), sf::Color(colorVal, colorVal, colorVal, 255));
        }
    }

    auto timeAfter = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
   
    auto timeDiff = timeAfter - timeBefore;

    std::cout << "done generating in " << timeDiff << " milliseconds" << std::endl;
}

//generates the image using multithreading
void generateMultithread(sf::VertexArray& image, sf::Vector2f c, float zoom, int maxIterations, sf::Vector2i dimensions, int numThreads, int k) {
    int screenWidth = dimensions.x;
    int screenHeight = dimensions.y;

    float sliceHeight = screenHeight / numThreads;

    for (int x = 0; x < screenWidth; x++) {
        for (int y = k * sliceHeight; y < (k + 1) * sliceHeight; y++) {
            sf::Vector2i current = { x, y };
            int iter = calculateIterations(current, c, maxIterations, sf::Vector2i(screenWidth, screenHeight), zoom);

            int index = y * screenWidth + x;
            int colorVal = iter * 255 / maxIterations;
            image[index] = sf::Vertex(sf::Vector2f((float)x, (float)y), sf::Color(colorVal, colorVal, colorVal, 255));
        }
    }
}

//dispatches the threads to begin multithreaded generation
void beginMultithreadGeneration(sf::VertexArray& image, sf::Vector2f c, float zoom, int maxIterations, sf::Vector2i dimensions, int numThreads) {
    std::vector<std::thread> workers;

    auto timeBefore = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    for (int i = 0; i < numThreads; i++) {
        workers.push_back(std::thread([&image, c, zoom, maxIterations, dimensions, i, numThreads]()
            {
                generateMultithread(image, c, zoom, maxIterations, dimensions, numThreads, i);
            }));
    }

    std::for_each(workers.begin(), workers.end(), [](std::thread& t)
        {
            t.join();
        });

    auto timeAfter = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    auto timeDiff = timeAfter - timeBefore;

    std::cout << "done generating in " << timeDiff << " milliseconds\n";
}

int main()
{
    const int screenWidth = 1600;
    const int screenHeight = 900;
    const int numThreads = 16;
    const float zoomSpeed = 0.2;

    sf::RenderWindow window(sf::VideoMode(screenWidth, screenHeight), "fractal");
    window.setFramerateLimit(20);
    sf::VertexArray image(sf::Points, screenHeight * screenWidth);

    const int maxIterations = 500;
    sf::Vector2f c = sf::Vector2f(-0.8, 0.156);
    float zoom = 0.65;

    bool incrementing = false;
    bool zoomingIn = false;
    bool zoomingOut = false;
    bool incrementX = false;
    bool incrementY = false;

    sf::Font font;
    if (!font.loadFromFile("cambriaz.ttf"))
    {
        std::cout << "text didnt load\n";
    }
    sf::Text text;
    text.setFont(font);
    std::ostringstream oss;
    oss << "c = " << c.x << " + " << c.y << "i";
    std::string string = oss.str();
    text.setString(string);
    text.setCharacterSize(70);
    text.setFillColor(sf::Color::White);

    std::cout << "singlethread generation running\n";
    generate(image, c, zoom, maxIterations, sf::Vector2i(screenWidth, screenHeight));

    std::cout << "multithread generation running with " << numThreads << " threads\n";
    beginMultithreadGeneration(image, c, zoom, maxIterations, sf::Vector2i(screenWidth, screenHeight), numThreads);

    sf::Clock deltaClock;
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyReleased) {
                if (event.key.scancode == sf::Keyboard::Scan::E) {
                    if (incrementing) {
                        incrementing = false;
                    }
                    else {
                        incrementing = true;
                    }
                }

                if (event.key.scancode == sf::Keyboard::Scan::X) {
                    if (zoomingIn) {
                        zoomingIn = false;
                    }
                    else {
                        zoomingOut = false;
                        zoomingIn = true;
                    }
                }

                if (event.key.scancode == sf::Keyboard::Scan::Z) {
                    if (zoomingOut) {
                        zoomingOut = false;
                    }
                    else {
                        zoomingIn = false;
                        zoomingOut = true;
                    }
                }
            }
        }

        window.clear();

        window.draw(image);
        window.draw(text);

        window.display();

        sf::Time dt = deltaClock.restart();

        //zoom += zoomSpeed * dt.asSeconds();

        if (incrementing) {
            if (c.x > 1) {
                incrementX = false;
            }
            else if (c.x < -1) {
                incrementX = true;
            }

            if (c.y > 1) {
                incrementY = false;
            }
            else if (c.y < -1) {
                incrementY = true;
            }

            if (incrementX) {
                c.x += 0.005;
            }
            else {
                c.x -= 0.005;
            }

            if (incrementY) {
                c.y += 0.005;
            }
            else {
                c.y -= 0.005;
            }

            beginMultithreadGeneration(image, c, zoom, maxIterations, sf::Vector2i(screenWidth, screenHeight), numThreads);

            oss.str("");
            oss << "c = " << c.x << " + " << c.y << "i";
            string = oss.str();
            text.setString(string);
        }

        if (zoomingIn) {
            zoom += 0.1;
            beginMultithreadGeneration(image, c, zoom, maxIterations, sf::Vector2i(screenWidth, screenHeight), numThreads);
            oss.str("");
            oss << "zoom: " << zoom;
            string = oss.str();
            text.setString(string);
        }else if (zoomingOut) {
            zoom -= 0.1;
            beginMultithreadGeneration(image, c, zoom, maxIterations, sf::Vector2i(screenWidth, screenHeight), numThreads);
            oss.str("");
            oss << "zoom: " << zoom;
            string = oss.str();
            text.setString(string);
        }
    }

    return 0;
}