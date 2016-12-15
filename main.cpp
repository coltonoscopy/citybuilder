#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include "AnimatedSprite.hpp"
#include "tilemap.h"
#include "tiles.h"

bool percentChance(int percent) {
    return rand() % 100 < percent;
}

// Linear interpolation
float lerp(float a, float b, float t){
    return a * (1 - t) + b * t;
}

int main(void) {
    // create the window
    sf::RenderWindow window(sf::VideoMode(1920, 1080), "Tilemap", sf::Style::Fullscreen);
    window.setVerticalSyncEnabled(true);
    sf::Vector2i screenDimensions(1920, 1080);

    sf::View view;
    sf::View guiView;

    sf::Font font;
    font.loadFromFile("assets/04B_03__.TTF");

    sf::Text text("Tale Version 0.01", font);

    // collection of items
    std::vector<sf::Sprite> itemSprites;

    // keeps track of whether the player is currently moving
    bool playerMoving = false;

    // load the player
    sf::Texture creatureTexture;
    if (!creatureTexture.loadFromFile("assets/creatures.png")) {
        std::cout << "Failed to load creature spritesheet!" << std::endl;
        return 1;
    }

    // load the items sheet
    sf::Texture itemsTexture;
    int itemsWidth = 22;
    int itemsHeight = 14;
    if (!itemsTexture.loadFromFile("assets/items.png")) {
        std::cout << "Failed to load items spritesheet!" << std::endl;
        return 1;
    }

    // load the music
    sf::Music music;

    if (!music.openFromFile("assets/music.ogg")) {
        std::cout << "Failed to load music!" << std::endl;
        return 1;
    }

    music.setLoop(true);
    music.play();

    Animation walkingAnimationDown(creatureTexture);
    walkingAnimationDown.addFrames(2, CREATURE_SIZE, (const int[]){0, 18});

    Animation walkingAnimationLeft(creatureTexture);
    walkingAnimationLeft.addFrames(2, CREATURE_SIZE, (const int[]){0, 18});

    Animation walkingAnimationRight(creatureTexture);
    walkingAnimationRight.addFrames(2, CREATURE_SIZE, (const int[]){0, 18});

    Animation walkingAnimationUp(creatureTexture);
    walkingAnimationUp.addFrames(2, CREATURE_SIZE, (const int[]){0, 18});

    Animation* currentAnimation = &walkingAnimationDown;

    // set up AnimatedSprite
    AnimatedSprite animatedSprite(sf::seconds(0.2), true, false);

    // used to keep track of player movement lerping
    sf::Vector2f playerPosOld(2, 2);
    sf::Vector2f playerPosNew(2, 2);

    sf::Clock frameClock;
    sf::Clock lerpClock;

    bool noKeyWasPressed = true;

    // base text size
    const int TEXT_SIZE = 8;

    // text downscale to preserve crispness
    const float TEXT_SCALE = 0.125;

    // text multiplier before downscaling (to preserve crispness)
    const int TEXT_MULT = 16;

    // set text large and downscale to get large font without blur
    text.setCharacterSize(TEXT_SIZE * TEXT_MULT);
    text.setScale(sf::Vector2f(TEXT_SCALE, TEXT_SCALE));
    text.setPosition(sf::Vector2f(8, 0));
    text.setColor(sf::Color::White);

    // virtual viewport dimensions
    const int VIEWPORT_WIDTH = 640;
    const int VIEWPORT_HEIGHT = 360;

    // set camera in center of it
    int viewX = VIEWPORT_WIDTH / 2;
    int viewY = VIEWPORT_HEIGHT / 2;

    animatedSprite.setPosition(sf::Vector2f(
        CREATURE_SIZE * 2 + CREATURE_SIZE / 2,
        CREATURE_SIZE * 2 + CREATURE_SIZE / 2
    ));
    animatedSprite.setOrigin(sf::Vector2f(CREATURE_SIZE / 2, CREATURE_SIZE / 2));

    view.setSize(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    view.setCenter(
        CREATURE_SIZE * 2 + CREATURE_SIZE - 12,
        CREATURE_SIZE * 2 + CREATURE_SIZE - 12
    );
    window.setView(view);

    // separate GUI view to draw after everything else
    guiView.setSize(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    guiView.setCenter(viewX, viewY);

    // seed the RNG
    srand((unsigned)time(0));

    int map_width = 64;
    int map_height = 64;

    // define the level with its array of tile indices
    int level[map_width * map_height];
    int level2[map_width * map_height];

    // populate the tile array with random values between 0 and 2145
    for (int i = 0; i < map_width * map_height; i++) {
        level[i] = GRASS;
    }

    // populate second layer
    for (int i = 0; i < map_width * map_height; i++) {
        if (percentChance(50)) {
            level2[i] = EMPTY;
        } else {
            if (percentChance(25)) {
                level2[i] = PORTAL_DOOR;
            } else if (percentChance(25)) {
                level2[i] = CLOSED_CHEST;
            } else {
                level2[i] = EMPTY;
            }
        }
    }

    // populate randomized items
    for (int i = 0; i < 500; i++) {
        sf::Sprite item(itemsTexture);

        // randomize the texture rect
        int randomItem = rand() % (itemsWidth * itemsHeight);

        // calculate texture rect of item
        item.setTextureRect(sf::IntRect(
            randomItem % itemsHeight * ITEM_SIZE, randomItem / itemsHeight * ITEM_SIZE,
            ITEM_SIZE, ITEM_SIZE
        ));

        item.setPosition(sf::Vector2f(
            rand() % map_width * CREATURE_SIZE + 4,
            rand() % map_height * CREATURE_SIZE + 4
        ));

        itemSprites.push_back(item);
    }

    // create the tilemap from the level definition
    TileMap map;
    if (!map.load(("assets/tiles.png"), sf::Vector2u(CREATURE_SIZE, CREATURE_SIZE),
        level, map_width, map_height))
        return -1;

    TileMap map2;
    if (!map2.load(("assets/tiles.png"), sf::Vector2u(CREATURE_SIZE, CREATURE_SIZE),
        level2, map_width, map_height))
        return -1;

    // run the main loop
    while (window.isOpen()) {
        // handle events
        sf::Event event;

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            // keycodes that move window with WASD keys
            if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                    case sf::Keyboard::Escape:
                        window.close();
                        break;
                    default:
                        break;
                }
            }
        }

        sf::Time frameTime = frameClock.restart();

        // if a key was pressed set the correct animation and move correctly
        sf::Vector2f movement(0.f, 0.f);

        static const float MOVE_SPEED = 0.15f; // seconds it takes to move
        float t = lerpClock.getElapsedTime().asSeconds();

        // if the player is in his moving animation, lerp to destination
        if (playerMoving) {
            animatedSprite.setPosition(
                round(lerp(playerPosOld.x * CREATURE_SIZE + CREATURE_SIZE / 2,
                        playerPosNew.x * CREATURE_SIZE + CREATURE_SIZE / 2, t / MOVE_SPEED)),
                round(lerp(playerPosOld.y * CREATURE_SIZE + CREATURE_SIZE / 2,
                        playerPosNew.y * CREATURE_SIZE + CREATURE_SIZE / 2, t / MOVE_SPEED))
            );

            if (t >= MOVE_SPEED) {
                playerMoving = false;
                playerPosOld.x = playerPosNew.x;
                playerPosOld.y = playerPosNew.y;

                animatedSprite.setPosition(
                    playerPosNew.x * CREATURE_SIZE + CREATURE_SIZE / 2,
                    playerPosNew.y * CREATURE_SIZE + CREATURE_SIZE / 2
                );

                animatedSprite.setFrameTime(sf::seconds(0.2));
            }
        }

        if (!playerMoving) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
            {
                // make sure moving us won't put us out of map bounds
                if (playerPosOld.y - 1 >= 0) {
                    // check if the tile we want to move to is empty
                    if (level2[(int)((playerPosOld.y - 1) * map_width + playerPosOld.x)] == EMPTY) {
                        // move to that tile
                        playerMoving = true;
                        playerPosNew.y--;
                        lerpClock.restart();
                        currentAnimation = &walkingAnimationUp;
                        noKeyWasPressed = false;
                        animatedSprite.setFrameTime(sf::seconds(0.1));
                    }
                }
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
            {
                if (playerPosOld.y + 1 < map_height) {
                    if (level2[(int)((playerPosOld.y + 1) * map_width + playerPosOld.x)] == EMPTY) {
                        playerMoving = true;
                        playerPosNew.y++;
                        lerpClock.restart();
                        currentAnimation = &walkingAnimationDown;
                        noKeyWasPressed = false;
                        animatedSprite.setFrameTime(sf::seconds(0.1));
                    }
                }
            }
            else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
            {
                if (playerPosOld.x - 1 >= 0) {
                    if (level2[(int)((playerPosOld.y * map_width) + playerPosOld.x - 1)] == EMPTY) {
                        playerMoving = true;
                        playerPosNew.x--;
                        lerpClock.restart();
                        currentAnimation = &walkingAnimationLeft;
                        noKeyWasPressed = false;
                        animatedSprite.setFrameTime(sf::seconds(0.1));
                    }
                }
                animatedSprite.setScale(1, 1);
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
            {
                if (playerPosOld.x + 1 < map_width) {
                    if (level2[(int)((playerPosOld.y * map_width) + playerPosOld.x + 1)] == EMPTY) {
                        playerMoving = true;
                        playerPosNew.x++;
                        lerpClock.restart();
                        currentAnimation = &walkingAnimationRight;
                        noKeyWasPressed = false;
                        animatedSprite.setFrameTime(sf::seconds(0.1));
                    }
                }
                animatedSprite.setScale(-1, 1);
            }
        }

        animatedSprite.play(*currentAnimation);
        // animatedSprite.move(movement * frameTime.asSeconds());

        // if no key was pressed stop the animation
        if (noKeyWasPressed)
        {
            // animatedSprite.stop();
        }
        noKeyWasPressed = true;

        // update AnimatedSprite
        animatedSprite.update(frameTime);

        // cast floating-point coords to ints
        sf::Vector2f position = animatedSprite.getPosition();
        // animatedSprite.setPosition(round(position.x), round(position.y));

        view.setCenter(
            round(position.x) + CREATURE_SIZE / 2 - 12,
            round(position.y) + CREATURE_SIZE / 2 - 12
        );
        window.setView(view);

        // draw the map
        window.clear();
        window.draw(map);
        window.draw(map2);

        // iterate and draw items
        for (std::vector<sf::Sprite>::iterator it = itemSprites.begin(); it != itemSprites.end(); it++) {
            window.draw(*it);
        }

        // draw player
        window.draw(animatedSprite);

        // draw the GUI
        window.setView(guiView);
        window.draw(text);

        window.display();
    }

    return 0;
}
